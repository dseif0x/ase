/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
//! @cond
#pragma once

#include <cassert>
#include <stdexcept>

#ifndef NDEBUG
#define MEM_DEBUG 1
#endif

#ifdef MEM_DEBUG

#define VALIDATE_FREE_BLOCK(block) \
    if (block->magic[0] != 'F' || block->magic[1] != 'R' || block->magic[2] != 'E' || block->magic[3] != 'E') { \
        throw std::runtime_error("Invalid free block"); \
    }

#define VALIDATE_ALLOC_HEADER(block) \
    if (block->magic[0] != 'A' || block->magic[1] != 'L' || block->magic[2] != 'H' || block->magic[3] != 'R') { \
        throw std::runtime_error("Invalid allocation header"); \
    }
#else
#define VALIDATE_FREE_BLOCK(block)
#define VALIDATE_ALLOC_HEADER(block)
#endif

struct FreeBlock {
#ifdef MEM_DEBUG
    char magic[4] = {'F', 'R', 'E', 'E'};
#endif
    size_t size;
    size_t next_offset;
};

struct AllocationHeader {
#ifdef MEM_DEBUG
    char magic[4] = {'A', 'L', 'H', 'R'};
#endif
    size_t size;
    size_t padding;
};

inline bool is_power_of_two(uintptr_t x) {
    return (x & (x-1)) == 0;
}

inline size_t calc_padding_with_header(uintptr_t ptr, uintptr_t alignment, size_t header_size) {
    uintptr_t p, a, modulo, padding, needed_space;

    assert(is_power_of_two(alignment));

    p = ptr;
    a = alignment;
    modulo = p & (a-1); // (p % a) as it assumes alignment is a power of two

    padding = 0;
    needed_space = 0;

    if (modulo != 0) { // Same logic as 'align_forward'
        padding = a - modulo;
    }

    needed_space = (uintptr_t)header_size;

    if (padding < needed_space) {
        needed_space -= padding;

        if ((needed_space & (a-1)) != 0) {
            padding += a * (1+(needed_space/a));
        } else {
            padding += a * (needed_space/a);
        }
    }

    return (size_t)padding;
}

inline FreeBlock* get_first_free_block(const shmem_meta& meta, void* base) {
    return reinterpret_cast<FreeBlock*>(reinterpret_cast<uint8_t*>(base) + meta.first_free);
}

inline FreeBlock* get_free_block(void* base, size_t offset) {
    return reinterpret_cast<FreeBlock*>(reinterpret_cast<uint8_t*>(base) + offset);
}


inline size_t get_offset_from_ptr(void* base, void* ptr) {
    return reinterpret_cast<uint8_t*>(ptr) - reinterpret_cast<uint8_t*>(base);
}

inline void free_block_insert(shmem_meta& metadata, void* base, size_t prev_node, FreeBlock *new_node) {
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

inline void free_block_remove(shmem_meta& metadata, void* base, size_t prev_node, FreeBlock *node) {
    if (prev_node == -1) { // This is the first element in the list
        metadata.first_free = node->next_offset;
    } else {
        FreeBlock* prev_block = get_free_block(base, prev_node);
        prev_block->next_offset = node->next_offset;
    }
}

inline void free_coalescence(shmem_meta& metadata, void* base, size_t prev_node_off, FreeBlock *free_node) {
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

inline size_t find_tail_block(shmem_meta& metadata, void* base) {
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

inline size_t find_free_block(shmem_meta& metadata, void* base, size_t size, size_t alignment, size_t& padding, size_t& prev) {
    prev = -1;
    padding = 0;
    size_t current = metadata.first_free;

    // Traverse the free list to find a block with enough space
    while (current != -1) {
        auto* currentblock = get_free_block(base, current);
        padding = calc_padding_with_header((uintptr_t)currentblock, (uintptr_t)alignment, sizeof(AllocationHeader));
        VALIDATE_FREE_BLOCK(currentblock);
        if (currentblock->size >= size + padding) {
            return current;
        }

        prev = current;
        current = currentblock->next_offset;
    }
    return -1;
}
//! @endcond