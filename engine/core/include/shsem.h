/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once
#include <atomic>
#include "shmem.h"

/**
 * @brief A wrapper for a shared read-write lock.
 *
 * This class provides read-write locking functionality, allowing multiple threads
 * to acquire a shared lock for reading or an exclusive lock for writing. It is
 * designed for thread synchronization, providing methods to lock, unlock, and
 * try-lock in both shared and exclusive modes.
 */
class shsem {
public:
    /**
    * initializes the semaphore
    * @param the initial count
    */
    shsem(uint32_t initial);
    ~shsem() = default; //!< the destructor
    //! @cond
    shsem(const shsem&) = delete;
    shsem& operator=(const shsem&) = delete;
    shsem(shsem&&) = delete;
    shsem& operator=(shsem&&) = delete;
    //! @endcond

    static void acquire(offset_ptr<shsem> sem, shmem& mem); //!< acquires a semaphore
    void release(); //!< releases a semaphore
    /**
    * get the count of the semaphore
    * @return the count of the semaphore
    */
    int get();

private:
    std::atomic<int> counter;            // Semaphore counter
    mutable std::atomic_flag lock = {}; // Spinlock for mutual exclusion
};