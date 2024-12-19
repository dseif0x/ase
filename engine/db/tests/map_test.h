/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <gtest/gtest.h>
#include "db.h"

class MapTest : public ::testing::Test {
public:
    void SetUp() override {
        shmem::destroy_data();
        auto init_db = [](shmem& mem) {
            mem.construct<db_key_map>(mem);
        };
        mem = new shmem(init_db);
    }

    void TearDown() override {
        delete mem;
    }


    offset_ptr<db_key_map> get_entrypoint() const {
        return {mem->get_management_offset()};
    }


    shmem* mem{nullptr};
};


TEST_F(MapTest, InsertOne) {
    auto entrypoint = get_entrypoint();

    CopiedValue new_value{};
    new_value.type = ValueType::INT;
    new_value.data = 42;

    db_key_map::set(entrypoint, *mem, "test", Value::create_value(*mem, new_value));

    entrypoint.atomic_access(*mem, [&](db_key_map* map) {
        map->transaction([&](locked_db_map* locked_map) {
            ASSERT_TRUE(locked_map->has("test"));
            ASSERT_EQ(locked_map->get("test")->type, ValueType::INT);
            ASSERT_EQ(std::get<int>(locked_map->get("test")->data), 42);
        });
    });
}

// TODO Fix map. If we increase this (or reduce the alloc size in resize) these tests may not pass (possible hash collision not handled correctly)!
constexpr int ITERATIONS = 32;

TEST_F(MapTest, InsertMany) {
    auto get_name_from_int = [](int i) {
        return (std::string("test") + std::to_string(i));
    };
    auto entrypoint = get_entrypoint();

    for (int i = 0; i < ITERATIONS; ++i) {
        CopiedValue new_value{};
        new_value.type = ValueType::INT;
        new_value.data = i;

        db_key_map::set(entrypoint, *mem, get_name_from_int(i).c_str(), Value::create_value(*mem, new_value));
    }

    for (int i = 0; i < ITERATIONS; ++i) {
        entrypoint.atomic_access(*mem, [&](db_key_map *map) {
            map->transaction([&](locked_db_map *locked_map) {
                ASSERT_TRUE(locked_map->has(get_name_from_int(i).c_str()));
                ASSERT_EQ(locked_map->get(get_name_from_int(i).c_str())->type, ValueType::INT);
                ASSERT_EQ(std::get<int>(locked_map->get(get_name_from_int(i).c_str())->data), i);
            });
        });
    }
}

TEST_F(MapTest, InsertAndDeleteMany) {
    auto get_name_from_int = [](int i) {
        return (std::string("test") + std::to_string(i));
    };
    auto entrypoint = get_entrypoint();

    for (int i = 0; i < ITERATIONS; ++i) {
        CopiedValue new_value{};
        new_value.type = ValueType::INT;
        new_value.data = i;

        db_key_map::set(entrypoint, *mem, get_name_from_int(i).c_str(), Value::create_value(*mem, new_value));
    }

    for (int i = 0; i < ITERATIONS; ++i) {
        db_key_map::remove(entrypoint, *mem, get_name_from_int(i).c_str());
    }

    entrypoint.atomic_access(*mem, [&](db_key_map *map) {
        map->transaction([&](locked_db_map *locked_map) {
            ASSERT_EQ(locked_map->keys().size(), 0);
            for (int i = 0; i < ITERATIONS; ++i) {
                ASSERT_FALSE(locked_map->has(get_name_from_int(i).c_str()));
            }
        });
    });
}