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
#include <optional>

#include "sys/time.h"
#include "db_types.h"

#if defined(WIN)
#include <winsock2.h>
#endif

constexpr int DECODER_BUFFER_SIZE = 8;
/**
 * @brief Provides methods for decoding various data types from a byte buffer.
 */
class GenericDecoder {
protected: // todo think about if there is an elegant way to encapsulate buffer into private
    std::deque<std::vector<unsigned char>> buffer;
    std::mutex buffer_lock;
    size_t start = 0;

    virtual std::vector<unsigned char> get_bytes(size_t bytes) = 0;
    /**
     * @brief Adds a block of data to the decoder's buffer.
     *
     * @param block The data block to add.
     */
    void add_to_buffer(std::vector<unsigned char> &block);

    /**
     * @brief Decodes a general basic value based on its type.
     *
     * @param type The value type to decode.
     * @return The decoded value as a CopiedValue.
     */
    CopiedValue to_basic_general(ValueType type);
    /**
     * @brief Decodes a complex general value from the buffer.
     *
     * @return The decoded value as a CopiedValue.
     */
    CopiedValue to_complex_general();

public:
    virtual ~GenericDecoder() = default;

    GenericDecoder() = default;

    virtual void read_data() = 0;

    /**
     * @brief Checks if the buffer is empty.
     *
     * @return True if the buffer is empty, otherwise false.
     */
    bool is_empty();

    /**
     * Clears the buffer. Waits for a defined time to see if new data arrives which will then be deleted also.
     * A time between 200ms and 500ms is advised.
     * @param delay_ms The time in ms which must pass without new data arriving for the function to return
     */
    void clear_buffer(int delay_ms = 250);

    /**
     * @brief Decodes the buffer content as a string.
     *
     * @return A string decoded from the buffer.
     */
    std::string to_string();
    /**
     * @brief Decodes the buffer content as an integer.
     *
     * @return An integer decoded from the buffer.
     */
    int to_int();
    /**
     * @brief Decodes the buffer content as a double.
     *
     * @return A double decoded from the buffer.
     */
    double to_double();
    /**
     * @brief Decodes a single byte as an acknowledgment or negative acknowledgment.
     *
     * @return 1 for acknowledgment, 0 for negative acknowledgment, -1 for invalid.
     */
    int to_acknak();
    /**
     * @brief Decodes the buffer content as a boolean.
     *
     * @return A boolean decoded from the buffer.
     */
    bool to_bool();
    /**
     * @brief Decodes the buffer content as an unsigned integer.
     *
     * @param bytes The number of bytes to decode.
     * @return An unsigned integer decoded from the buffer.
     */
    unsigned long long to_unsigned(int bytes);
    /**
     * @brief Decodes a single byte from the buffer.
     *
     * @return The decoded byte.
     */
    unsigned char to_byte();
    /**
     * @brief Decodes an expanding unsigned integer from the buffer.
     *
     * @return The decoded unsigned integer.
     */
    unsigned long long to_expanding_unsigned(); // limited to the size of ull regardless of actual buffer
    /**
     * @brief Decodes a general value, handling errors gracefully.
     *
     * @return An optional containing the decoded value or empty on failure.
     */
    std::optional<CopiedValue> to_general();
};

/**
 * @brief Specialized decoder for TCP data streams.
 */
class TCP2Decoder : public GenericDecoder {
    std::atomic<bool> kys;
    // error message if tcp is closed or error
    std::string error_msg;
    // shared lock to protect tcp connection from simultaneous read and write
    std::mutex *pipe_lock;

#if defined(UNIX)
    // tcp stuff
    int client_socket;
#elif defined(WIN)
    // tcp stuff for win
    SOCKET client_socket;
#endif
    struct timeval timeout{};
    fd_set readfds{};
    char tcp_buffer[DECODER_BUFFER_SIZE]{};
    ssize_t bytesReceived{};
protected:
    /**
     * @brief Retrieves a specified number of bytes from the buffer.
     *
     * @param bytes The number of bytes to retrieve.
     * @return A vector containing the retrieved bytes.
     */
    std::vector<unsigned char> get_bytes(size_t bytes) override;

public:
#ifdef UNIX
    TCP2Decoder(int client_socket, std::mutex *pipe_lock);
#elifdef WIN
    TCP2Decoder(SOCKET client_socket, std::mutex *pipe_lock);
#endif

    void read_data() override;

    // following functions are system independent implemented
    /**
     * @brief Marks the decoder as stopped.
     */
    void set_kys();
    /**
     * @brief Checks if the decoder is in a healthy state.
     *
     * @return True if the decoder is healthy, otherwise false.
     */
    bool is_healthy();
    /**
     * @brief Checks if the decoder is in a stop state.
     *
     * @return True if the decoder is in a stop state, otherwise false.
     */
    bool is_stop();
    /**
     * @brief Retrieves the error message from the decoder.
     *
     * @return The error message.
     */
    std::string get_error();
};

/**
* Specialized decoder for decoding from files.
*/
class FileDecoder : public GenericDecoder {
    std::string file_path;

public:
    FileDecoder(std::string &file_path);

    std::vector<unsigned char> get_bytes(size_t bytes) override;
    void read_data() override;
};



#endif //DECODER_H
