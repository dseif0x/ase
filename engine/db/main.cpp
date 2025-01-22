/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <ranges>
#include <mutex>
#include <thread>

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
void get_and_print(DB &db, const char *key) {
    std::optional<CopiedValue> val = db.get(key);
    if (val.has_value()) {
        if (val->type == ValueType::INT) {
            std::cout << "Value found for key " << key << ": " << get<int>(val.value().data) << std::endl;
        } else if (val->type == ValueType::STRING) {
            std::cout << "Value found for key " << key << ": " << get<std::string>(val.value().data) << std::endl;
        } else if (val->type == ValueType::DOUBLE) {
            std::cout << "Value found for key " << key << ": " << get<double>(val.value().data) << std::endl;
        } else if (val->type == ValueType::REFERENCE) {
            std::cout << "Value found for key " << key << ": " << get<std::string>(val.value().data) << std::endl;
        } else {
            std::cout << "Value found for key " << key << ", " << "but type not handeled" << std::endl;
        }
    } else {
        std::cout << "Value not found for key " << key << std::endl;
    }
}

void list_and_print(DB &db, const char *key) {
    DB::list_t val = db.list(key);
    if (val.has_value()) {
        std::cout << "Listing values for " << key << std::endl;
        for (auto &entry: val.value()) {
            if (entry.second.first == ValueType::INT) {
                std::cout << "--- Value found for key " << entry.first << ": " << get<int>(entry.second.second) <<
                        std::endl;
            } else if (entry.second.first == ValueType::STRING) {
                std::cout << "--- Value found for key " << entry.first << ": " << get<std::string>(entry.second.second)
                        << std::endl;
            } else if (entry.second.first == ValueType::DOUBLE) {
                std::cout << "--- Value found for key " << entry.first << ": " << get<double>(entry.second.second) <<
                        std::endl;
            } else {
                std::cout << "--- Value found for key " << entry.first << ": -" << std::endl;
            }
        }
    } else {
        std::cout << "Value not found for key " << key << std::endl;
    }
}

/**
 * @brief Retrieves a value from the database by key and prints it.
 *
 * This function attempts to retrieve a value associated with the given reference from
 * the database. If the value is found, it prints the integer value to the standard
 * output. If not, it prints a "Value not found" message.
 *
 * @param db A reference to the `DB` object from which the value is retrieved.
 * @param key A pointer to a C-string representing the key of the reference to retrieve.
 */
void follow_and_print(DB &db, const char *key) {
    std::optional<CopiedValue> val = db.follow(key);
    if (val.has_value()) {
        if (val->type == ValueType::INT) {
            std::cout << "Reference followed for key " << key << ": " << get<int>(val.value().data) << std::endl;
        } else if (val->type == ValueType::STRING) {
            std::cout << "Reference followed for key " << key << ": " << get<std::string>(val.value().data) <<
                    std::endl;
        } else if (val->type == ValueType::DOUBLE) {
            std::cout << "Reference followed for key " << key << ": " << get<double>(val.value().data) << std::endl;
        } else {
            std::cout << "Reference followed for key " << key << ", " << "but type not handeled" << std::endl;
        }
    } else {
        std::cout << "Value not found for key " << key << std::endl;
    }
}

/**
 * @brief Simply tests a few functionalities of the DB
 */
