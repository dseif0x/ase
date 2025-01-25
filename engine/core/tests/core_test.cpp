/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <gtest/gtest.h>
#include <thread>
#include "shmem.h"
#include "shsem.h"

class Core : public ::testing::Test {
public:
    void SetUp() override {
        {
            shmem::destroy_data();
        }
        mem = new shmem();

    }

    void TearDown() override {
        delete mem;
    }

    shmem* mem{nullptr};
};

TEST_F(Core, SingleProcessAllocOnce) {
    using alloc_type = int;
    auto ptr = mem->construct<alloc_type>();
    EXPECT_EQ(ptr.get_offset(), mem->get_management_offset());
}

TEST_F(Core, SingleProcessAllocMultiple) {
    using alloc_type = int;

    auto prev_offset = mem->construct<alloc_type>();
    for (int i = 0; i < 1024; i++) {
        auto ptr = mem->construct<alloc_type>();
        EXPECT_GT(ptr.get_offset(), prev_offset.get_offset());
        prev_offset = ptr;
    }
}

TEST_F(Core, SingleProcessAllocDeallocSingle) {
    using alloc_type = int;
    auto ptr = mem->construct<alloc_type>();
    EXPECT_EQ(ptr.get_offset(), mem->get_management_offset());
    mem->destroy(ptr);
    auto ptr2 = mem->construct<alloc_type>();
    EXPECT_EQ(ptr.get_offset(), ptr2.get_offset());
}

TEST_F(Core, SingleProcessAllocDeallocMultiple) {
    using alloc_type = int;

    std::vector<offset_ptr<alloc_type>> ptrs;
    auto prev_offset = mem->construct<alloc_type>();
    ptrs.push_back(prev_offset);
    for (int i = 0; i < 1024; i++) {
        auto ptr = mem->construct<alloc_type>();
        EXPECT_GT(ptr.get_offset(), prev_offset.get_offset());
        ptrs.push_back(ptr);
        prev_offset = ptr;
    }

    for (auto& ptr : ptrs) {
        mem->destroy(ptr);
    }

    auto ptr = mem->construct<alloc_type>();
    EXPECT_EQ(ptr.get_offset(), mem->get_management_offset());
}

TEST_F(Core, MultiProcessAllocMultiple) {
    using alloc_type = int;

    auto do_lambda = [this](int count) {
        auto prev_offset = mem->construct<alloc_type>();
        for (int i = 0; i < count; i++) {
            auto ptr = mem->construct<alloc_type>();
            EXPECT_GT(ptr.get_offset(), prev_offset.get_offset());
            prev_offset = ptr;
        }
    };
    std::thread t1(do_lambda, 1024);
    std::thread t2(do_lambda, 1024 * 8);
    t1.join();
    t2.join();
}


TEST_F(Core, MultiProcessAllocDeallocMultiple) {
    using alloc_type = int;

    auto do_lambda = [this](int count) {
        std::vector<offset_ptr<alloc_type>> ptrs;
        auto prev_offset = mem->construct<alloc_type>();
        ptrs.push_back(prev_offset);
        for (int i = 0; i < count; i++) {
            auto ptr = mem->construct<alloc_type>();
            ptrs.push_back(ptr);
        }

        for (auto &ptr: ptrs) {
            mem->destroy(ptr);
        }
    };

    std::thread t1(do_lambda, 1024);
    std::thread t2(do_lambda, 1024 * 8);
    t1.join();
    t2.join();

    auto ptr = mem->construct<alloc_type>();
    EXPECT_EQ(ptr.get_offset(), mem->get_management_offset());
}

TEST(Data, DumpData) {
    shmem::destroy_data();
    auto* mem = new shmem([](shmem& mem) {
        mem.construct<int>(42);
    });
    auto x = mem->dump_data();
    mem->destroy();
    delete mem;
    auto* mem2 = new shmem();
    mem2->load_data(x);
    offset_ptr<int> tmp = mem2->get_management_offset();
    EXPECT_EQ(*tmp.get(*mem2), 42);
}

TEST_F(Core, SemaphoreAcquireRelease) {
    auto sem = mem->construct<shsem>(1);
    shsem::acquire(sem, *mem);
    EXPECT_EQ(sem.get(*mem)->get(), 0);
    sem.atomic_access(*mem, [](shsem* sem) {
        sem->release();
    });
    EXPECT_EQ(sem.get(*mem)->get(), 1);
}