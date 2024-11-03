/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <sys/mman.h>
#include <pthread.h>

class shrwlock {
public:
    shrwlock();
    ~shrwlock();
    shrwlock(const shrwlock&) = delete;
    shrwlock& operator=(const shrwlock&) = delete;
    shrwlock(shrwlock&&) = delete;
    shrwlock& operator=(shrwlock&&) = delete;

    void lock();
    void unlock();
    bool try_lock();

    void lock_shared();
    void unlock_shared();
    bool try_lock_shared();

private:
#ifdef _WIN32
    LONG counter;
#else
    pthread_rwlock_t lck;
#endif
};