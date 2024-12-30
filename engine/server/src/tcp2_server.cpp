/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <thread>
#include "tcp2_server.h"
#include "tcp2_utils.h"
#include <atomic>


TCP2Server::TCP2Server(TCP_ACL &acl, DB &db, int port, int max_clients): port(port), max_clients(max_clients), acl(acl), db(db) {
}

TCP2Server::~TCP2Server() {
    this->stop();
}

void TCP2Server::start() {
    if (!this->running) {
        this->running = true;
        this->client_acceptor = std::thread(tcp2_utils::client_acceptor, std::ref(this->kill_acceptor), std::ref(this->acl), std::ref(this->db), this->port, this->max_clients);
    }
}

void TCP2Server::stop() {
    if (this->running) {
        this->kill_acceptor = true;
        this->client_acceptor.join();
        this->running = false;
    }
}