/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */


#include <cstring>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "decoder.h"
#include "tcp2_utils.h"
#include "tcp2_server.h"

TCP2Decoder::TCP2Decoder(const int client_socket, std::mutex *pipe_lock): GenericDecoder(),
client_socket(client_socket), pipe_lock(pipe_lock), kys(false) {
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
}

void TCP2Decoder::read_data() {
    this->pipe_lock->lock();
    FD_ZERO(&this->readfds);
    FD_SET(this->client_socket, &this->readfds);
    int sel;
    sel = select(this->client_socket + 1, &this->readfds, nullptr, nullptr, &this->timeout);
    if (sel > 0) {
        bytesReceived = recv(this->client_socket, tcp_buffer, BUFFER_SIZE, 0);
        this->pipe_lock->unlock();
        if (bytesReceived == 0) {
            std::cout << "Client disconnected.\n";
            this->error_msg = "Client disconnected";
            this->kys = true;
        } else if (bytesReceived < 0) {
            std::cerr << "recv error: " << strerror(errno) << std::endl;
            this->error_msg = "error: recv: " + std::string(strerror(errno));
            this->kys = true;
        } else {
#ifdef DEBUG
            std::cout << "Bytes: ";
            for (int x = 0; x < bytesReceived; x++) std::cout << "[" << (int)(unsigned char)tcp_buffer[x] << "]";
            std::cout << std::endl;
#endif
            std::vector<unsigned char> unsigned_buffer(tcp_buffer, tcp_buffer + bytesReceived);
            this->add_to_buffer(unsigned_buffer);
        }
    } else if (sel == 0) {
        // in case of timeout sleep for a bit to give others time to access pipe mutex
        // this way large data pack would not be slowed down, as we only sleep in case of nothing arriving
        this->pipe_lock->unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } else {
        this->pipe_lock->unlock();
        std::cerr << "select error: " << strerror(errno) << std::endl;
        this->error_msg = "error: select: " + std::string(strerror(errno));
        this->kys = true;
    }
}

