/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <gtest/gtest.h>
#include "db.h"
#include "utils.h"
#include "map_test.h"

void insert_test_data(DB& db) {
    { // Integer
        CopiedValue newval{};
        newval.type = ValueType::INT;
        newval.data.emplace<int>(42);
        db.set("test", std::move(newval));


        CopiedValue newval2{};
        newval2.type = ValueType::INT;
        newval2.data.emplace<int>(43);
        db.set("test2", std::move(newval2));
    }

    { // String
        CopiedValue newval2{};
        newval2.type = ValueType::STRING;
        newval2.data.emplace<std::string>("Hello World!");
        db.set("test3", std::move(newval2));
    }

    { // Boolean
        CopiedValue newval3{};
        newval3.type = ValueType::BOOLEAN;
        newval3.data.emplace<bool>(true);
        db.set("test4", std::move(newval3));
    }

    { // Double
        CopiedValue newval4{};
        newval4.type = ValueType::DOUBLE;
        newval4.data.emplace<double>(1.2);
        db.set("test5", std::move(newval4));
    }

    { // Array
        CopiedValue newval5{};
        newval5.type = ValueType::ARRAY;
        auto& vector = newval5.data.emplace<CopiedValue::copied_array_t>();

        CopiedValue arrval1{};
        arrval1.type = ValueType::INT;
        arrval1.data.emplace<int>(13);
        CopiedValue arrval2{};
        arrval2.type = ValueType::INT;
        arrval2.data.emplace<int>(12);

        vector.emplace_back(std::move(arrval1));
        vector.emplace_back(std::move(arrval2));

        db.set("test6", std::move(newval5));
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
        db.set("test7", std::move(newval6));
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
        db.set("test8", std::move(newval6));
    }
    { // Reference
        CopiedValue newval1{};
        newval1.type = ValueType::REFERENCE;
        newval1.data.emplace<std::string>("reference2");
        db.set("reference1", std::move(newval1));
        CopiedValue newval2{};
        newval2.type = ValueType::REFERENCE;
        newval2.data.emplace<std::string>("reference3");
        db.set("reference2", std::move(newval2));
        CopiedValue newval3{};
        newval3.type = ValueType::REFERENCE;
        newval3.data.emplace<std::string>("referenceVal");
        db.set("reference3", std::move(newval3));
        CopiedValue newval{};
        newval.type = ValueType::INT;
        newval.data.emplace<int>(123);
        db.set("referenceVal", std::move(newval));
    }
    { // Delete
        CopiedValue newval{};
        newval.type = ValueType::INT;
        newval.data.emplace<int>(123);
        db.set("toDelete", std::move(newval));
    }
}

class DatabaseWithData : public ::testing::Test {
public:
    void SetUp() override {
        DB::destroy();
        db = new DB();

        insert_test_data(*db);
    }

    void TearDown() override {
        delete db;
    }

    DB* db{nullptr};
};

class DatabaseWithoutData : public ::testing::Test {
public:
    void SetUp() override {
        DB::destroy();
        db = new DB();
    }

    void TearDown() override {
        delete db;
    }

    DB* db{nullptr};
};

TEST_F(DatabaseWithData, Subdomain) {
    {
        CopiedValue newval{};
        newval.type = ValueType::INT;
        newval.data = 15;
        db->set(".top.second.sub.leaf", std::move(newval));
    }
    {
        auto temp = db->get(".top.second.sub.leaf");
        ASSERT_EQ(get<int>(temp->data), 15);
    }
}

TEST_F(DatabaseWithData, GetInt) {
    auto test = get<int>(*db, "test");
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value(), 42);

    test = get<int>(*db, "test2");
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value(), 43);
}

