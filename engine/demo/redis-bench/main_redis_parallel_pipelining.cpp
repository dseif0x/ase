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

int do_run(const std::string& name,
           const std::unordered_map<std::string, std::string>& test_data,
           std::optional<std::ofstream> &out,
           std::string bench_type)
{
    redisContext* c = redisConnect("127.0.0.1", 6379);
    if (c == nullptr || c->err) {
        std::cerr << "Error: " << (c ? c->errstr : "Can't allocate redis context") << std::endl;
        if (c) redisFree(c);
        return 1;
    }

    std::stringstream ss;
    ss << "Redis par;" << bench_type << ";";

    // -------------------------------
    // 1. Pipeline all SET commands
    // -------------------------------
    stopwatch s;
    for (const auto& [key, value] : test_data) {
        // Append a SET command to the pipeline
        if (redisAppendCommand(c, "SET %s %s", key.c_str(), value.c_str()) != REDIS_OK) {
            std::cerr << "Error appending SET command for key: " << key << std::endl;
            redisFree(c);
            return 1;
        }
    }

    // Read all SET replies from the pipeline
    for (size_t i = 0; i < test_data.size(); ++i) {
        redisReply* reply = nullptr;
        if (redisGetReply(c, reinterpret_cast<void**>(&reply)) != REDIS_OK) {
            std::cerr << "Error getting SET reply: " << c->errstr << std::endl;
            redisFree(c);
            return 1;
        }
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "Error setting key (pipelined): "
                      << (reply ? reply->str : "NULL") << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(c);
            return 1;
        }
        freeReplyObject(reply);
    }

    // Measure time for the SET phase
    auto duration = s.peek();
    auto microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    std::string temp = std::string("\t") + name + " Set Time: ";
    print_duration(microsduration, temp);
    ss << microsduration << ";";

    // -------------------------------
    // 2. Pipeline all GET commands
    // -------------------------------
    s.reset();  // reset stopwatch for GET phase
    for (const auto& [key, _] : test_data) {
        // Append a GET command to the pipeline
        if (redisAppendCommand(c, "GET %s", key.c_str()) != REDIS_OK) {
            std::cerr << "Error appending GET command for key: " << key << std::endl;
            redisFree(c);
            return 1;
        }
    }

    // Read all GET replies from the pipeline
    for (size_t i = 0; i < test_data.size(); ++i) {
        redisReply* reply = nullptr;
        if (redisGetReply(c, reinterpret_cast<void**>(&reply)) != REDIS_OK) {
            std::cerr << "Error getting GET reply: " << c->errstr << std::endl;
            redisFree(c);
            return 1;
        }
        if (!reply || reply->type != REDIS_REPLY_STRING) {
            std::cerr << "Error getting key (pipelined): "
                      << (reply ? reply->str : "NULL") << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(c);
            return 1;
        }
        freeReplyObject(reply);
    }

    // Measure time for the GET phase
    duration = s.peek();
    microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    std::string temp2 = std::string("\t") + name + " Get Time: ";
    print_duration(microsduration, temp2);
    ss << microsduration << ";\n";

    // If there's an output file, log our line
    if (out && out->is_open()) {
        *out << ss.str();
    }

    // Cleanup
    redisFree(c);
    return 0;
}

template<typename T>
int run_test(const char* name, T&& generate_test_data, std::optional<std::ofstream> &out) {
    std::cout << "Running Redis Parallel Test (Pipelined): " << name << std::endl;
    auto test_data = generate_test_data();

    constexpr int num_threads = 2;
    std::vector<std::unique_ptr<std::thread>> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(std::make_unique<std::thread>(
                do_run,
                std::string("Thread ") + std::to_string(i),
                test_data,
                std::ref(out),
                name
        ));
    }

    for (auto& thread : threads) {
        thread->join();
    }

    return 0;
}

#include "entrypoint.h"