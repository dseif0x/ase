/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <tuple>
#include <mutex>
#include <vector>
#include <thread>
#include <algorithm>
#include <shared_mutex>

/**
 * @brief A wrapper around a mutex that provides shared lock functionality.
 *
 * This template class wraps a mutex and exposes the shared lock functionality
 * by providing the `lock_shared()`, `unlock_shared()`, and `try_lock_shared()` methods.
 * It ensures that the underlying mutex is locked and unlocked in a shared manner.
 *
 * @tparam Mutex The type of the mutex to wrap (e.g., `std::shared_mutex`).
 */
template <typename Mutex>
class shared_wrapped_mutex {
    Mutex& mtx;
public:
    shared_wrapped_mutex(Mutex& mtx) : mtx{mtx} {}
    shared_wrapped_mutex(const shared_wrapped_mutex&) = delete;
    shared_wrapped_mutex(shared_wrapped_mutex&&) = delete;
    void lock() {
        mtx.lock_shared();
    }

    void unlock() {
        mtx.unlock_shared();
    }

    bool try_lock() {
        return mtx.try_lock_shared();
    }
};