TEST_F(DatabaseWithData, GetIntSetForce) {
    auto test = get<int>(*db, "test");
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value(), 42);

    CopiedValue newval{};
    newval.type = ValueType::INT;
    newval.data = 15;
    db->set("test", std::move(newval), false);

    test = get<int>(*db, "test");
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value(), 42);

    CopiedValue newval2{};
    newval2.type = ValueType::INT;
    newval2.data = 15;
    db->set("test", std::move(newval2), true);

    test = get<int>(*db, "test");
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value(), 15);
}

TEST_F(DatabaseWithData, GetString) {
    auto test_string = get<std::string>(*db, "test3");
    ASSERT_TRUE(test_string.has_value());
    ASSERT_EQ(test_string.value(), "Hello World!");
}

TEST_F(DatabaseWithData, GetBoolean) {
    auto test_bool = get<bool>(*db, "test4");
    ASSERT_TRUE(test_bool.has_value());
    ASSERT_EQ(test_bool.value(), true);
}

TEST_F(DatabaseWithData, GetDouble) {
    auto test_double = get<double>(*db, "test5");
    ASSERT_TRUE(test_double.has_value());
    ASSERT_EQ(test_double.value(), 1.2);
}

TEST_F(DatabaseWithData, GetArray) {
    auto test_array = get<CopiedValue::copied_array_t>(*db, "test6");
    ASSERT_TRUE(test_array.has_value());
    auto& test_vec = test_array.value();

    ASSERT_EQ(test_vec.size(), 2);
    auto& first = test_vec[0];
    auto& second = test_vec[1];
    ASSERT_EQ(first.type, ValueType::INT);
    ASSERT_EQ(get<int>(first.data), 13);

    ASSERT_EQ(second.type, ValueType::INT);
    ASSERT_EQ(get<int>(second.data), 12);
}

TEST_F(DatabaseWithData, GetPair) {
    auto test_pair_opt = get<CopiedValue::copied_pair_t>(*db, "test7");
    ASSERT_TRUE(test_pair_opt.has_value());
    auto& test_pair = test_pair_opt.value();

    auto& first = test_pair.first;
    auto& second = test_pair.second;
    ASSERT_EQ(first->type, ValueType::INT);
    ASSERT_EQ(get<int>(first->data), 182);

    ASSERT_EQ(second->type, ValueType::DOUBLE);
    ASSERT_EQ(get<double>(second->data), 1.7);
}

TEST_F(DatabaseWithData, GetMap) {
    auto test_map_opt = get<CopiedValue::copied_map_t>(*db, "test8");
    ASSERT_TRUE(test_map_opt.has_value());
    auto& test_map = test_map_opt.value();

    ASSERT_EQ(test_map.size(), 2);
    ASSERT_TRUE(test_map.contains("key1"));
    ASSERT_TRUE(test_map.contains("key2"));

    auto& first = test_map["key1"];
    auto& second = test_map["key2"];
    ASSERT_EQ(first->type, ValueType::INT);
    ASSERT_EQ(get<int>(first->data), 1222);

    ASSERT_EQ(second->type, ValueType::DOUBLE);
    ASSERT_EQ(get<double>(second->data), 1.2);
}

TEST_F(DatabaseWithData, GetSubKey) {
    auto test_int = get<int>(*db, "test8.key1");
    ASSERT_TRUE(test_int.has_value());
    ASSERT_EQ(test_int.value(), 1222);

    auto test_double = get<double>(*db, "test8.key2");
    ASSERT_TRUE(test_double.has_value());
    ASSERT_EQ(test_double.value(), 1.2);
}

TEST_F(DatabaseWithData, GetNonExistentKey) {
    auto test = get<int>(*db, "test_123");
    ASSERT_FALSE(test.has_value());
}

TEST_F(DatabaseWithData, GetExistingKeyWithWrongType) {
    auto test = get<std::string>(*db, "test");
    ASSERT_FALSE(test.has_value());
}


TEST_F(DatabaseWithData, Follow) {
    auto test = follow<int>(*db, "reference1", 999);
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value(), 123);
}

