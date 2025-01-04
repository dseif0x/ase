/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "test_data.h"
#include <hiredis.h>

template<typename T>
int run_test(const char* name, T&& generate_test_data) {
    std::cout << "Running Redis Test: " << name << std::endl;
    auto test_data = generate_test_data();
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
    print_stopwatch(s, "\tSet Time: ");
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
    print_stopwatch(s, "\tGet Time: ");
    return 0;
}

#include "entrypoint.h"