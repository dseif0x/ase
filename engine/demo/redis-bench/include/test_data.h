/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <unordered_map>
#include <string>
#include <stopwatch.h>
#include <iostream>

inline std::unordered_map<std::string, std::string> get_test_data_32_time_32_times_16_two_deep() {
    std::unordered_map<std::string, std::string> data;
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int k = 0; k < 16; k++) {
                data["key" + std::to_string(i) + "." + std::to_string(j) + "." + std::to_string(k)] = std::string("value") + std::to_string(i);
            }
        }
    }
    return data;
}

inline std::unordered_map<std::string, std::string> get_test_data_16_time_1024_one_deep() {
    std::unordered_map<std::string, std::string> data;
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 1024; j++) {
            data["key" + std::to_string(i) + "." + std::to_string(j)] = std::string("value") + std::to_string(i);
        }
    }
    return data;
}

inline std::unordered_map<std::string, std::string> get_test_data_1024_time_16_one_deep() {
    std::unordered_map<std::string, std::string> data;
    for (int i = 0; i < 1024; i++) {
        for (int j = 0; j < 16; j++) {
            data["key" + std::to_string(i) + "." + std::to_string(j)] = std::string("value") + std::to_string(i);
        }
    }
    return data;
}

inline std::unordered_map<std::string, std::string> get_test_data_16384_flat() {
    std::unordered_map<std::string, std::string> data;
    for (int i = 0; i < 1024*16; i++) {
        data["key" + std::to_string(i)] = std::string("value") + std::to_string(i);
    }
    return data;
}

inline std::unordered_map<std::string, std::string> get_test_data_1048576_flat() {
    std::unordered_map<std::string, std::string> data;
    for (int i = 0; i < 1024*1024; i++) {
        data["key" + std::to_string(i)] = std::string("value") + std::to_string(i);
    }
    return data;
}

inline void print_stopwatch(stopwatch& s, std::string prefix = "Time: ") {
    auto duration = s.peek();
    auto microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    std::stringstream ss;
    if (microsduration < 9999) {
        ss << prefix << microsduration << "µs" << std::endl;
    } else {
        ss << prefix << microsduration / 1000 << "ms" << std::endl;
    }
    std::cout << ss.str();
}