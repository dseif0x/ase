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

// todo replace symbol with correct one
#define UNIX

constexpr int DECODER_BUFFER_SIZE = 8;

class GenericDecoder {
protected: // todo think about if there is an elegant way to encapsulate buffer into private
    std::deque<std::vector<unsigned char>> buffer;
    std::mutex buffer_lock;
    size_t start = 0;

    virtual std::vector<unsigned char> get_bytes(size_t bytes) = 0;
    void add_to_buffer(std::vector<unsigned char> &block);

    CopiedValue to_basic_general(ValueType type);
    CopiedValue to_complex_general();

public:
    virtual ~GenericDecoder() = default;

    GenericDecoder() = default;

    virtual void read_data() = 0;

    bool is_empty();
    void clear_buffer(int delay_ms);

    std::string to_string();
    int to_int();
    double to_double();
    int to_acknak();
    bool to_bool();
    unsigned long long to_unsigned(int bytes);
    unsigned char to_byte();
    unsigned long long to_expanding_unsigned(); // limited to the size of ull regardless of actual buffer
    std::optional<CopiedValue> to_general();
};


class TCP2Decoder : public GenericDecoder {
    std::atomic<bool> kys;
    // error message if tcp is closed or error
    std::string error_msg;
    // shared lock to protect tcp connection from simultaneous read and write
    std::mutex *pipe_lock;

#ifdef UNIX
    // tcp stuff
    int client_socket;
    struct timeval timeout{};
    fd_set readfds{};
    char tcp_buffer[DECODER_BUFFER_SIZE]{};
    ssize_t bytesReceived{};
#elifdef WIN32
    // tcp stuff for win
#endif
protected:
    std::vector<unsigned char> get_bytes(size_t bytes) override;

public:
#ifdef UNIX
    TCP2Decoder(int client_socket, std::mutex *pipe_lock);
#endif

    void read_data() override;

    // following functions are system independent implemented
    void set_kys();
    bool is_healthy();
    bool is_stop();
    std::string get_error();
};



#endif //DECODER_H
