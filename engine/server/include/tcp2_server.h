/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include "acl.h"
#include <atomic>
#include <thread>
#include "tcp2_coding.h"


constexpr int BUFFER_SIZE = 8;


/**
 * @brief Represents a TCP server capable of handling multiple clients concurrently.
 */
class TCP2Server {
    int port;
    int max_clients;
    ACL acl{};
    std::atomic<bool> kill_acceptor{false};
    std::atomic<int> socket_error{0};

    std::thread client_acceptor;

public:
    TCP2Server(int port, int max_clients);
    bool load_acl_from_disk(std::string path);
    /**
     * @brief Starts the TCP server and begins accepting client connections.
     *
     * Spawns a separate thread to handle client connections using the
     * `tcp2_utils::client_acceptor` utility function.
     */
    void start();
    /**
     * @brief Stops the TCP server.
     *
     * Signals the client acceptor thread to terminate and waits for it to join.
     */
    void stop();

};




#endif //TCP_SERVER_H
