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
    /**
     * @brief Constructs a relative_offset_ptr by copying another relative_offset_ptr.
     *
     * @param other The other relative_offset_ptr.
     */
    relative_offset_ptr(const relative_offset_ptr<T>& other): relative_offset_ptr{other.get()} {}
    /**
     * @brief Constructs an offset_ptr with a specific offset value.
     *
     * @param offset The offset value to initialize the offset_ptr.
     */
    relative_offset_ptr(T* other) { set(other); };
    /**
     * @brief Default constructor for offset_ptr.
     *
     * Initializes the offset_ptr with a default invalid offset value (0).
     */
    relative_offset_ptr() : offset{0} {};

    /**
     * @brief Assigns the value of another relative_offset_ptr to this one.
     *
     * Copies the value of the pointer stored in another relative_offset_ptr and assigns it to this object.
     *
     * @param other The relative_offset_ptr to copy the value from.
     * @return A reference to this relative_offset_ptr.
     */
    relative_offset_ptr& operator=(const relative_offset_ptr<T>& other) {
        set(other.get());
        return *this;
    }

    /**
     * @brief Assigns a raw pointer to this relative_offset_ptr.
     *
     * Sets the value of the relative_offset_ptr to the address of the given raw pointer.
     *
     * @param ptr The raw pointer to assign.
     * @return A reference to this relative_offset_ptr.
     */
    relative_offset_ptr& operator=(T* ptr) {
        set(ptr);
        return *this;
    }

    /**
     * @brief Move-assigns the value of another relative_offset_ptr to this one.
     *
     * Transfers the value of the pointer stored in another relative_offset_ptr to this object.
     *
     * @param other The relative_offset_ptr to move the value from.
     * @return A reference to this relative_offset_ptr.
     */
    relative_offset_ptr& operator=(relative_offset_ptr<T>&& other) {
        set(other.get());
        return *this;
    }

    /**
     * Resolve to a reference to contained Object.
     *
     * @onlyInAtomicAccess
     * @return A reference to the contained Object
     */
    T& operator*() const { return *get(); }
    /**
     * Resolve to a pointer to contained Object.
     *
     * @onlyInAtomicAccess
     * @return A pointer to the contained Object
     */
    T* operator->() const { return get(); }

    /**
     * Resolve to a pointer to contained Object.
     *
     * @onlyInAtomicAccess
     * @return A pointer to the contained Object
     */
    T* get() const {
        return reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(this) + offset);
    }

};