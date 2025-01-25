/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "shmem.h"
#include "shmem_util.h"

#include <fcntl.h>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <shared_mutex>
#include <stdexcept>

namespace {
    size_t page_size = sysconf(_SC_PAGESIZE);
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
            // Initially we set the first free block to the entire segment
            FreeBlock* freeList = get_first_free_block(*metadata, base);
#ifdef MEM_DEBUG
            new (freeList) FreeBlock();
#endif
            freeList->size = metadata->alloc_size;
            freeList->next_offset = -1;
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

void shmem::destroy_data() {
    shm_unlink("shmem_meta");
    shm_unlink("shmem_data");
}

void shmem::ensure_enough_space(std::size_t required_memory, std::size_t required_alignment) {
    if (can_fit(required_memory, required_alignment)) { // We need this to fit the required memory and possibly another FreeBlock
        return;
    }
    int flags = O_RDWR;
    std::size_t prev_size = metadata->alloc_size;
    auto* tmp = static_cast<uint8_t*>(mmap(nullptr, prev_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0)); // Use mmap to allocate a temporary buffer
    if (tmp == MAP_FAILED) {
        throw std::runtime_error("Failed to allocate temporary buffer for ensure_enough_space");
    }
    memcpy(tmp, base, prev_size);

    // Unmap & delete existing data segment
    munmap(base, local_size);
    shm_unlink("shmem_data");
    flags |= O_CREAT;
    // Increase `alloc_size` (double it, for example) and truncate to new size
    std::size_t new_size = metadata->alloc_size;
    while (new_size < metadata->alloc_size + required_memory) {
        new_size *= 2;
    }
    metadata->alloc_size = new_size;
    int data_fd = shm_open("shmem_data", flags, S_IRUSR | S_IWUSR);
    if (data_fd == -1) {
        throw std::runtime_error("Failed to open shared memory segment for data after ensure_enough_space");
    }
    if (ftruncate(data_fd, metadata->alloc_size) == -1) {
        throw std::runtime_error("Failed to truncate shared memory segment for data after ensure_enough_space");
    }

    // We dont use mremap because its not supported on macos
    base = mmap(nullptr, metadata->alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
    if (base == MAP_FAILED) {
        throw std::runtime_error("Failed to mmap shared memory segment for data after ensure_enough_space");
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
    local_version = metadata->version;
    local_size = metadata->alloc_size;

    // Find last block and change size
    size_t tail = find_tail_block(*metadata, base);
    FreeBlock* tailblock = get_free_block(base, tail);
    if (tail != -1 && tail + tailblock->size == prev_size) { // This block is at the end so we can just extend it
        tailblock->size = metadata->alloc_size - tail;
    } else { // The block is not at the end so we need to create a new block
        size_t new_block_offset = prev_size;
        FreeBlock* newblock = get_free_block(base, new_block_offset);
#ifdef MEM_DEBUG
        new (newblock) FreeBlock();
#endif
        newblock->next_offset = -1;
        newblock->size = metadata->alloc_size - prev_size;
        if (tail != -1) {
            tailblock->next_offset = new_block_offset;
        } else {
            metadata->first_free = new_block_offset;
        }
    }
}

void shmem::ensure_mounted() {
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

void shmem::load_data(const shmem::dump_t& data) {
    const auto& meta = data.first;
    const auto& mem = data.second;
    std::unique_lock<shrwlock> lk(metadata->meta_mutex);
    ensure_mounted();
    if (base) {
        munmap(base, metadata->alloc_size);
        base = nullptr;
    }
    shm_unlink("shmem_data");

    metadata->version = meta.version;
    metadata->alloc_size = meta.alloc_size;
    metadata->first_free = meta.first_free;
    local_version = metadata->version;

    // Re-Create the shared memory
    int data_fd = shm_open("shmem_data", O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

    if (data_fd != -1) {
        // Successfully created
        if (ftruncate(data_fd, metadata->alloc_size) == -1) {
            printf("%s", strerror(errno));
            throw std::runtime_error("Failed to truncate shared memory segment for data");
        }
    } else {
        throw std::runtime_error("The shared memory segment for data already exists");
    }

    base = mmap(nullptr, metadata->alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
    if (base == MAP_FAILED) {
        printf("%s", strerror(errno));
        throw std::runtime_error("Failed to mmap shared memory segment for data");
    }
    mlock(base, metadata->alloc_size);
    local_size = metadata->alloc_size;
    ::close(data_fd);

    std::memcpy(base, mem.data(), metadata->alloc_size);
}