/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "shrwlock.h"
#include <functional>
#include "offset_ptr.h"

struct shmem_meta {
    std::size_t version{1};
    offset_t alloc_size{1024};
    std::size_t used_size{0};
    // This is platform specific
    shrwlock meta_mutex;
    shrwlock data_mutex;
};

class shmem {
    std::size_t local_version{1}; // This is the version this process is currently working with on access we check if the version in the metadata segment has changed

    shmem_meta* metadata{nullptr}; // This is one shared memory segment that is fixed in size and contains the metadata
    void* base{nullptr}; // This is the second shared memory segment that is variable in size and contains the actual data

    void attach();
    void close();

    void resize(offset_t min_size); // This function is used to resize the base segment if the required size is larger than the current size
    void* access(offset_t offset); // This function is used to access a pointer that is stored as an offset
    void atomic_write(offset_t offset, std::function<void(void*)>&& lambda, bool locks_held = false); // This function is used to access the data segment in an atomic way
    void atomic_read(offset_t offset, std::function<void(const void*)>&& lambda, bool locks_held = false); // This function is used to access the data segment in an atomic way
    offset_t allocate(std::size_t size); // This function is used to allocate a new block of memory in the base segment
    void deallocate(offset_ptr<void> ptr); // This function is used to allocate a new block of memory in the base segment
public:
    shmem();
    ~shmem();
    void destroy();

    template <typename T>
    offset_ptr<T> construct() {
        auto storage = allocate(sizeof(T));
        offset_ptr<T> ptr{storage};
        new (ptr.get(*this)) T();
        return ptr;
    }

    template <typename T>
    offset_ptr<T> construct(int count) {
        auto storage = allocate(sizeof(T));
        offset_ptr<T> ptr{storage};
        new (ptr.get(*this)) T();
        return ptr;
    }

    template <typename T>
    friend class offset_ptr;
};

template <typename T>
T* offset_ptr<T>::get(shmem& mem) const {
    return reinterpret_cast<T*>(mem.access(this->offset));
}

template <typename T>
template <typename Lambda>
void offset_ptr<T>::atomic_write(shmem& mem, Lambda&& value, bool locks_held) {
    static_assert(std::is_invocable_v<Lambda, T*>, "Lambda must be invocable with T*");
    mem.atomic_write(this->offset, [value](void *ptr) {
        value(reinterpret_cast<T *>(ptr));
    }, locks_held);
}

template <typename T>
template <typename Lambda>
void offset_ptr<T>::atomic_read(shmem& mem, Lambda&& value, bool locks_held) {
    static_assert(std::is_invocable_v<Lambda, T*>, "Lambda must be invocable with T*");
    mem.atomic_read(this->offset, [value](const void *ptr) {
        value(reinterpret_cast<const T *>(ptr));
    }, locks_held);
}