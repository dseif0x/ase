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
    constexpr uint16_t general_protocol_error       = 0xFFFF;

    constexpr uint16_t permission_denied_self       = 0xFFF0;
    constexpr uint16_t permission_denied            = 0xFF00;

    constexpr uint16_t invalid_version              = 0xFF01;
    constexpr uint16_t user_not_found               = 0xFF02;
    constexpr uint16_t wrong_password               = 0xFF03;
    constexpr uint16_t already_connected            = 0xFF04;
    constexpr uint16_t username_already_exists      = 0xFF05;

    constexpr uint16_t invalid_key                  = 0x0100;
    constexpr uint16_t key_not_found                = 0x0101;
    constexpr uint16_t key_already_exists           = 0x0200;

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
    };
}

#endif //TCP_CODING_H
