/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "shmem.h"
#include <sys/mman.h>
#include <unistd.h>
#include <mutex>
#include <shared_mutex>

shmem::shmem() {
    attach();
}

shmem::~shmem() {
    close();
}

void shmem::attach() {
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
        ::close(data_fd);
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
    metadata->~shmem_meta();

    close();
    shm_unlink("shmem_meta");
    shm_unlink("shmem_data");
}

void shmem::resize(offset_t min_size) {
    // Atomically lock both semaphores
    std::lock(metadata->data_mutex, metadata->meta_mutex);
    std::lock_guard<shrwlock> lk1(metadata->data_mutex, std::adopt_lock);
    std::lock_guard<shrwlock> lk2(metadata->meta_mutex, std::adopt_lock);

    int flags = O_RDWR;
#ifdef __APPLE__ // Unfortunately on darwin you can only call ftruncate once per shared memory segment. So we recerate a new one and move the data over
    offset_t prev_size = metadata->alloc_size;
    void* tmp = malloc(prev_size);
    memcpy(tmp, base, prev_size);

    // Unmap & delete existing data segment
    munmap(base, metadata->alloc_size);
    shm_unlink("shmem_data");
    flags |= O_CREAT;
#else
    munmap(base, metadata->alloc_size);
#endif
    // Increase `alloc_size` (double it, for example) and truncate to new size
    metadata->alloc_size = min_size * 2;
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
    ::close(data_fd);

#ifdef __APPLE__
    memcpy(base, tmp, prev_size);
    free(tmp);
#endif

    ++metadata->version;
}

void* shmem::access(offset_t offset) {
    {
        // TODO This semaphore is very slow and should be replaced with a more efficient solution
        std::shared_lock<shrwlock> lk2(metadata->meta_mutex);
        if (metadata->version > local_version) { // If the version has changed, remap the data segment
            munmap(base, metadata->alloc_size);
            int data_fd = shm_open("shmem_data", O_RDWR, 0);
            base = mmap(nullptr, metadata->alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
            ::close(data_fd);
            local_version = metadata->version;
        } else if (metadata->version < local_version) {
            throw std::runtime_error("Version mismatch");
        }
    }

    // TODO This is not good, we should lock the data semaphore while accessing the data segment but since we return a pointer for now we ignore possible race conditions on the pointers value
    return reinterpret_cast<uint8_t *>(base) + offset;
}

offset_t shmem::allocate(std::size_t size) {
    // Ensure we have enough space, resize if needed
    if (metadata->used_size + size > metadata->alloc_size) {
        resize(metadata->used_size + size);
    }

    // Compute the offset, allocate space, and return pointer
    std::size_t offset = metadata->used_size;
    metadata->used_size += size;
    return offset;
}

void shmem::deallocate(offset_ptr<void> ptr) {
    // This is currently left free for this simple bump allocator
}

void shmem::atomic_write(offset_t offset, std::function<void(void *)>&& lambda, bool locks_held) {
    auto do_access = [this, lambda, offset]() {
        {
            if (metadata->version > local_version) { // If the version has changed, remap the data segment
                munmap(base, metadata->alloc_size);
                int data_fd = shm_open("shmem_data", O_RDWR, 0);
                base = mmap(nullptr, metadata->alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
                ::close(data_fd);
                local_version = metadata->version;
            } else if (metadata->version < local_version) {
                throw std::runtime_error("Version mismatch");
            }
        }

        auto *ptr = reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(base) + offset);
        lambda(ptr);
    };

    // Atomically lock both semaphores
    if (!locks_held) {
        std::lock(metadata->meta_mutex, metadata->data_mutex);
        std::lock_guard<shrwlock> lk1(metadata->meta_mutex, std::adopt_lock);
        std::lock_guard<shrwlock> lk2(metadata->data_mutex, std::adopt_lock);
        do_access();
    } else {
        do_access();
    }
}

void shmem::atomic_read(offset_t offset, std::function<void(const void *)>&& lambda, bool locks_held) {
    auto do_access = [this, lambda, offset]() {
        {
            if (metadata->version > local_version) { // If the version has changed, remap the data segment
                munmap(base, metadata->alloc_size);
                int data_fd = shm_open("shmem_data", O_RDWR, 0);
                base = mmap(nullptr, metadata->alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
                ::close(data_fd);
                local_version = metadata->version;
            } else if (metadata->version < local_version) {
                throw std::runtime_error("Version mismatch");
            }
        }

        auto *ptr = reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(base) + offset);
        lambda(ptr);
    };

    // Atomically lock both semaphores
    if (!locks_held) {
        // TODO These should be locked simultaneously
        std::shared_lock<shrwlock> lk1(metadata->meta_mutex);
        std::shared_lock<shrwlock> lk2(metadata->data_mutex);
        do_access();
    } else {
        do_access();
    }
}
