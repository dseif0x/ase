/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#ifndef TCP_CODING_H
#define TCP_CODING_H

#include <atomic>
#include <deque>
#include <unordered_map>
#include <vector>


namespace errors {
    static std::unordered_map<unsigned long long, std::string> messages{
        {0xFFFF, "General Protocol Error."},
        {0x8000, "Invalid client version."},
        {0x8001, "There is no user with that username."},
        {0x8002, "The provided password is invalid."},
    };
}

/**
*   The ByteDecoder class is used to decode byte data into c++ datatypes.
*   There are two ways to use the function:
*   1. Passing a std::atomic<bool> into the constructor.
*      This allows for different threads to simultaneously add and extract data from the class.
*      If data is requested that is not in the structure, the function will block and wait for data until data arrives
*        or the passed atomic is set to true.
*   2. Not passing anything to the function:
*      Meant for use in a single thread. Add data once and extract it again.
*      if data is requested that is not in the structure, the function will throw an exception.
*/
class ByteDecoder {
    std::deque<std::vector<unsigned char>> buffer;
    std::mutex buffer_lock;
    size_t start = 0;
    std::atomic<bool> *kys{nullptr};

    std::vector<unsigned char> get_bytes(size_t bytes);

public:
    ByteDecoder(std::atomic<bool> *kys);

    void add_to_buffer(std::vector<unsigned char> &block);
    bool empty();

    std::string decode_string();
    int decode_int();
    double decode_double();
    int decode_acknak();

    unsigned long long decode_unsigned(int bytes);
    unsigned long long decode_expanding_unsigned(); // limited to the size of ull regardless of actual buffer
    void decode_general(); // todo will later return the some kind of db compatible type
};


class ByteEncoder {
    std::deque<std::vector<unsigned char>> buffer;
    std::mutex buffer_lock;
    size_t start = 0;

public:
    ByteEncoder() = default;

    std::pair<std::unique_ptr<unsigned char[]>, int> get_bytes(size_t bytes);
    void add_to_buffer(std::vector<unsigned char> &block);
    bool empty();

    void encode_string(std::string val);
    void encode_int(int val);
    void encode_double(double val);
    void encode_acknak(bool val);
    void encode_ack();
    void encode_nak();

    void encode_error(unsigned long long error_id);
    void encode_nak_error(unsigned long long error_id);

    void encode_unsigned(int bytes, unsigned long long val);
    void encode_expanding_unsigned(unsigned long long val);
    void encode_general(); // todo will later return the some kind of db compatible type
};

#endif //TCP_CODING_H
