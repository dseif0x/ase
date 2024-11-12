/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "shrwlock.h"

shrwlock::shrwlock() {
    counter = 0;

}

shrwlock::~shrwlock() {
}

void shrwlock::lock() {
    while (InterlockedCompareExchange(&counter, -1, 0) != 0) {
        Sleep(0);  // Yield execution to reduce busy-waiting
    }
}

void shrwlock::unlock() {
    InterlockedExchange(&counter, 0);
}

bool shrwlock::try_lock() {
    return InterlockedCompareExchange(&counter, -1, 0) == 0;
}

void shrwlock::lock_shared() {
    LONG value;
    do {
        // Spin until the counter is non-negative and increment it
        value = counter;
        while (value < 0) {  // Wait if a writer holds the lock
            Sleep(0);  // Yield execution to avoid busy-waiting
            value = counter;
        }
    } while (InterlockedCompareExchange(&counter, value + 1, value) != value);
}

void shrwlock::unlock_shared() {
    InterlockedDecrement(&counter);
}

bool shrwlock::try_lock_shared() {
    LONG value = counter;
    return (value >= 0) && (InterlockedCompareExchange(&counter, value + 1, value) == value);
}