TEST_F(DatabaseWithData, FollowWithLimit) {
    auto test = follow<int>(*db, "reference3", 1);
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value(), 123);
}

TEST_F(DatabaseWithData, FollowWithUnderestimatedLimit) {
    auto test = db->follow("reference2", 1);
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value().type, ValueType::REFERENCE);
    ASSERT_EQ(std::get<std::string>(test.value().data), "referenceVal");
}

TEST_F(DatabaseWithData, FollowInt) {
    auto test = follow<int>(*db, "referenceVal", 99);
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value(), 123);
}

TEST_F(DatabaseWithData, Follow0) {
    auto test = db->follow("reference1", 0);
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value().type, ValueType::REFERENCE);
    ASSERT_EQ(std::get<std::string>(test.value().data), "reference2");
}

TEST_F(DatabaseWithData, Delete) {
    auto test = get<int>(*db, "toDelete");
    ASSERT_TRUE(test.has_value());
    db->remove("toDelete");
    test = get<int>(*db, "toDelete");
    ASSERT_FALSE(test.has_value());
}

TEST_F(DatabaseWithData, List) {
    auto test = db->list("test8");
    ASSERT_TRUE(test.has_value());
    auto& map = test.value();
    ASSERT_TRUE(map.contains("key1") && map.contains("key2"));
    ASSERT_EQ(map["key2"].first, ValueType::DOUBLE);
    ASSERT_EQ(map["key1"].first, ValueType::INT);
    ASSERT_EQ(std::get<double>(map["key2"].second), 1.2);
    ASSERT_EQ(std::get<int>(map["key1"].second), 1222);
}

TEST_F(DatabaseWithData, ListNonExistentKey) {
    auto test = db->list("test_123");
    ASSERT_FALSE(test.has_value());
}

TEST_F(DatabaseWithData, StoreAndLoad) {
    auto data = db->dump_data();

    db->remove("test");

    auto test = get<int>(*db, "test");
    ASSERT_FALSE(test.has_value());

    db->load_data(data);

    test = get<int>(*db, "test");
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value(), 42);
}

TEST_F(DatabaseWithoutData, CreateLock) {
    constexpr auto init = 4;
    db->create_lock("lock", init);
    auto lock = db->get("lock");
    ASSERT_TRUE(lock.has_value());
    ASSERT_EQ(lock.value().type, ValueType::SEMAPHORE);
    ASSERT_EQ(std::get<int>(lock.value().data), init);
}

TEST_F(DatabaseWithoutData, CreateAndAquireLock) {
    constexpr auto init = 4;
    db->create_lock("lock", init);
    auto lock = db->get("lock");
    ASSERT_TRUE(lock.has_value());
    ASSERT_EQ(lock.value().type, ValueType::SEMAPHORE);
    ASSERT_EQ(std::get<int>(lock.value().data), init);

    db->acquire_lock("lock");

    lock = db->get("lock");
    ASSERT_TRUE(lock.has_value());
    ASSERT_EQ(lock.value().type, ValueType::SEMAPHORE);
    ASSERT_EQ(std::get<int>(lock.value().data), init - 1);

    db->release_lock("lock");

    lock = db->get("lock");
    ASSERT_TRUE(lock.has_value());
    ASSERT_EQ(lock.value().type, ValueType::SEMAPHORE);
    ASSERT_EQ(std::get<int>(lock.value().data), init);
}

TEST_F(DatabaseWithoutData, CreateAndAquireLockMultiple) {
    constexpr auto init = 4;
    db->create_lock("lock", init);
    auto lock = db->get("lock");
    ASSERT_TRUE(lock.has_value());
    ASSERT_EQ(lock.value().type, ValueType::SEMAPHORE);
    ASSERT_EQ(std::get<int>(lock.value().data), init);

    for (int i = 0; i < init; i++) {
        db->acquire_lock("lock");
    }

    lock = db->get("lock");
    ASSERT_TRUE(lock.has_value());
    ASSERT_EQ(lock.value().type, ValueType::SEMAPHORE);
    ASSERT_EQ(std::get<int>(lock.value().data), 0);

    for (int i = 0; i < init; i++) {
        db->release_lock("lock");
    }

    lock = db->get("lock");
    ASSERT_TRUE(lock.has_value());
    ASSERT_EQ(lock.value().type, ValueType::SEMAPHORE);
    ASSERT_EQ(std::get<int>(lock.value().data), init);
}

