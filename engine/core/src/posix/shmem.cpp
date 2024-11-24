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

#ifndef NDEBUG
#define MEM_DEBUG 1
#endif

namespace {
    size_t page_size = sysconf(_SC_PAGESIZE);
}

struct FreeBlock {
#ifdef MEM_DEBUG
    char magic[4] = {'F', 'R', 'E', 'E'};
#endif
    size_t size;
    size_t next_offset;
};

struct AllocationHeader {
    size_t size;
};

FreeBlock* get_first_free_block(const shmem_meta& meta, void* base) {
    return reinterpret_cast<FreeBlock*>(reinterpret_cast<uint8_t*>(base) + meta.first_free);
}

FreeBlock* get_free_block(void* base, size_t offset) {
    return reinterpret_cast<FreeBlock*>(reinterpret_cast<uint8_t*>(base) + offset);
}


size_t get_offset_from_ptr(void* base, void* ptr) {
    return reinterpret_cast<uint8_t*>(ptr) - reinterpret_cast<uint8_t*>(base);
}

void free_block_insert(shmem_meta& metadata, void* base, size_t prev_node, FreeBlock *new_node) {
    if (prev_node == -1) { // This is the first element in the list
        if (metadata.first_free != -1) { // There are other elements in the list
            new_node->next_offset = metadata.first_free;
        }
        metadata.first_free = get_offset_from_ptr(base, new_node); // Update the first free block
    } else {
        FreeBlock* prev_block = get_free_block(base, prev_node);
        if (prev_block->next_offset == -1) { // This is the last element in the list
            prev_block->next_offset = get_offset_from_ptr(base, new_node);
            new_node->next_offset = -1;
        } else { // This is not the last element
            new_node->next_offset  = prev_block->next_offset;
            prev_block->next_offset = get_offset_from_ptr(base, new_node);
        }
    }
}

void free_block_remove(shmem_meta& metadata, void* base, size_t prev_node, FreeBlock *node) {
    if (prev_node == -1) { // This is the first element in the list
        metadata.first_free = node->next_offset;
    } else {
        FreeBlock* prev_block = get_free_block(base, prev_node);
        prev_block->next_offset = node->next_offset;
    }
}

void free_coalescence(shmem_meta& metadata, void* base, size_t prev_node_off, FreeBlock *free_node) {
    auto* next_free_node = get_free_block(base, free_node->next_offset);
    if (free_node->next_offset != -1 && (void *)((char *)free_node + free_node->size) == next_free_node) {
        free_node->size += next_free_node->size;
        free_block_remove(metadata, base, get_offset_from_ptr(base, free_node), next_free_node);
    }
    if (prev_node_off == -1) return;
    auto* prev_node = get_free_block(base, prev_node_off);
    if (prev_node->next_offset != -1 && (void *)((char *)prev_node + prev_node->size) == free_node) {
        prev_node->size += free_node->size;
        free_block_remove(metadata, base, prev_node_off, free_node);
    }
}

size_t find_tail_block(shmem_meta& metadata, void* base) {
    size_t current = metadata.first_free;
    while (current != -1) {
        auto* currentblock = get_free_block(base, current);
        if (currentblock->next_offset == -1) {
            return current;
        }
        current = currentblock->next_offset;
    }
    return -1;
}

size_t find_free_block(shmem_meta& metadata, void* base, size_t size, size_t& prev) {
    prev = -1;
    size_t current = metadata.first_free;

    // Traverse the free list to find a block with enough space
    while (current != -1) {
        auto* currentblock = get_free_block(base, current);
        if (currentblock->size >= size + sizeof(FreeBlock)) {
            return current;
        }

        prev = current;
        current = currentblock->next_offset;
    }
    return -1;
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

void shmem::ensure_enough_space(std::size_t required_memory) {
    if (can_fit(required_memory + sizeof(FreeBlock))) { // We need this to fit the required memory and possibly another FreeBlock
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

offset_t shmem::allocate(std::size_t size) {
    size += sizeof(AllocationHeader); // We also need an allocation header to store the size of the block
    size = std::max(size, sizeof (FreeBlock)); // We need to ensure that the block is at least the size of a FreeBlock
    std::unique_lock<shrwlock> lk2(metadata->meta_mutex);
    ensure_mounted();
    // Ensure we have enough space, resize if needed
    ensure_enough_space(size);

    size_t prev;
    size_t newblock = find_free_block(*metadata, base, size, prev);
    if (newblock == -1) {
        // No suitable block found
        throw std::runtime_error("Failed to allocate memory in shared memory segment");
    }

    auto* newblockptr = get_free_block(base, newblock);
    size_t required_space = size;
    size_t remaining = newblockptr->size - required_space;
    if (remaining > sizeof(FreeBlock)) {
        // Split the block
        auto* nextblock = get_free_block(base, newblock + required_space);
#ifdef MEM_DEBUG
        new (nextblock) FreeBlock();
#endif
        nextblock->size = remaining;
        free_block_insert(*metadata, base, newblock, nextblock);
    }

    free_block_remove(*metadata, base, prev, newblockptr);

    auto* header_ptr = reinterpret_cast<AllocationHeader*>(newblockptr);
    header_ptr->size = required_space;
    return newblock + sizeof(AllocationHeader);
}

void shmem::deallocate(offset_t ptr) {
    std::unique_lock<shrwlock> lk2(metadata->meta_mutex);
    ensure_mounted();

    auto* header_ptr = reinterpret_cast<AllocationHeader*>(reinterpret_cast<uint8_t*>(base) + ptr - sizeof(AllocationHeader));
    size_t size = header_ptr->size;

    auto* newblock = reinterpret_cast<FreeBlock*>(header_ptr);
#ifdef MEM_DEBUG
    new (newblock) FreeBlock();
#endif
    newblock->size = size;

    size_t prev = -1;
    size_t curr = metadata->first_free;
    while (curr != -1) {
        if (curr > ptr) { // Find the block that is greater than the new block
            free_block_insert(*metadata, base, prev, newblock);
            break;
        }
        auto* currblock = get_free_block(base, curr);
        prev = curr;
        curr = currblock->next_offset;
    }

    free_coalescence(*metadata, base, prev, newblock);
}

void shmem::atomic_access(offset_t offset, std::function<void(void *)>&& lambda, bool locks_held) {
    auto do_access = [this, &lambda, &offset]() {
        {
            ensure_mounted();
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
    return sizeof(AllocationHeader);
}

shrwlock &shmem::get_meta_mutex() {
    return metadata->meta_mutex;
}

bool shmem::can_fit(std::size_t required_memory) {
    size_t current = metadata->first_free;
    while (current != -1) {
        auto* currentblock = get_free_block(base, current);
        if (currentblock->size >= required_memory) return true;
        current = currentblock->next_offset;
    }
    return false;
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
