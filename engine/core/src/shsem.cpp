/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <thread>
#include "shsem.h"

shsem::shsem(uint32_t initial) : counter(initial), lock(false) {}

void shsem::acquire() {
    while (true) {
        // Spinlock to ensure mutual exclusion
        while (lock.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        if (counter > 0) {
            --counter;
            lock.clear(std::memory_order_release);
            return;
        }

        lock.clear(std::memory_order_release);

        // Yield to avoid busy-waiting (optional: add sleep)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void shsem::release() {
    // Spinlock to ensure mutual exclusion
    while (lock.test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    ++counter;

    lock.clear(std::memory_order_release);
}

int shsem::get() {
    return counter.load(std::memory_order_relaxed);
}
