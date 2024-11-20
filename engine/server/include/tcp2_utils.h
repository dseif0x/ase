//
// Created by Tobias Panek on 18.11.24.
//

#ifndef TCP2_UTILS_H
#define TCP2_UTILS_H
#include "acl.h"
#include <atomic>
#include "tcp2_coding.h"
#include "decoder.h"

namespace tcp2_utils {
    // following functions are system dependent
    void client_acceptor(std::atomic<bool> &kys, ACL &acl, int port, int max_clients);

    // following functions are system independent
    void client_main_thread(std::atomic<bool>& kys, int clientSocket, ACL &acl);
    void client_state_thread(int clientSocket, std::mutex &pipe_lock, ACL &acl, TCP2Decoder &decoder);
    void client_command_switch(ACLUser *user, TCP2Decoder &decoder, ByteEncoder &encoder, unsigned char self_perms);

    namespace command {
        void stop(ACLUser *user, TCP2Decoder &decoder, ByteEncoder &encoder);
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
