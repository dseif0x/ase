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


constexpr int BUFFER_SIZE = 8;

class TCPServer {
    int port;
    int max_clients;
    ACL acl{};
    std::atomic<bool> kill_acceptor{false};
    std::atomic<int> socket_error{0};

    std::thread client_acceptor;
    static void accept_clients(std::atomic<bool> &kys, ACL &acl, int port, int max_clients);

public:
    TCPServer(int port, int max_clients);
    bool load_acl_from_disk(std::string path);
    void start();
    void stop();

};




#endif //TCP_SERVER_H
