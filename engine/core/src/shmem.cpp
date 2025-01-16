/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "shmem.h"

#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <utility>
#include <cmath>
#include <cassert>
#include "shmem_util.h"

shmem::shmem() {
    attach([](shmem&){});
}

shmem::shmem(std::function<void(shmem&)> on_create) {
    attach(std::move(on_create));
}

shmem::~shmem() {
    close();
}

offset_t shmem::allocate(std::size_t size, std::size_t alignment, bool lock_held) {
    //size += sizeof(AllocationHeader); // We also need an allocation header to store the size of the block
    if (size < sizeof(FreeBlock)) { // We need to ensure that the block is at least the size of a FreeBlock
        size = sizeof(FreeBlock);
    }
    if (alignment < 8) {
        alignment = 8;
    }
    std::unique_lock<shrwlock> lk2(metadata->meta_mutex, std::defer_lock);
    if (!lock_held) {
        lk2.lock();
    }
    ensure_mounted();
    // Ensure we have enough space, resize if needed
    ensure_enough_space(size, alignment);

    size_t prev;
    size_t padding;
    size_t newblock = find_free_block(*metadata, base, size, alignment, padding, prev);
    if (newblock == -1) {
        // No suitable block found
        throw std::runtime_error("Failed to allocate memory in shared memory segment");
    }
    assert(newblock >= metadata->first_free);

    auto* newblockptr = get_free_block(base, newblock);
    VALIDATE_FREE_BLOCK(newblockptr);

    size_t alignment_padding = padding - sizeof(AllocationHeader);
    size_t required_space = size + padding;
    size_t remaining = newblockptr->size - required_space;
    if (remaining > sizeof(FreeBlock)) {
        // Split the block
        auto* nextblock = get_free_block(base, newblock + required_space);
#ifdef MEM_DEBUG
        new (nextblock) FreeBlock();
#endif
        nextblock->size = remaining;
        free_block_insert(*metadata, base, newblock, nextblock);
        VALIDATE_FREE_BLOCK(nextblock);
    } else {
        // If the remaining space is not enough for a FreeBlock, just use the entire block
        required_space = newblockptr->size;
    }

    free_block_remove(*metadata, base, prev, newblockptr);
    if (prev != -1) {
        VALIDATE_FREE_BLOCK(get_free_block(base, prev));
    }
    VALIDATE_FREE_BLOCK(newblockptr);

    auto* header_ptr = reinterpret_cast<AllocationHeader*>(newblockptr + alignment_padding);
#ifdef MEM_DEBUG
    new (header_ptr) AllocationHeader();
#endif
    header_ptr->size = required_space;
    header_ptr->padding = alignment_padding;
    VALIDATE_ALLOC_HEADER(header_ptr);
    return newblock + sizeof(AllocationHeader);
}

void shmem::deallocate(offset_t ptr, bool lock_held) {
    std::unique_lock<shrwlock> lk2(metadata->meta_mutex, std::defer_lock);
    if (!lock_held) {
        lk2.lock();
    }
    ensure_mounted();

    auto* header_ptr = reinterpret_cast<AllocationHeader*>(reinterpret_cast<uint8_t*>(base) + ptr - sizeof(AllocationHeader));
#if MEM_DEBUG
    auto* freeblock = reinterpret_cast<FreeBlock*>(header_ptr);
    if (freeblock->magic[0] == 'F' && freeblock->magic[1] == 'R' && freeblock->magic[2] == 'E' && freeblock->magic[3] == 'E') { \
        throw std::runtime_error("Double Free Detected");
    }
#endif
    VALIDATE_ALLOC_HEADER(header_ptr);
    size_t size = header_ptr->size + header_ptr->padding;

    auto* newblock = reinterpret_cast<FreeBlock*>(header_ptr);
#ifdef MEM_DEBUG
    new (newblock) FreeBlock();
#endif
    newblock->size = size;
    newblock->next_offset = -1;

    size_t prev = -1;
    size_t curr = metadata->first_free;
    while (curr != -1) {
        if (curr > ptr) { // Find the block that is greater than the new block
            free_block_insert(*metadata, base, prev, newblock);
            if (prev != -1) {
                VALIDATE_FREE_BLOCK(get_free_block(base, prev));
            }
            VALIDATE_FREE_BLOCK(newblock);
            VALIDATE_FREE_BLOCK(get_free_block(base, curr));
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

offset_t shmem::get_used_memory() {
    std::shared_lock<shrwlock> lock_guard(metadata->meta_mutex);
    ensure_mounted();

    auto free = 0;
    size_t current = metadata->first_free;
    while (current != -1) {
        auto* currentblock = get_free_block(base, current);
        VALIDATE_FREE_BLOCK(currentblock);
        free += currentblock->size;
        current = currentblock->next_offset;
    }
    return metadata->alloc_size - free;
}


shrwlock &shmem::get_meta_mutex() {
    return metadata->meta_mutex;
}

bool shmem::can_fit(std::size_t required_memory, std::size_t required_alignment) {
    size_t current = metadata->first_free;
    while (current != -1) {
        auto* currentblock = get_free_block(base, current);
        VALIDATE_FREE_BLOCK(currentblock);
        size_t padding = calc_padding_with_header((uintptr_t)currentblock, (uintptr_t)required_alignment, sizeof(AllocationHeader));
        if (currentblock->size >= required_memory + padding) return true;
        current = currentblock->next_offset;
    }
    return false;
}


shmem::dump_t shmem::dump_data() {
    std::unique_lock<shrwlock> lk(metadata->meta_mutex);
    ensure_mounted();

    dump_meta meta;
    meta.version = metadata->version;
    meta.alloc_size = metadata->alloc_size;
    meta.first_free = metadata->first_free;

    std::vector<uint8_t> data(metadata->alloc_size);
    std::memcpy(data.data(), base, metadata->alloc_size);

    return {meta, std::move(data)};
}