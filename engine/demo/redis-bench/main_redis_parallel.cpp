/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "test_data.h"
#include <thread>
#include <hiredis.h>

int do_run(const std::string& name, const std::unordered_map<std::string, std::string>& test_data) {
    redisContext* c = redisConnect("127.0.0.1", 6379);
    if (c->err) {
        std::cerr << "error: " << c->errstr << std::endl;
        return 1;
    }

    stopwatch s;
    for (const auto& [key, value] : test_data) {
        redisReply* reply = (redisReply*)redisCommand(c, "SET %s %s", key.c_str(), value.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "Error setting key: " << key << " (" << (reply ? reply->str : "NULL") << ")" << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(c);
            return 1;
        }
        freeReplyObject(reply);
    }
    print_stopwatch(s, std::string("\t") + name + " Set Time: ");
    s.reset();
    for (const auto& [key, _] : test_data) {
        redisReply* reply = (redisReply*)redisCommand(c, "GET %s", key.c_str());
        if (!reply || reply->type != REDIS_REPLY_STRING) {
            std::cerr << "Error getting key: " << key << " (" << (reply ? reply->str : "NULL") << ")" << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(c);
            return 1;
        }
        freeReplyObject(reply);
    }
    print_stopwatch(s, std::string("\t") + name + " Get Time: ");
    return 0;
}

template<typename T>
int run_test(const char* name, T&& generate_test_data) {
    std::cout << "Running Redis Parallel Test: " << name << std::endl;
    auto test_data = generate_test_data();

    constexpr int num_threads = 2;
    std::vector<std::unique_ptr<std::thread>> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(std::make_unique<std::thread>(do_run, std::string("Thread ") + std::to_string(i), test_data));
    }

    for (auto& thread : threads) {
        thread->join();
    }

    return 0;
}

#include "entrypoint.h"