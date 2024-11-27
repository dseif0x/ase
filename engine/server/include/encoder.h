/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */


#ifndef ENCODER_H
#define ENCODER_H

// todo replace symbol with correct one
#define UNIX

#include <atomic>
#include <db_types.h>
#include <deque>
#include <unordered_map>
#include <vector>

constexpr int ENCODER_BUFFER_SIZE = 8;

class GenericEncoder {
    std::deque<std::vector<unsigned char>> buffer;
    std::mutex buffer_lock;
    size_t start = 0;

protected:
    std::pair<std::unique_ptr<unsigned char[]>, int> get_bytes(size_t bytes);
    void add_to_buffer(std::vector<unsigned char> &block);

    void from_basic_general(CopiedValue &val);
    void from_complex_general(CopiedValue &val);

public:
    virtual ~GenericEncoder() = default;

    GenericEncoder() = default;

    virtual bool write_data() = 0;

    bool is_empty();
    void clear_buffer();

    void from_string(std::string val);
    void from_int(int val);
    void from_double(double val);
    void from_acknak(bool val);
    void from_ack();
    void from_nak();

    void from_bool(bool val);
    void from_byte(unsigned char val);
    void from_unsigned(int bytes, unsigned long long val);
    void from_expanding_unsigned(unsigned long long val);
    bool from_general(CopiedValue &val);
};


class TCP2Encoder : public GenericEncoder {
    std::mutex *pipe_lock;

#ifdef UNIX
    int client_socket;
#elifdef WIN32
#endif

public:
#ifdef UNIX
    TCP2Encoder(int client_socket, std::mutex *pipe_lock);
#elifdef WIN32
#endif

    bool write_data() override;

    void encode_error(unsigned long long error_id);
    void encode_nak_error(unsigned long long error_id);
};


class FileEncoder : public GenericEncoder {
    std::string file_path;

public:
    explicit FileEncoder(std::string &file_path);

    bool write_data() override;
};


#endif //ENCODER_H
