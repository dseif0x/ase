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
template <typename T>
class offset_ptr {
private:
    offset_t offset;
public:
    template <typename U>
    offset_ptr(const offset_ptr<U>& other): offset{other.offset} {}
    offset_ptr(offset_t offset) : offset{offset} {};

    template <typename Lambda>
    void atomic_access(shmem& mem, Lambda&& lambda, bool locks_held = false); // Will ensure that the passed lambda expression is executed atomically (no concurrent resizing of the shared memory)

    template <typename U>
    friend class offset_ptr;
};