/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "shrwlock.h"
#include <stdexcept>
#include <thread>

shrwlock::shrwlock() : state(0) {}

shrwlock::~shrwlock() {
}

void shrwlock::lock() {
    while (!try_lock()) {
        std::this_thread::yield(); // Avoid busy-waiting
    }
}

void shrwlock::unlock() {
    if (state != -1) {
        throw std::runtime_error("Unlock called without an exclusive lock");
    }
    state = 0; // Release the lock
}

bool shrwlock::try_lock() {
    int expected = 0;
    return state.compare_exchange_strong(expected, -1, std::memory_order_acquire);
}

void shrwlock::lock_shared() {
    while (!try_lock_shared()) {
        std::this_thread::yield(); // Avoid busy-waiting
    }
}

void shrwlock::unlock_shared() {
    int current_state = state.load(std::memory_order_acquire);
    if (current_state <= 0) {
        throw std::runtime_error("Unlock_shared called without a shared lock");
    }
    state.fetch_sub(1, std::memory_order_release); // Decrement reader count
}

bool shrwlock::try_lock_shared() {
    int current_state = state.load(std::memory_order_acquire);
    if (current_state >= 0) { // No writer holding the lock
        return state.fetch_add(1, std::memory_order_acquire) >= 0;
    }
    return false;
}