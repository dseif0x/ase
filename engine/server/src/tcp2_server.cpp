/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <thread>
#include "tcp2_server.h"
#include "tcp_coding.h"
#include "tcp2_utils.h"
#include <atomic>



TCP2Server::TCP2Server(int port, int max_clients): port(port), max_clients(max_clients) {
}



void TCP2Server::start() {
    this->client_acceptor = std::thread(tcp2_utils::client_acceptor, std::ref(this->kill_acceptor), std::ref(this->acl), this->port, this->max_clients);
}

void TCP2Server::stop() {
    this->kill_acceptor = true;
    this->client_acceptor.join();
}
