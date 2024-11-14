/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "shmem.h"

#include <cstring>
#include <mutex>
#include <sys/mman.h>
#include <unistd.h>
#include <shared_mutex>
#include <stdexcept>
#include <utility>
#include <cmath>

#include <unistd.h>
namespace {
    size_t page_size = sysconf(_SC_PAGESIZE);
}

shmem::shmem() {
    attach([](shmem&){});
}

shmem::shmem(std::function<void(shmem&)> on_create) {
    attach(std::move(on_create));
}

shmem::~shmem() {
    close();
}

void shmem::attach(std::function<void(shmem&)> on_create) {
    // Metadata shared memory segment
    {
        int meta_fd = shm_open("shmem_meta", O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        bool created = false;

        if (meta_fd != -1) {
            // Successfully created
            created = true;
            if (ftruncate(meta_fd, sizeof(shmem_meta)) == -1) {
                throw std::runtime_error("Failed to truncate shared memory segment for metadata");
            }
        } else {
            // If the segment already exists, open it normally
            meta_fd = shm_open("shmem_meta", O_RDWR, S_IRUSR | S_IWUSR);
            if (meta_fd == -1) {
                throw std::runtime_error("Failed to open shared memory segment for metadata");
            }
        }

        metadata = static_cast<shmem_meta *>(mmap(nullptr, sizeof(shmem_meta), PROT_READ | PROT_WRITE, MAP_SHARED,
                                                  meta_fd, 0));
        if (metadata == MAP_FAILED) {
            throw std::runtime_error("Failed to mmap shared memory segment for metadata");
        }
        mlock(metadata, sizeof(shmem_meta));
        ::close(meta_fd);

        // If this was a new segment, initialize metadata fields
        if (created) {
            new (metadata) shmem_meta();
        }
    }

    // Data shared memory segment
    {
        int data_fd = shm_open("shmem_data", O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        bool created = false;

        if (data_fd != -1) {
            // Successfully created
            created = true;
            if (ftruncate(data_fd, metadata->alloc_size) == -1) {
                printf("%s", strerror(errno));
                throw std::runtime_error("Failed to truncate shared memory segment for data");
            }
        } else {
            // If the segment already exists, open it normally
            data_fd = shm_open("shmem_data", O_RDWR, S_IRUSR | S_IWUSR);
            if (data_fd == -1) {
                throw std::runtime_error("Failed to open shared memory segment for data");
            }
        }

        base = mmap(nullptr, metadata->alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
        if (base == MAP_FAILED) {
            printf("%s", strerror(errno));
            throw std::runtime_error("Failed to mmap shared memory segment for data");
        }
        mlock(base, metadata->alloc_size);
        local_size = metadata->alloc_size;
        ::close(data_fd);
        if (created) {
            on_create(*this);
        }
    }
}

void shmem::close() {
    if (base) {
        munmap(base, metadata->alloc_size);
        base = nullptr;
    }
    if (metadata) {
        munmap(metadata, sizeof(shmem_meta));
        metadata = nullptr;
    }
}

void shmem::destroy() {
    if (base) {
        munmap(base, metadata->alloc_size);
        base = nullptr;
    }

    if (metadata) {
        metadata->~shmem_meta();
        munmap(metadata, sizeof(shmem_meta));
        metadata = nullptr;
    }
    shm_unlink("shmem_meta");
    shm_unlink("shmem_data");
}

void shmem::resize(offset_t min_size) {
    int flags = O_RDWR;
    offset_t prev_size = metadata->used_size;
    auto* tmp = static_cast<uint8_t*>(mmap(nullptr, prev_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0)); // Use mmap to allocate a temporary buffer
    if (tmp == MAP_FAILED) {
        throw std::runtime_error("Failed to allocate temporary buffer for resize");
    }
    memcpy(tmp, base, prev_size);

    // Unmap & delete existing data segment
    munmap(base, local_size);
    shm_unlink("shmem_data");
    flags |= O_CREAT;
    // Increase `alloc_size` (double it, for example) and truncate to new size
    metadata->alloc_size = (min_size * 2 + page_size - 1) & ~(page_size - 1);;
    int data_fd = shm_open("shmem_data", flags, S_IRUSR | S_IWUSR);
    if (data_fd == -1) {
        throw std::runtime_error("Failed to open shared memory segment for data after resize");
    }
    if (ftruncate(data_fd, metadata->alloc_size) == -1) {
        throw std::runtime_error("Failed to truncate shared memory segment for data after resize");
    }

    // We dont use mremap because its not supported on macos
    base = mmap(nullptr, metadata->alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
    if (base == MAP_FAILED) {
        throw std::runtime_error("Failed to mmap shared memory segment for data after resize");
    }
    mlock(base, metadata->alloc_size);
    ::close(data_fd);

    // Try touching every page to ensure they are backed by physical memory
    for (size_t i = 0; i < metadata->alloc_size; i += page_size) {
        ((volatile uint8_t *)base)[i] = 0;
    }

    memcpy(base, tmp, prev_size);
    munmap(tmp, prev_size); // Unmap the temporary buffer

    ++metadata->version;
    local_size = metadata->alloc_size;
}

offset_t shmem::allocate(std::size_t size) {
    std::unique_lock<shrwlock> lk2(metadata->meta_mutex);
    // Ensure we have enough space, resize if needed
    if (metadata->used_size + size > metadata->alloc_size) {
        resize(metadata->used_size + size);
    }

    // Compute the offset, allocate space, and return pointer
    std::size_t offset = metadata->used_size;
    metadata->used_size += size;

    return offset;
}

void shmem::deallocate(offset_t ptr) {
    // This is currently left free for this simple bump allocator
}

void shmem::atomic_access(offset_t offset, std::function<void(void *)>&& lambda, bool locks_held) {
    auto do_access = [this, &lambda, &offset]() {
        {
            if (metadata->version > local_version) { // If the version has changed, remap the data segment
                munmap(base, local_size);
                int data_fd = shm_open("shmem_data", O_RDWR, S_IRUSR | S_IWUSR);
                base = mmap(nullptr, metadata->alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
                if (base == MAP_FAILED) {
                    throw std::runtime_error("Failed to mmap shared memory segment for data after version change");
                }
                mlock(base, metadata->alloc_size);
                ::close(data_fd);
                local_version = metadata->version;
                local_size = metadata->alloc_size;
            } else if (metadata->version < local_version) {
                throw std::runtime_error("Version mismatch");
            }
        }

        auto *ptr = reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(base) + offset);
        lambda(ptr);
    };

    // Atomically lock both semaphores
    if (!locks_held) {
        std::shared_lock<shrwlock> lk1(metadata->meta_mutex);
        do_access();
    } else {
        do_access();
    }
}

offset_t shmem::get_management_offset() const {
    return 0;
}

shrwlock &shmem::get_meta_mutex() {
    return metadata->meta_mutex;
}
