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
    /**
     * @brief Extracts a specified number of bytes from the buffer.
     *
     * This method locks the buffer, extracts `bytes` number of bytes, and manages
     * the internal state of the buffer (e.g., removing empty blocks).
     *
     * @param bytes The number of bytes to extract.
     * @return A pair containing a unique pointer to the extracted bytes and the number of bytes extracted.
     */
    std::pair<std::unique_ptr<unsigned char[]>, int> get_bytes(size_t bytes);
    /**
     * @brief Adds a block of data to the buffer.
     *
     * This method locks the buffer and appends a new block of data to it.
     *
     * @param block The block of data to add, passed as a vector of unsigned char.
     */
    void add_to_buffer(std::vector<unsigned char> &block);
    /**
     * @brief Encodes a basic data value into the buffer.
     *
     * Handles simple types like booleans, integers, strings, and doubles.
     *
     * @param val The value to encode.
     * @throws std::invalid_argument If the value type is invalid.
     */
    void from_basic_general(CopiedValue &val);
    /**
     * @brief Encodes a complex data structure into the buffer.
     *
     * Handles more complex types like arrays, maps, and pairs, supporting nested structures.
     *
     * @param val The complex value to encode.
     */
    void from_complex_general(CopiedValue &val);

public:
    virtual ~GenericEncoder() = default;

    GenericEncoder() = default;

    virtual bool write_data() = 0;

    /**
     * @brief Checks whether the buffer is empty.
     *
     * Locks the buffer and checks if there is any data present.
     *
     * @return True if the buffer is empty, false otherwise.
     */
    bool is_empty();
    /**
     * @brief Clears all data from the buffer.
     *
     * Locks the buffer, clears its contents, and resets the internal state.
     */
    void clear_buffer();

    /**
     * @brief Encodes a string into the buffer.
     *
     * Converts a string into a vector of unsigned chars and encodes its length
     * before appending it to the buffer.
     *
     * @param val The string to encode.
     */
    void from_string(std::string val);

    /**
     * @brief Encodes an integer into the buffer.
     *
     * Converts an integer into a fixed-size unsigned representation
     * and appends it to the buffer.
     *
     * @param val The integer to encode.
     */
    void from_int(int val);
    /**
     * @brief Encodes a double into the buffer.
     *
     * Converts a double into a fixed-size unsigned representation
     * and appends it to the buffer.
     *
     * @param val The double to encode.
     */
    void from_double(double val);
    /**
     * @brief Encodes an acknowledgment or negative acknowledgment into the buffer.
     *
     * Adds a single byte to the buffer to represent acknowledgment (0xFF)
     * or negative acknowledgment (0x00).
     *
     * @param val True for acknowledgment, false for negative acknowledgment.
     */
    void from_acknak(bool val);
    /**
     * @brief Encodes an acknowledgment (ACK) into the buffer.
     */
    void from_ack();
    /**
     * @brief Encodes a negative acknowledgment (NAK) into the buffer.
     */
    void from_nak();

    /**
     * @brief Encodes a boolean value into the buffer.
     *
     * Converts a boolean value into a single byte and appends it to the buffer.
     *
     * @param val The boolean value to encode.
     */
    void from_bool(bool val);
    /**
     * @brief Encodes a single byte into the buffer.
     *
     * Adds the byte to the buffer as a vector of unsigned char.
     *
     * @param val The byte to encode.
     */
    void from_byte(unsigned char val);
    /**
     * @brief Encodes an unsigned value into the buffer with fixed byte length.
     *
     * Converts the unsigned value to a vector of bytes and appends it to the buffer.
     *
     * @param bytes The number of bytes to use for the representation.
     * @param val The unsigned value to encode.
     */
    void from_unsigned(int bytes, unsigned long long val);
    /**
     * @brief Encodes an unsigned value into the buffer using expanding representation.
     *
     * Dynamically determines the number of bytes required to represent the value
     * and appends it to the buffer.
     *
     * @param val The unsigned value to encode.
     */
    void from_expanding_unsigned(unsigned long long val);
    /**
     * @brief Encodes a general data value into the buffer.
     *
     * Attempts to encode the given value, catching any exceptions that occur
     * during the process.
     *
     * @param val The value to encode.
     * @return True if encoding succeeds, false otherwise.
     */
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
    /**
     * @brief Encodes an error message into the buffer.
     *
     * Encodes an error ID along with a corresponding error message.
     *
     * @param error_id The unique identifier for the error.
     */
    void encode_error(unsigned long long error_id);
    /**
     * @brief Encodes a negative acknowledgment (NAK) followed by an error message.
     *
     * Adds a NAK byte, encodes the error ID, and appends an error message.
     *
     * @param error_id The unique identifier for the error.
     */
    void encode_nak_error(unsigned long long error_id);
};


#endif //ENCODER_H
