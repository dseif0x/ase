/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include "types.h"

class shmem;

/**
 * @onlyInAtomicAccess
 * @brief A pointer-like class that stores an offset relative to another pointer.
 *
 * This template class represents a pointer with an offset relative to the current object.
 * It provides methods for accessing the data and manipulating the offset, but must be used
 * within an atomic access block to ensure thread safety.
 *
 * @tparam T The type of the object pointed to by the `relative_offset_ptr`.
 */
template <typename T>
class relative_offset_ptr {
private:
    offset_t offset;

    void set(T* ptr) {
        // Calculate offset relative to 'this' pointer
        offset = reinterpret_cast<std::uintptr_t>(ptr) - reinterpret_cast<std::uintptr_t>(this);
    }
public:
    relative_offset_ptr(const relative_offset_ptr<T>& other): relative_offset_ptr{other.get()} {}
    relative_offset_ptr(T* other) { set(other); };
    relative_offset_ptr() : offset{0} {};

    relative_offset_ptr& operator=(const relative_offset_ptr<T>& other) {
        set(other.get());
        return *this;
    }

    relative_offset_ptr& operator=(relative_offset_ptr<T>&& other) {
        set(other.get());
        return *this;
    }

    T& operator*() const { return *get(); } // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
    T* operator->() const { return get(); } // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK

    T* get() const { // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
        return reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(this) + offset);
    }

    relative_offset_ptr& operator=(T* ptr) {
        set(ptr);
        return *this;
    }
};