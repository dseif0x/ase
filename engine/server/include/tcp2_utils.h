/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */


#ifndef TCP2_UTILS_H
#define TCP2_UTILS_H
#include "acl.h"
#include <atomic>
#include <db.h>

#include "tcp2_coding.h"
#include "decoder.h"
#include "encoder.h"

// todo replace symbol with correct one
#define UNIX

namespace tcp2_utils {
    // client_acceptor needs a system dependent implementation but keeps the same signature
    void client_acceptor(std::atomic<bool> &kys, ACL &acl, DB &db, int port, int max_clients);
#ifdef UNIX
    // following function needs system dependent implementation as well as possibly different signatures
    void client_coding_setup(std::atomic<bool> &kys, int client_socket, ACL &acl, DB &db);
#elifdef WIN32
#endif

    // following functions are system independent
    void client_main_thread(std::atomic<bool>& kys, TCP2Decoder &decoder, TCP2Encoder &encoder, ACL &acl, DB &db);
    void client_state_thread(TCP2Decoder &decoder, TCP2Encoder &encoder, ACL &acl, DB &db);
    int client_command_switch(ACLUser *user, TCP2Decoder &decoder, TCP2Encoder &encoder, DB &db, unsigned char self_perms);

    std::optional<std::vector<std::string>> check_and_create_key(std::string &key);

    namespace command {
        int stop(ACLUser *user, TCP2Decoder &decoder, TCP2Encoder &encoder, DB &db);
        int ping(ACLUser *user, TCP2Decoder &decoder, TCP2Encoder &encoder, DB &db);
        int disconnect(ACLUser *user, TCP2Decoder &decoder, TCP2Encoder &encoder, DB &db);
        int get(ACLUser *user, TCP2Decoder &decoder, TCP2Encoder &encoder, DB &db);

        int testing(ACLUser *user, TCP2Decoder &decoder, TCP2Encoder &encoder, DB &db);
    }

    namespace permissions {
        constexpr unsigned char none = 0x00;
        constexpr unsigned char read = 0x01;
        constexpr unsigned char write = 0x02;
        constexpr unsigned char del = 0x04;
        constexpr unsigned char admin = 0x08;
        constexpr unsigned char read_write = read | write;
        constexpr unsigned char read_write_del = read_write | del;
        constexpr unsigned char all = read | write | del;
    }
}

#endif //TCP2_UTILS_H
