/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <fstream>
#include <unordered_map>
#include <string>
#include <stopwatch.h>
#include <iostream>
#include <random>
#include <sstream>
#include <unordered_set>

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

template<typename RNG>
std::string generate_random_string(RNG& rng, int length) {
    static const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    std::string result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        result += charset[dist(rng)];
    }
    return result;
}

// Function to generate deterministic data
inline std::unordered_map<std::string, std::string> get_unique_keys(int process, int keys, int max_depth) {
    // Seed the random engine with the given parameter to ensure determinism
    std::mt19937 rng(process);

    std::unordered_map<std::string, std::string> result;
    std::uniform_int_distribution<> subkey_count_dist(1, max_depth);    // For number of subkeys
    std::uniform_int_distribution<> subkey_length_dist(1, 10);         // For subkey length
    std::uniform_int_distribution<> value_length_dist(1, 128);          // For value length
    std::vector<std::unordered_set<std::string>> used;
    for (int x = 0; x < max_depth; x++) {
        used.push_back(std::unordered_set<std::string>());
    }

    for (int key_idx = 0; key_idx < keys; ++key_idx) {
        int subkey_count = subkey_count_dist(rng);

        // have to check if duplicate keys are inserted as set would break
        std::ostringstream key_stream;
        for (int sub_idx = 0; sub_idx < subkey_count; ++sub_idx) {
            int subkey_length = subkey_length_dist(rng);
            std::string temp = generate_random_string(rng, subkey_length);
            if (used[sub_idx].contains(temp)) {
                sub_idx--; //retry
                continue;
            }
            key_stream << "." << temp;
            used[sub_idx].insert(temp);
        }

        std::string key = key_stream.str();
        int value_length = value_length_dist(rng);
        std::string value = generate_random_string(rng, value_length);
        //if (value.size() >= 28) std::cout << "long val: " << value << std::endl;

        result[key] = value;
    }

    return result;
}

inline void print_duration(long long microsduration, const std::string &prefix) {
    std::stringstream ss;
    if (microsduration < 9999) {
        ss << prefix << microsduration << "µs" << std::endl;
    } else {
        ss << prefix << microsduration / 1000 << "ms" << std::endl;
    }
    std::cout << ss.str();
}

inline void print_stopwatch(stopwatch& s, const std::string &prefix, std::optional<std::ofstream> &out) {
    auto duration = s.peek();
    auto microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    if (out && out->is_open()) *out << microsduration << ";";
    print_duration(microsduration, prefix);
}