int main(int argc, char* argv[]) {
    if (argc <= 1)
        DB::destroy();
    //int pid = fork(); // FOOOOORRRRRKKKKK ALSO WORKS
    int pid = 1;
    stopwatch s;
    if (pid != 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    DB db;
    {
        CopiedValue newval{};
        newval.type = ValueType::INT;
        newval.data.emplace<int>(42);
        db.set("test", std::move(newval));
    }
    get_and_print(db, "test");
    get_and_print(db, "test1");
    get_and_print(db, "test2");

    bool doInteger = true;
    bool doString = true;
    bool doBoolean = true;
    bool doDouble = true;
    bool doArray = true;
    bool doPair = false;
    bool doMap = false;
    bool doReference = false;
    bool doDelete = false;
    bool doSum = false;

    for (int i: std::views::iota(0, 1024 * 1024)) {
        if (doInteger) {
            // Integer
            CopiedValue newval{};
            newval.type = ValueType::INT;
            newval.data.emplace<int>(42);
            db.set("test", std::move(newval), true);

            CopiedValue newval2{};
            newval2.type = ValueType::INT;
            newval2.data.emplace<int>(43);
            db.set("test2", std::move(newval2), true);
        }

        if (doString) {
            // String
            CopiedValue newval2{};
            newval2.type = ValueType::STRING;
            newval2.data.emplace<std::string>("Hello World!");
            db.set("test3", std::move(newval2), true);
        }

        if (doBoolean) {
            // Boolean
            CopiedValue newval3{};
            newval3.type = ValueType::BOOLEAN;
            newval3.data.emplace<bool>(true);
            db.set("test4", std::move(newval3), true);
        }

        if (doDouble) {
            // Double
            CopiedValue newval4{};
            newval4.type = ValueType::DOUBLE;
            newval4.data.emplace<double>(1.2);
            db.set("test5", std::move(newval4), true);
        }

        if (doArray) {
            // Array
            CopiedValue newval5{};
            newval5.type = ValueType::ARRAY;
            auto &vector = newval5.data.emplace<CopiedValue::copied_array_t>();

            CopiedValue arrval1{};
            arrval1.type = ValueType::STRING;
            arrval1.data.emplace<std::string>("test");
            CopiedValue arrval2{};
            arrval2.type = ValueType::INT;
            arrval2.data.emplace<int>(12);

            vector.emplace_back(std::move(arrval1));
            vector.emplace_back(std::move(arrval2));

            db.set("test6", std::move(newval5), true);


            CopiedValue newval6{};
            newval6.type = ValueType::ARRAY;
            newval6.data.emplace<CopiedValue::copied_array_t>();

            db.set("testEmptyArray", std::move(newval6), true);
        }

        if (doPair) {
            // Pair
            auto pair_val = std::make_unique<CopiedValue>();
            pair_val->type = ValueType::INT;
            pair_val->data.emplace<int>(182);

            auto pair_val2 = std::make_unique<CopiedValue>();
            pair_val2->type = ValueType::DOUBLE;
            pair_val2->data.emplace<double>(1.7);

            std::pair<std::unique_ptr<CopiedValue>, std::unique_ptr<CopiedValue> > pair;
            pair.first = std::move(pair_val);
            pair.second = std::move(pair_val2);

            CopiedValue newval6{};
            newval6.type = ValueType::PAIR;
            newval6.data.emplace<CopiedValue::copied_pair_t>(std::move(pair));
            db.set("test7", std::move(newval6), true);
        }

        if (doMap) {
            // Map
            CopiedValue map_val{};
            map_val.type = ValueType::INT;
            map_val.data.emplace<int>(1222);

            CopiedValue map_val2{};
            map_val2.type = ValueType::DOUBLE;
            map_val2.data.emplace<double>(1.2);

            std::unordered_map<std::string, std::unique_ptr<CopiedValue> > map;

            CopiedValue newval6{};
            newval6.type = ValueType::MAP;
            newval6.data.emplace<CopiedValue::copied_map_t>(std::move(map));

            db.set("test8", std::move(newval6), true);

            db.set("test8.key1", std::move(map_val), true);
            db.set("test8.key2", std::move(map_val2), true);
        }

        if (doReference) {
            // Reference
            CopiedValue newval2{};
            newval2.type = ValueType::REFERENCE;
            newval2.data.emplace<std::string>("test3");
            db.set("test9", std::move(newval2), true);
        }

        if (doDelete) {
            // Delete
            CopiedValue newval{};
            newval.type = ValueType::INT;
            newval.data.emplace<int>(42);
            db.set("test10", std::move(newval), true);

            db.remove("test10");

            CopiedValue newval2{};
            CopiedValue newval3{};

            auto pair_val = std::make_unique<CopiedValue>();
            pair_val->type = ValueType::INT;
            pair_val->data.emplace<int>(182);

            auto pair_val2 = std::make_unique<CopiedValue>();
            pair_val2->type = ValueType::DOUBLE;
            pair_val2->data.emplace<double>(1.7);

            std::pair<std::unique_ptr<CopiedValue>, std::unique_ptr<CopiedValue> > pair;
            pair.first = std::move(pair_val);
            pair.second = std::move(pair_val2);

            newval2.type = ValueType::PAIR;
            newval2.data.emplace<CopiedValue::copied_pair_t>(std::move(pair));
            newval3.type = ValueType::ARRAY;
            auto &vector = newval3.data.emplace<CopiedValue::copied_array_t>();

            CopiedValue arrval1{};
            arrval1.type = ValueType::INT;
            arrval1.data.emplace<int>(15);
            CopiedValue arrval2{};
            arrval2.type = ValueType::INT;
            arrval2.data.emplace<int>(17);

            vector.emplace_back(std::move(arrval1));
            vector.emplace_back(std::move(arrval2));

            db.set("delete.me.1", std::move(newval2), true);
            db.set("delete.me.2", std::move(newval3), true);

            db.remove("delete.me.1");
            db.remove("delete.me.2");
            db.remove("delete.me");
        }

        if (doSum)
        {
            CopiedValue newval{};
            newval.type = ValueType::INT;
            newval.data.emplace<int>(1);
            db.set("GraphRoot.value", std::move(newval), true);

            CopiedValue newval3{};
            newval3.type = ValueType::INT;
            newval3.data.emplace<int>(2);
            db.set("GraphRoot.left.value", std::move(newval3), true);

            CopiedValue newval4{};
            newval4.type = ValueType::INT;
            newval4.data.emplace<int>(3);
            db.set("GraphRoot.right.value", std::move(newval4), true);

            CopiedValue newval5{};
            newval5.type = ValueType::INT;
            newval5.data.emplace<int>(4);
            db.set("GraphRoot.left.left.value", std::move(newval5), true);

            std::optional<CopiedValue> res = db.aggregate<ValueType::INT>(Aggregation::AVERAGE, "GraphRoot", "value", {"left", "right"});
            if (res.has_value())
                std::cout << "Aggregated Value: " << std::get<double>(res.value().data) << std::endl;
            else
                std::cout << "Could not aggregate value" << std::endl;
        }


        CopiedValue newvalAcl{};
        newvalAcl.type = ValueType::INT;
        newvalAcl.data.emplace<int>(42);
        db.set("aclTest", std::move(newvalAcl), true);
        return 0;


        get_and_print(db, "test");
        get_and_print(db, "test2");
        get_and_print(db, "test3");
        get_and_print(db, "test4");
        get_and_print(db, "test5");
        get_and_print(db, "test6");
        get_and_print(db, "test7");
        get_and_print(db, "test8.key1");
        get_and_print(db, "test8.key2");
        list_and_print(db, "test8");
        follow_and_print(db, "test9");
        get_and_print(db, "test10");

        std::cout << "Memory in use: " << db.get_used_memory() << " bytes" << std::endl;
    }
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(s.peek()).count() << "ms" <<
            std::endl;
    return 0;
}
