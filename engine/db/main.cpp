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

void get_and_print(DB& db, const char* key) {
    std::optional<CopiedValue> val = db.get(key);
    if (val.has_value()) {
        std::cout << "Value found: " << val.value().int_value << std::endl;
    } else {
        std::cout << "Value not found" << std::endl;
    }
}

int main() {
    {
        DB db;
        db.destroy();
    }
    DB db;
    if (db.get("test").has_value()) {
        std::cout << "Value found" << std::endl;
    } else {
        std::cout << "Value not found" << std::endl;
    }

    CopiedValue newval{};
    newval.type = ValueType::INT;
    newval.int_value = 42;
    db.set("test", newval);
    newval.int_value = 43;
    db.set("test2", newval);


    get_and_print(db, "test");
    get_and_print(db, "test2");
    return 0;
}
