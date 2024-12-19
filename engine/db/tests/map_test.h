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


TEST_F(MapTest, InsertTest) {
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