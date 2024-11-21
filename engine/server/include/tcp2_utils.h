//
// Created by Tobias Panek on 18.11.24.
//

#ifndef TCP2_UTILS_H
#define TCP2_UTILS_H
#include "acl.h"
#include <atomic>
#include "tcp2_coding.h"
#include "decoder.h"
#include "encoder.h"

namespace tcp2_utils {
    // following functions are system dependent
    // todo add unix clause
    void client_acceptor(std::atomic<bool> &kys, ACL &acl, int port, int max_clients);
    void client_coding_setup(std::atomic<bool> &kys, int client_socket, ACL &acl);

    // following functions are system independent
    void client_main_thread(std::atomic<bool>& kys, TCP2Decoder &decoder, TCP2Encoder &encoder, ACL &acl);
    void client_state_thread(TCP2Decoder &decoder, TCP2Encoder &encoder, ACL &acl);
    void client_command_switch(ACLUser *user, TCP2Decoder &decoder, TCP2Encoder &encoder, unsigned char self_perms);

    namespace command {
        void stop(ACLUser *user, TCP2Decoder &decoder, TCP2Encoder &encoder);
    }

    namespace permissions {
        constexpr unsigned char none = 0x00;
        constexpr unsigned char read = 0x01;
        constexpr unsigned char write = 0x02;
        constexpr unsigned char del = 0x04;
        constexpr unsigned char admin = 0x08;
        constexpr unsigned char read_write = read | write;
        constexpr unsigned char read_write_del = read_write | del;
    }
}

#endif //TCP2_UTILS_H
