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
    shrwlock meta_mutex;
};

/**
 * @brief Manages shared memory segments with metadata and data handling.
 *
 * The `shmem` class provides an interface for creating, accessing, and managing
 * shared memory segments. It handles metadata and data segments, allowing for
 * atomic access, resizing, and allocation within shared memory.
 */
class shmem {
    std::size_t local_version{1}; //!< This is the version this process is currently working with on access we check if the version in the metadata segment has changed
    std::size_t local_size{0};

    shmem_meta* metadata{nullptr}; //!< This is one shared memory segment that is fixed in size and contains the metadata
    void* base{nullptr}; //!< This is the second shared memory segment that is variable in size and contains the actual data

    void attach(std::function<void(shmem&)> on_create);
    void close();

    void resize(offset_t min_size); //!< This function is used to resize the base segment if the required size is larger than the current size
    void atomic_access(offset_t offset, std::function<void(void*)>&& lambda, bool locks_held = false); //!< This function is used to access the data segment in an atomic way
    offset_t allocate(std::size_t size); //!< This function is used to allocate a new block of memory in the base segment
    void deallocate(offset_t ptr); //!< This function is used to deallocate a block of memory in the base segment
public:
    shmem();
    shmem(std::function<void(shmem&)> on_create);
    ~shmem();
    void destroy();

    shrwlock& get_meta_mutex();

    offset_t get_management_offset() const;

    //! @neverInAtomicAccess
    template <typename T, typename... Args>
    offset_ptr<T> construct(Args&&... args) { // Using this inside an atomic_access lambda will cause a deadlock
        auto storage = allocate(sizeof(T));
        offset_ptr<T> ptr{storage};
        atomic_access(storage, [&args...](void *ptr) {
            new (ptr) T(std::forward<Args>(args)...);
        });
        return ptr;
    }

    //! @neverInAtomicAccess
    template <typename T>
    offset_ptr<T> construct_array(size_t count) { // Using this inside an atomic_access lambda will cause a deadlock
        auto storage = allocate(sizeof(T) * count);
        offset_ptr<T> ptr{storage};
        atomic_access(storage, [count](void *ptr) {
            for (size_t i = 0; i < count; i++) {
                new (reinterpret_cast<uint8_t*>(ptr) + i * sizeof(T)) T();
            }
        });
        return ptr;
    }

    //! @neverInAtomicAccess
    template<typename T>
    void destroy(offset_ptr<T> ptr) { // Using this inside an atomic_access lambda will cause a deadlock
        atomic_access(ptr.offset, [](void *ptr) {
            reinterpret_cast<T*>(ptr)->~T();
        });
        deallocate(ptr.offset);
    }

    //! @neverInAtomicAccess
    template<typename T>
    void destroy_array(offset_ptr<T> ptr) { // Using this inside an atomic_access lambda will cause a deadlock
        // TODO get count and call destructors
        atomic_access(ptr.offset, [](void *ptr) {
            reinterpret_cast<T*>(ptr)->~T();
        });
        deallocate(ptr.offset);
    }

    template <typename T>
    friend class offset_ptr;
};

template <typename T>
template <typename Lambda>
void offset_ptr<T>::atomic_access(shmem& mem, Lambda&& value, bool locks_held) {
    static_assert(std::is_invocable_v<Lambda, T*>, "Lambda must be invocable with T*");
    mem.atomic_access(this->offset, [value](void *ptr) {
        value(reinterpret_cast<T *>(ptr));
    }, locks_held);
}

template<typename T>
T *offset_ptr<T>::get(shmem &mem) const {
    return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(mem.base) + offset);
}