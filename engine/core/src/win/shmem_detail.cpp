/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "shmem.h"
#include "shmem_util.h"

#include <shared_mutex>
#include <stdexcept>

void shmem::attach(std::function<void(shmem&)> on_create) {
    // TODO
    throw std::runtime_error("Not implemented");
}

void shmem::close() {
    // TODO
    throw std::runtime_error("Not implemented");
}

void shmem::destroy() {
    // TODO
    throw std::runtime_error("Not implemented");
}

void shmem::destroy_data() {
    // TODO
    throw std::runtime_error("Not implemented");
}

void shmem::ensure_enough_space(std::size_t required_memory, std::size_t required_alignment) {
    // TODO
    throw std::runtime_error("Not implemented");
}

void shmem::ensure_mounted() {
    // TODO
    throw std::runtime_error("Not implemented");
}
