/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once
#include <fcntl.h> // For O_CREAT and O_EXCL
#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/semaphore.h>

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
    shsem(uint32_t initial);
    ~shsem();
    shsem(const shsem&) = delete;
    shsem& operator=(const shsem&) = delete;
    shsem(shsem&&) = delete;
    shsem& operator=(shsem&&) = delete;

    void acquire(); //!< acquires an semaphore
    void release(); //!< releases an semaphore

private:
#ifdef _WIN32
#error "Not implemented"
#else
    sem_t lck;
#endif
};