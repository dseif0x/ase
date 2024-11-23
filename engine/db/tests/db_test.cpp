/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <db.h>
#include <db_types.h>
#include <gtest/gtest.h>


TEST(IntTest, HandlesPositiveInput) {
    // TODO This should not be needed
    {
        DB db;
        db.destroy();
    }

    DB db;
    CopiedValue newval{};
    newval.type = ValueType::INT;
    newval.data.emplace<int>(43);
    db.set("test2", newval);
    EXPECT_EQ(std::get<int>(db.get("test2")->data), 43);
}

TEST(MapTest, HandlesPositiveInput) {
    // TODO This should not be needed
    {
        DB db;
        db.destroy();
    }

    DB db;

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
    db.set("test8", newval6);

    db.set("test8.key1", map_val);
    db.set("test8.key2", map_val2);

    EXPECT_EQ(std::get<int>(db.get("test8.key1")->data), 1222);
    EXPECT_EQ(std::get<double>(db.get("test8.key2")->data), 1.2);
}