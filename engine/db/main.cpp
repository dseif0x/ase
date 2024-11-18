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

/**
 * @brief Retrieves a value from the database by key and prints it.
 *
 * This function attempts to retrieve a value associated with the given key from
 * the database. If the value is found, it prints the integer value to the standard
 * output. If not, it prints a "Value not found" message.
 *
 * @param db A reference to the `DB` object from which the value is retrieved.
 * @param key A pointer to a C-string representing the key of the value to retrieve.
 */
void get_and_print(DB& db, const char* key) {
    std::optional<CopiedValue> val = db.get(key);
    if (val.has_value()) {
        std::cout << "Value found: " << val.value().int_value << std::endl;
    } else {
        std::cout << "Value not found" << std::endl;
    }
}

/**
 * @brief Simply tests a few functionalities of the DB
 */
int main() {
    {
        DB db;
        db.destroy();
    }
    int pid = fork(); // FOOOOORRRRRKKKKK ALSO WORKS
    stopwatch s;
    if (pid == 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    DB db;
    if (db.get("test").has_value()) {
        std::cout << "Value found" << std::endl;
    } else {
        std::cout << "Value not found" << std::endl;
    }

    for (int i : std::views::iota(0, 1024 * 1024)) {

        CopiedValue newval{};
        newval.type = ValueType::INT;
        newval.int_value = 42;
        db.set("test", newval);
        newval.int_value = 43;
        db.set("test2", newval);


        get_and_print(db, "test");
        get_and_print(db, "test2");
    }
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(s.peek()).count() << "ms" << std::endl;
    return 0;
}
