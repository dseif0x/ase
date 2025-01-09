/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "shrwlock.h"
#include <stdexcept>
#include <thread>

shrwlock::shrwlock() : state(0) {
    if (!state.is_lock_free()) {
        throw std::runtime_error("Atomics are not lock free and might not work in SHMEM");
    }
}

shrwlock::~shrwlock() {
}

void shrwlock::lock() {
    while (!try_lock()) {
        std::this_thread::yield(); // Avoid busy-waiting
    }
}

void shrwlock::unlock() {
    int expected = -1;
    if (!state.compare_exchange_strong(expected, 0, std::memory_order_seq_cst, std::memory_order_seq_cst)) {
        throw std::runtime_error("Unlock called without an exclusive lock");
    }
}

bool shrwlock::try_lock() {
    int expected = 0;
    return state.compare_exchange_strong(expected, -1, std::memory_order_seq_cst, std::memory_order_seq_cst);
}

void shrwlock::lock_shared() {
    while (!try_lock_shared()) {
        std::this_thread::yield(); // Avoid busy-waiting
    }
}

void shrwlock::unlock_shared() {
    int old_val = state.fetch_sub(1, std::memory_order_release);
    if (old_val <= 0) {
        throw std::runtime_error("Unlock_shared called without a shared lock");
    }
}

bool shrwlock::try_lock_shared() {
    int current = state.load(std::memory_order_relaxed);
    while (current >= 0) {
        // Attempt transition from current -> current + 1
        // If someone else changed `state` in the meantime, the CAS fails.
        if (state.compare_exchange_weak(
            current, current + 1,
            std::memory_order_acquire,
            std::memory_order_relaxed))
        {
            // Successfully took a shared lock
            return true;
        }
        // if CAS failed but current >= 0 still, we’ll retry
        // if current changed to negative, we’ll break
    }
    // If we exit the loop, it means state < 0 => exclusive locked
    return false;
}