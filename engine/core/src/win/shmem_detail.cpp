/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "shmem.h"
#include "shmem_util.h"
#include <windows.h>
#include <stdexcept>
#include <functional>
#include <cstring>
#include <memory>

void shmem::attach(std::function<void(shmem&)> on_create) {
    HANDLE meta_handle = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(shmem_meta), "shmem_meta");
    if (!meta_handle) {
        throw std::runtime_error("Failed to create or open metadata shared memory segment");
    }

    this->meta_handle = meta_handle;

    bool created = (GetLastError() != ERROR_ALREADY_EXISTS);
    metadata = static_cast<shmem_meta*>(MapViewOfFile(meta_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shmem_meta)));
    if (!metadata) {
        throw std::runtime_error("Failed to map view of metadata shared memory");
    }
    if (created) {
        new (metadata) shmem_meta();
    }

    const std::string handle_name = this->get_handle_name();

    HANDLE data_handle = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, metadata->alloc_size, handle_name.c_str());
    if (!data_handle) {
        close_meta_handle();
        throw std::runtime_error("Failed to create or open data shared memory segment");
    }

    this->data_handle = data_handle;

    created = (GetLastError() != ERROR_ALREADY_EXISTS);
    base = MapViewOfFile(data_handle, FILE_MAP_ALL_ACCESS, 0, 0, metadata->alloc_size);
    if (!base) {
        close_meta_handle();
        close_data_handle();
        throw std::runtime_error("Failed to map view of data shared memory");
    }
    local_size = metadata->alloc_size;

    if (created) {
        FreeBlock* freeList = get_first_free_block(*metadata, base);
#ifdef MEM_DEBUG
        new (freeList) FreeBlock();
#endif
        freeList->size = metadata->alloc_size;
        freeList->next_offset = -1;
        on_create(*this);
    }
}

void shmem::close() {
    if (base) {
        UnmapViewOfFile(base);
        base = nullptr;
        close_data_handle();
    }
    if (metadata) {
        UnmapViewOfFile(metadata);
        metadata = nullptr;
        close_meta_handle();
    }
}

void shmem::close_meta_handle() {
    if (this->meta_handle) {
        CloseHandle(this->meta_handle);
        this->meta_handle = nullptr;
    }
}

void shmem::close_data_handle() {
    if (this->data_handle) {
        CloseHandle(this->data_handle);
        this->data_handle = nullptr;
    }
}

void shmem::destroy() {
    if (base) {
        UnmapViewOfFile(base);
        base = nullptr;
    }
    if (metadata) {
        UnmapViewOfFile(metadata);
        metadata = nullptr;
    }
    close_meta_handle();
    close_data_handle();
}

void shmem::destroy_data() {
    // Should not be needed since windows cleans them up automatically
    // In fact this will fail as long as a process holds an open handle
    DeleteFile("shmem_data");
}

void shmem::ensure_enough_space(std::size_t required_memory, std::size_t required_alignment) {
    if (can_fit(required_memory, required_alignment)) {
        return;
    }

    std::size_t prev_size = metadata->alloc_size;
    std::unique_ptr<uint8_t[]> tmp(new uint8_t[prev_size]);
    memcpy(tmp.get(), base, prev_size);

    UnmapViewOfFile(base);
    close_data_handle();

    metadata->alloc_size *= 2;
    while (metadata->alloc_size < prev_size + required_memory) {
        metadata->alloc_size *= 2;
    }
    ++metadata->version;

    const std::string handle_name = get_handle_name();

    HANDLE data_handle = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, metadata->alloc_size, handle_name.c_str());
    if (!data_handle) {
        close_meta_handle();
        throw std::runtime_error("Failed to create data shared memory segment after resize");
    }

    this->data_handle = data_handle;

    base = MapViewOfFile(data_handle, FILE_MAP_ALL_ACCESS, 0, 0, metadata->alloc_size);
    if (!base) {
        close_meta_handle();
        close_data_handle();
        throw std::runtime_error("Failed to map view of data shared memory after resize");
    }
    memcpy(base, tmp.get(), prev_size);

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
    if (metadata->version > local_version) {
        UnmapViewOfFile(base);
        close_data_handle();
        const std::string handle_name = get_handle_name();
        HANDLE data_handle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, handle_name.c_str());
        if (!data_handle) {
            close_meta_handle();
            throw std::runtime_error("Failed to open data shared memory segment");
        }
        this->data_handle = data_handle;
        base = MapViewOfFile(data_handle, FILE_MAP_ALL_ACCESS, 0, 0, metadata->alloc_size);
        if (!base) {
            close_meta_handle();
            close_data_handle();
            throw std::runtime_error("Failed to map view of data shared memory after version change");
        }

        local_version = metadata->version;
        local_size = metadata->alloc_size;
    } else if (metadata->version < local_version) {
        throw std::runtime_error("Version mismatch");
    }
}
