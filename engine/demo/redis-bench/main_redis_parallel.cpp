/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "test_data.h"
#include <thread>
#include <hiredis.h>
#include <vector>

int do_run(const std::string& name, const std::unordered_map<std::string, std::string>& test_data, std::optional<std::ofstream> &out, std::string bench_type) {
    redisContext* c = redisConnect("127.0.0.1", 6379);
    if (c->err) {
        std::cerr << "error: " << c->errstr << std::endl;
        return 1;
    }

    std::stringstream ss;
    ss << "DMA par;" << bench_type << ";";

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

    auto duration = s.peek();
    auto microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    //print_stopwatch(s, std::string("\t") + name + " Set Time: ", out);
    std::string temp = std::string("\t") + name + " Set Time: ";
    print_duration(microsduration, temp);
    ss << microsduration << ";";

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

    duration = s.peek();
    microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    //print_stopwatch(s, std::string("\t") + name + " Set Time: ", out);
    std::string temp2 = std::string("\t") + name + " Get Time: ";
    print_duration(microsduration, temp2);
    ss << microsduration << ";\n";
    if (out && out->is_open()) *out << ss.str();

    return 0;
}

template<typename T>
int run_test(const char* name, T&& generate_test_data, std::optional<std::ofstream> &out) {
    std::cout << "Running Redis Parallel Test: " << name << std::endl;
    auto test_data = generate_test_data();

    constexpr int num_threads = 2;
    std::vector<std::unique_ptr<std::thread>> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(std::make_unique<std::thread>(do_run, std::string("Thread ") + std::to_string(i), test_data, std::ref(out), name));
    }

    for (auto& thread : threads) {
        thread->join();
    }

    return 0;
}

#include "entrypoint.h"