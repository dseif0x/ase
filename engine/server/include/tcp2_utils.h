//
// Created by Tobias Panek on 18.11.24.
//

#ifndef TCP2_UTILS_H
#define TCP2_UTILS_H
#include "acl.h"
#include <atomic>
#include "tcp_coding.h"

namespace tcp2_utils {
    // following functions are system dependent
    void client_acceptor(std::atomic<bool> &kys, ACL &acl, int port, int max_clients);

    // following functions are system independent
    void client_main_thread(std::atomic<bool>& kys, int clientSocket, ACL &acl);
    void client_state_thread(std::atomic<bool>& kys, int clientSocket, std::mutex &pipe_lock, ACL &acl, ByteDecoder &decoder);
}

#endif //TCP2_UTILS_H
