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
        if (val->type == ValueType::INT) {
            std::cout << "Value found: " << get<int>(val.value().data) << std::endl;
        } else if (val->type == ValueType::STRING) {
            std::cout << "Value found: " << get<std::string>(val.value().data) << std::endl;
        } else {
            std::cout << "Value found, but type not handeled" << std::endl;
        }
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
    //int pid = fork(); // FOOOOORRRRRKKKKK ALSO WORKS
    int pid = 1;
    stopwatch s;
    if (pid == 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    DB db;
    get_and_print(db, "test");
    get_and_print(db, "test1");
    get_and_print(db, "test2");

    for (int i : std::views::iota(0, 1024 * 1024)) {

        { // Integer
            CopiedValue newval{};
            newval.type = ValueType::INT;
            newval.data.emplace<int>(42);
            db.set("test", newval);
            newval.data.emplace<int>(43);
            db.set("test2", newval);
        }

        { // String
            CopiedValue newval2{};
            newval2.type = ValueType::STRING;
            newval2.data.emplace<std::string>("Hello World!");
            db.set("test3", newval2);
        }

        { // Boolean
            CopiedValue newval3{};
            newval3.type = ValueType::BOOLEAN;
            newval3.data.emplace<bool>(true);
            db.set("test4", newval3);
        }

        { // Double
            CopiedValue newval4{};
            newval4.type = ValueType::DOUBLE;
            newval4.data.emplace<double>(1.2);
            db.set("test5", newval4);
        }

        { // Array
            CopiedValue arrval1{};
            arrval1.type = ValueType::INT;
            arrval1.data.emplace<int>(13);
            CopiedValue arrval2{};
            arrval2.type = ValueType::INT;
            arrval2.data.emplace<int>(12);

            CopiedArrayType new_array{};
            new_array.size = 2;
            new_array.array_value = new CopiedValue[2]{
                std::move(arrval1), std::move(arrval2)
            };

            CopiedValue newval5{};
            newval5.type = ValueType::ARRAY;
            newval5.data.emplace<CopiedArrayType>(new_array);
            db.set("test6", newval5);
        }

        { // Pair
            auto pair_val = std::make_unique<CopiedValue>();
            pair_val->type = ValueType::INT;
            pair_val->data.emplace<int>(182);

            auto pair_val2 = std::make_unique<CopiedValue>();
            pair_val2->type = ValueType::DOUBLE;
            pair_val2->data.emplace<double>(1.7);

            std::pair<std::unique_ptr<CopiedValue>, std::unique_ptr<CopiedValue>> pair;
            pair.first = std::move(pair_val);
            pair.second = std::move(pair_val2);

            CopiedValue newval6{};
            newval6.type = ValueType::PAIR;
            newval6.data.emplace<CopiedValue::copied_pair_t>(std::move(pair));
            db.set("test7", newval6);
        }

        { // Map

            auto map_val = std::make_unique<CopiedValue>();
            map_val->type = ValueType::INT;
            map_val->data.emplace<int>(1222);

            auto map_val2 = std::make_unique<CopiedValue>();
            map_val2->type = ValueType::DOUBLE;
            map_val2->data.emplace<double>(1.2);

            std::unordered_map<std::string, std::unique_ptr<CopiedValue>> map;
            map.emplace("key1", std::move(map_val));
            map.emplace("key2", std::move(map_val2));

            CopiedValue newval6{};
            newval6.type = ValueType::MAP;
            newval6.data.emplace<CopiedValue::copied_map_t>(std::move(map));
            db.set("test8", newval6);
        }

        get_and_print(db, "test");
        get_and_print(db, "test2");
        get_and_print(db, "test3");
        get_and_print(db, "test4");
        get_and_print(db, "test5");
        get_and_print(db, "test6");
        get_and_print(db, "test7");
        get_and_print(db, "test8");
    }
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(s.peek()).count() << "ms" << std::endl;
    return 0;
}
