/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <ranges>
#include <mutex>
#include <thread>
#include <unistd.h>

#include "shmem.h"
#include "stopwatch.h"
#include "db.h"

int main() {
    DB db;
    if (db.get("test").has_value()) {
        std::cout << "Value found" << std::endl;
    } else {
        std::cout << "Value not found" << std::endl;
    }
    return 0;
}
