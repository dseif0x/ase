/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <cassert>
#include "shsem.h"

shsem::shsem(uint32_t initial) {
    assert(sem_init(&lck, 1, initial) == 0 && "Failed to initialize semaphore");
}

shsem::~shsem() {
    sem_close(&lck);
}

void shsem::acquire() {
    sem_wait(&lck);
}

void shsem::release() {
    sem_post(&lck);
}