TEST_F(DatabaseWithoutData, ParallelSet) {
    constexpr auto count = 4096;
    constexpr auto threads = 8;

    CopiedValue newval{};
    newval.type = ValueType::INT;
    newval.data = 1;
    db->set("test", std::move(newval), true);
    auto val = db->get("test");
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(val.value().type, ValueType::INT);
    ASSERT_EQ(std::get<int>(val.value().data), 1);

    auto do_lambda = [this](int count) {
        for (int i = 0; i < count; i++) {
            CopiedValue newval{};
            newval.type = ValueType::INT;
            newval.data = i;
            db->set("test", std::move(newval), true);
        }
    };
    std::vector<std::unique_ptr<std::thread>> threads_vec;
    threads_vec.reserve(threads);
    for (int i = 0; i < threads; i++) {
        threads_vec.emplace_back(std::make_unique<std::thread>(do_lambda, count / threads));
    }

    for (auto& thread : threads_vec) {
        thread->join();
    }

    auto post_inc = db->get("test");
    ASSERT_TRUE(post_inc.has_value());
    ASSERT_EQ(post_inc.value().type, ValueType::INT);
    ASSERT_EQ(std::get<int>(post_inc.value().data), count/threads - 1);
}

TEST_F(DatabaseWithoutData, ParallelRemove) {
    constexpr auto count = 4096;
    constexpr auto threads = 8;

    auto do_lambda = [this](int begin, int end) {
        for (int i = begin; i < end; i++) {
            CopiedValue newval{};
            newval.type = ValueType::INT;
            newval.data = i;
            auto name = std::string("test") + std::to_string(i);
            db->set(name.c_str(), std::move(newval), true);
        }
    };

    std::vector<std::unique_ptr<std::thread>> threads_vec;
    threads_vec.reserve(threads);
    for (int i = 0; i < threads; i++) {
        threads_vec.emplace_back(std::make_unique<std::thread>(do_lambda, i * count / threads, (i+1) * count / threads));
    }

    for (auto& thread : threads_vec) {
        thread->join();
    }
    threads_vec.clear();

    auto post_inc = db->list(".");
    ASSERT_TRUE(post_inc.has_value());
    ASSERT_EQ(post_inc.value().size(), count);

    auto remove_lambda = [this](int begin, int end) {
        for (int i = begin; i < end; i++) {
            auto name = std::string("test") + std::to_string(i);
            db->remove(name.c_str());
        }
    };

    for (int i = 0; i < threads; i++) {
        threads_vec.emplace_back(std::make_unique<std::thread>(remove_lambda, i * count / threads, (i+1) * count / threads));
    }

    for (auto& thread : threads_vec) {
        thread->join();
    }

    auto post_rem = db->list(".");
    ASSERT_TRUE(post_rem.has_value());
    ASSERT_EQ(post_rem.value().size(), 0);
}

TEST_F(DatabaseWithoutData, StoreAndLoadEmpty) {
    auto data = db->dump_data();

    CopiedValue newval{};
    newval.type = ValueType::INT;
    newval.data.emplace<int>(42);
    db->set("test", std::move(newval));

    auto test = get<int>(*db, "test");
    ASSERT_TRUE(test.has_value());
    ASSERT_EQ(test.value(), 42);

    db->load_data(data);

    test = get<int>(*db, "test");
    ASSERT_FALSE(test.has_value());
}