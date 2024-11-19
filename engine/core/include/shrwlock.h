/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <sys/mman.h>
#include <pthread.h>

/**
 * @brief A wrapper for a shared read-write lock.
 *
 * This class provides read-write locking functionality, allowing multiple threads
 * to acquire a shared lock for reading or an exclusive lock for writing. It is
 * designed for thread synchronization, providing methods to lock, unlock, and
 * try-lock in both shared and exclusive modes.
 */
class shrwlock {
public:
    shrwlock();
    ~shrwlock();
    /*shrwlock(const shrwlock&) = delete;
    shrwlock& operator=(const shrwlock&) = delete;
    shrwlock(shrwlock&&) = delete;
    shrwlock& operator=(shrwlock&&) = delete;*/

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