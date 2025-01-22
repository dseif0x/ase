/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "test_data.h"
#include <hiredis.h>
template<typename T>
int run_test(const char* name, T&& generate_test_data, std::optional<std::ofstream> &out) {
    if (out && out->is_open()) {
        // Label this as a pipelined test in your CSV output, for clarity
        *out << "Redis Pipelined;" << name << ";";
    }

    std::cout << "Running Redis Test (Pipelined): " << name << std::endl;
    auto test_data = generate_test_data();

    // Connect to Redis
    redisContext* c = redisConnect("127.0.0.1", 6379);
    if (c == nullptr || c->err) {
        std::cerr << "Connection error: " << (c ? c->errstr : "can't allocate redis context") << std::endl;
        if (c) redisFree(c);
        return 1;
    }

    stopwatch s;

    //---------------------------------------------
    // 1) Pipeline all SET commands
    //---------------------------------------------
    for (const auto& [key, value] : test_data) {
        if (redisAppendCommand(c, "SET %s %s", key.c_str(), value.c_str()) != REDIS_OK) {
            std::cerr << "Error appending SET command: " << c->errstr << std::endl;
            redisFree(c);
            return 1;
        }
    }

    // 2) Receive all replies for the SET commands
    for (std::size_t i = 0; i < test_data.size(); ++i) {
        redisReply* reply = nullptr;
        if (redisGetReply(c, reinterpret_cast<void**>(&reply)) != REDIS_OK) {
            std::cerr << "Error getting SET reply: " << c->errstr << std::endl;
            redisFree(c);
            return 1;
        }

        // Check for Redis error in the reply
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "Error in SET reply: "
                      << (reply ? reply->str : "NULL reply") << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(c);
            return 1;
        }

        freeReplyObject(reply);
    }
    print_stopwatch(s, "\tSet Time: ", out);

    //---------------------------------------------
    // 3) Pipeline all GET commands
    //---------------------------------------------
    s.reset();
    for (const auto& [key, _] : test_data) {
        if (redisAppendCommand(c, "GET %s", key.c_str()) != REDIS_OK) {
            std::cerr << "Error appending GET command: " << c->errstr << std::endl;
            redisFree(c);
            return 1;
        }
    }

    // 4) Receive all replies for the GET commands
    for (std::size_t i = 0; i < test_data.size(); ++i) {
        redisReply* reply = nullptr;
        if (redisGetReply(c, reinterpret_cast<void**>(&reply)) != REDIS_OK) {
            std::cerr << "Error getting GET reply: " << c->errstr << std::endl;
            redisFree(c);
            return 1;
        }

        // Verify we got a string reply
        if (!reply || reply->type != REDIS_REPLY_STRING) {
            std::cerr << "Error in GET reply: "
                      << (reply ? reply->str : "NULL reply") << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(c);
            return 1;
        }

        freeReplyObject(reply);
    }
    print_stopwatch(s, "\tGet Time: ", out);

    // Optionally close out your CSV row
    if (out && out->is_open()) {
        *out << std::endl;
    }

    // Clean up
    redisFree(c);
    return 0;
}

#include "entrypoint.h"