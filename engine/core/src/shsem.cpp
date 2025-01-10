/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <thread>
#include "shsem.h"

shsem::shsem(uint32_t initial) : counter(initial) {}

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

void shsem::acquire(offset_ptr<shsem> sem, shmem &mem) {
    while (true) {
        // Spinlock to ensure mutual exclusion

        while (true) {
            bool not_acquired = true;
            sem.atomic_access(mem, [&not_acquired](shsem *sem) {
                not_acquired = sem->lock.test_and_set(std::memory_order_acquire);
            });
            if (!not_acquired) break;
            std::this_thread::yield();
        }

        bool locked = false;
        sem.atomic_access(mem, [&locked](shsem *sem) {
            if (sem->counter > 0) {
                --sem->counter;
                sem->lock.clear(std::memory_order_release);
                locked = true;
            } else {
                sem->lock.clear(std::memory_order_release);
            }
        });
        if (locked) return;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
