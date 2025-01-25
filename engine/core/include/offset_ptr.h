/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include "types.h"
#include "relative_offset_ptr.h"

class shmem;
/**
 * @brief A pointer-like wrapper that manages offsets to memory locations within a shared memory segment.
 *
 * This template class represents a pointer to a memory location within a shared memory segment.
 * It stores an offset relative to the start of the memory region and provides methods to access
 * and manipulate data at that offset. The `offset_ptr` is designed to be used safely within
 * an atomic context to ensure that concurrent access or resizing of the shared memory does not
 * cause data corruption.
 *
 * @tparam T The type of the object pointed to by the `offset_ptr`.
 */
template <typename T>
class offset_ptr {
private:
    offset_t offset;
public:
    /**
     * @brief Constructs an offset_ptr by copying the offset from another offset_ptr.
     *
     * @tparam U The type of the other offset_ptr.
     * @param other The other offset_ptr to copy the offset from.
     */
    template <typename U>
    offset_ptr(const offset_ptr<U>& other): offset{other.offset} {}
    /**
     * @brief Constructs an offset_ptr with a specific offset value.
     *
     * @param offset The offset value to initialize the offset_ptr.
     */
    offset_ptr(offset_t offset) : offset{offset} {};
    /**
     * @brief Default constructor for offset_ptr.
     *
     * Initializes the offset_ptr with a default invalid offset value (-1).
     */
    offset_ptr() : offset{-1} {};

    /**
    * Returns the offset
    * @return the offset
    */
    offset_t get_offset() const {
        return offset;
    }
    /**
     * @onlyInAtomicAccess
     *
     * Resolves the Pointer
     * @param mem a reference to the shared memory.
     * @return A pointer to the contained object.
     */
    T* get(shmem& mem) const;
    /**
     * @onlyInAtomicAccess
     *
     * Transform to relative offset pointer
     * @param mem a reference to the shared memory.
     * @return A relative offset pointer pointing to the same object.
     */
    relative_offset_ptr<T> get_relative(shmem& mem) const;
    /**
     * @onlyInAtomicAccess
     *
     * Will ensure that the passed lambda expression is executed atomically (no concurrent resizing of the shared memory)
     * @param mem a reference to the shared memory.
     * @param lambda a function that take the contained pointer as an argument
     * @param locks_held indicates if locks are already held or need to be acquired (default false)
     */
    template <typename Lambda>
    void atomic_access(shmem& mem, Lambda&& lambda, bool locks_held = false);
    /**
     * @onlyInAtomicAccess
     *
     * Will ensure that the passed lambda expression is executed atomically (no concurrent resizing of the shared memory)
     * @param mem a reference to the shared memory.
     * @param lambda a function that take the contained pointer as an argument
     * @param locks_held indicates if locks are already held or need to be acquired (default false)
     */
    explicit operator bool() const {
        return offset >= 0;
    }
    template <typename U>
    friend class offset_ptr;
    friend class shmem;
};

template<typename T>
relative_offset_ptr<T> offset_ptr<T>::get_relative(shmem& mem) const {
    return relative_offset_ptr<T>{get(mem)};
}
