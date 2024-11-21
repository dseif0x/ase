/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#ifndef DECODER_H
#define DECODER_H
#include <atomic>
#include <deque>
#include <mutex>
#include "db_types.h"

constexpr int DECODER_BUFFER_SIZE = 8;

class GenericDecoder {
protected: // todo think about if there is an elegant way to encapsulate buffer into private
    std::deque<std::vector<unsigned char>> buffer;
    std::mutex buffer_lock;
    size_t start = 0;

    virtual std::vector<unsigned char> get_bytes(size_t bytes) = 0;
    void add_to_buffer(std::vector<unsigned char> &block);

    std::unique_ptr<CopiedValue> to_basic_general(ValueType type);

public:
    virtual ~GenericDecoder() = default;

    GenericDecoder() = default;

    virtual void read_data() = 0;

    bool is_empty();

    std::string to_string();
    int to_int();
    double to_double();
    int to_acknak();
    bool to_bool();
    unsigned long long to_unsigned(int bytes);
    unsigned char to_byte();
    unsigned long long to_expanding_unsigned(); // limited to the size of ull regardless of actual buffer
    std::unique_ptr<CopiedValue> to_general();
};


class TCP2Decoder : public GenericDecoder {
// todo update with correct symbol (ifdef UNIX)
    std::atomic<bool> kys;

    // tcp stuff
    int client_socket;
    struct timeval timeout{};
    fd_set readfds{};
    char tcp_buffer[DECODER_BUFFER_SIZE]{};
    ssize_t bytesReceived{};

    // shared lock to protect tcp connection from simultaneous read and write
    std::mutex *pipe_lock;

    // error message if tcp is closed or error
    std::string error_msg;
protected:
    std::vector<unsigned char> get_bytes(size_t bytes) override;

public:
    TCP2Decoder(int client_socket, std::mutex *pipe_lock);

    void read_data() override;

    void set_kys();
    bool is_healthy();
    std::string get_error();
};



#endif //DECODER_H
