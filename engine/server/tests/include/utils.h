/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <vector>

template <typename T>
bool vec_equ(const std::vector<T>& vec1, const std::vector<T>& vec2) {
    if (vec1.size() != vec2.size()) {
        return false;
    }

    return std::equal(vec1.begin(), vec1.end(), vec2.begin());
}

template <typename T>
unsigned char get_nth_byte(T val, int n) {
    return (unsigned char)((*(unsigned long long*)&val)>>(n*8));
}


inline void print_vec(std::vector<unsigned char> &vec) {
    std::cout << "vek:";
    for (auto &x : vec) std::cout << (int)x << ", ";
    std::cout << std::endl;
}

constexpr unsigned char hex_to_byte(char c) {
    return ('0' <= c && c <= '9') ? c - '0' :
           ('a' <= c && c <= 'f') ? c - 'a' + 10 :
           ('A' <= c && c <= 'F') ? c - 'A' + 10 :
           throw "Invalid hex digit";
}

constexpr std::vector<unsigned char> make_hex_vector(const char* hex) {
    std::vector<unsigned char> result;
    for (size_t i = 0; hex[i] && hex[i + 1]; i += 2) {
        if (hex[i] == ' ') i -= 1;
        else result.push_back((hex_to_byte(hex[i]) << 4) | hex_to_byte(hex[i + 1]));
    }
    return result;
}

constexpr size_t count_hex_valid_chars(const std::string& input) {
    size_t count = 0;

    for (char c : input) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            ++count;
        }
    }

    return count;
}

#endif //UTILS_H
