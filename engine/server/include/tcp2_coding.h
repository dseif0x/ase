/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#ifndef TCP_CODING_H
#define TCP_CODING_H

#include <atomic>
#include <db_types.h>
#include <deque>
#include <unordered_map>
#include <vector>


/**
 * @brief Contains error codes and their corresponding messages for the protocol.
 */
namespace errors {
    constexpr uint16_t general_protocol_error       = 0xFFFF;
    constexpr uint16_t general_server_error         = 0xFFFE;

    constexpr uint16_t permission_denied_self       = 0xFFF0;
    constexpr uint16_t permission_denied            = 0xFF00;

    constexpr uint16_t invalid_version              = 0xFF01;
    constexpr uint16_t user_not_found               = 0xFF02;
    constexpr uint16_t wrong_password               = 0xFF03;
    constexpr uint16_t already_connected            = 0xFF04;
    constexpr uint16_t username_already_exists      = 0xFF05;
    constexpr uint16_t unknown_command              = 0xFF06;

    constexpr uint16_t invalid_key                  = 0x0100;
    constexpr uint16_t key_not_found                = 0x0101;
    constexpr uint16_t key_already_exists           = 0x0200;
    constexpr uint16_t invalid_value                = 0x0201;

    static std::unordered_map<uint16_t, std::string> messages{
        {general_protocol_error, "General Protocol Error."},
        {permission_denied_self, "Connection denies permission."},
        {invalid_version, "Invalid client version."},
        {permission_denied, "Permission denied."},
        {user_not_found, "There is no user with that username."},
        {wrong_password, "The provided password is invalid."},
        {already_connected, "The provided user is already connected to the DB."},
        {invalid_key, "Invalid Key."},
        {key_not_found, "Key not found."},
        {key_already_exists, "Key already exists."},
        {username_already_exists, "The provided username already exists."},
        {unknown_command, "The provided command code is unknown."},
        {general_server_error, "An unexpected error occurred."},
        {invalid_value, "Invalid value. Could not extract a valid value."}
    };
}

namespace info_block {
    constexpr unsigned char array_bit = 0x80;
    constexpr unsigned char pair_bit = 0x40;
    constexpr unsigned char complex_bit = 0x20;
    constexpr unsigned char map_bit = 0x10;
    constexpr unsigned char basic_mask = 0x07;

    /**
     * @brief Checks if the byte represents an array.
     * @param byte The byte to check.
     * @return True if the byte represents an array; otherwise false.
     */
    constexpr bool is_array(const unsigned char byte) {
        return (byte & array_bit) > 0;
    }

    /**
     * @brief Checks if the byte represents a pair.
     * @param byte The byte to check.
     * @return True if the byte represents a pair; otherwise false.
     */
    constexpr bool is_pair(const unsigned char byte) {
        return (byte & pair_bit) > 0;
    }

    /**
     * @brief Checks if the byte represents a complex type.
     * @param byte The byte to check.
     * @return True if the byte represents a complex type; otherwise false.
     */
    constexpr bool is_complex(const unsigned char byte) {
        return (byte & complex_bit) > 0;
    }

    /**
     * @brief Checks if the byte represents a map.
     * @param byte The byte to check.
     * @return True if the byte represents a map; otherwise false.
     */
    constexpr bool is_map(const unsigned char byte) {
        return (byte & map_bit) > 0;
    }

    /**
     * @brief Retrieves the basic type from the given byte.
     * @param byte The byte to evaluate.
     * @return The corresponding ValueType.
     */
    constexpr ValueType get_basic_type(const unsigned char byte) {
        switch (byte & basic_mask) {
            case 1:
                return ValueType::BOOLEAN;
            case 2:
                return ValueType::INT;
            case 3:
                return ValueType::STRING;
            case 4:
                return ValueType::DOUBLE;
            case 5:
                return ValueType::SEMAPHORE;
            case 6:
                return ValueType::REFERENCE;
            default:
                return ValueType::UNKNOWN;
        }
    }

    /**
     * @brief Retrieves the byte representation of the given ValueType.
     * @param type The ValueType to encode.
     * @return The corresponding byte.
     */
    constexpr unsigned char get_basic_byte(const ValueType type) {
        switch (type) {
            case ValueType::BOOLEAN:
                return 1;
            case ValueType::INT:
                return 2;
            case ValueType::STRING:
                return 3;
            case ValueType::DOUBLE:
                return 4;
            case ValueType::SEMAPHORE:
                return 5;
            case ValueType::REFERENCE:
                return 6;
            default:
                return 0;
        }
    }
}

#endif //TCP_CODING_H
