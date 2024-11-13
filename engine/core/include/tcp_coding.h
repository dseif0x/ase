/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#ifndef TCP_CODING_H
#define TCP_CODING_H

#include <iostream>
#include <deque>

class ByteDecoder {
    std::deque<std::vector<unsigned char>> buffer;
    std::mutex buffer_lock;

public:
    ByteDecoder() = default;

    void add_to_buffer(std::vector<unsigned char> block);

    std::string decode_string();
    int decode_int();
    double decode_double();
    bool decode_acknak();

    unsigned long long decode_unsigned(int bits);
    unsigned long long decode_expanding_unsigned(); // limited to the size of ull regardless of actual buffer
    void decode_general(); // todo will later return the some kind of db compatible type
};


class ByteEncoder {
    std::deque<std::vector<unsigned char>> buffer;
    std::mutex buffer_lock;

public:
    ByteEncoder() = default;

    void get_buffer(int bytes);

    void encode_string(std::string val);
    void encode_int(int val);
    void encode_double(double val);
    void encode_acknak(bool val);

    void encode_unsigned(int bits, unsigned long long val);
    void decode_expanding_unsigned(unsigned long long val);
    void encode_general(); // todo will later return the some kind of db compatible type
};

#endif //TCP_CODING_H
