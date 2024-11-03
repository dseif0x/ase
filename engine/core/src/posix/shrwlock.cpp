/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "shrwlock.h"
#include <iostream>

shrwlock::shrwlock() {
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_rwlock_init(&lck, &attr);
    pthread_rwlockattr_destroy(&attr);

}

shrwlock::~shrwlock() {
    pthread_rwlock_destroy(&lck);
}

void shrwlock::lock() {
    pthread_rwlock_wrlock(&lck);
}

void shrwlock::unlock() {
    pthread_rwlock_unlock(&lck);
}

bool shrwlock::try_lock() {
    return pthread_rwlock_trywrlock(&lck) == 0;
}

void shrwlock::lock_shared() {
    pthread_rwlock_rdlock(&lck);
}

void shrwlock::unlock_shared() {
    pthread_rwlock_unlock(&lck);
}

bool shrwlock::try_lock_shared() {
    return pthread_rwlock_tryrdlock(&lck) == 0;
}