/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "tcp2_utils.h"
#include <iostream>
#include <tcp2_server.h>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/_select.h>
#include "decoder.h"


std::vector<unsigned char> TCP2Decoder::get_bytes(size_t bytes) {
    std::vector<unsigned char> res;
    res.reserve(bytes);
    this->buffer_lock.lock();
    while (!this->kys) {
        if (bytes == 0) break;
        if (this->buffer.empty()) {
            this->buffer_lock.unlock();
            // free lock to allow new data to be written into the buffer
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            this->buffer_lock.lock();
            continue;
        }
        auto bytes_to_copy = std::min(bytes, this->buffer.front().size() - this->start);
        res.insert(res.end(), this->buffer.front().begin() + this->start,
            this->buffer.front().begin() + this->start + bytes_to_copy);
        bytes -= bytes_to_copy;
        this->start += bytes_to_copy;
        if (this->buffer.front().size() <= this->start) {
            this->buffer.pop_front();
            this->start = 0;
        }
    }
    this->buffer_lock.unlock();
    return res;
}

TCP2Decoder::TCP2Decoder(const int client_socket, std::mutex *pipe_lock): GenericDecoder(),
client_socket(client_socket), pipe_lock(pipe_lock), kys(false) {
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
}

bool TCP2Decoder::is_healthy() {
    return !this->kys;
}

std::string TCP2Decoder::get_error() {
    return this->error_msg;
}

void TCP2Decoder::read_data() {
    FD_ZERO(&this->readfds);
    FD_SET(this->client_socket, &this->readfds);
    int sel;
    {
        std::lock_guard<std::mutex> lock(*this->pipe_lock);
        sel = select(this->client_socket + 1, &this->readfds, nullptr, nullptr, &this->timeout);
    }
    if (sel > 0) {
        bytesReceived = recv(this->client_socket, tcp_buffer, BUFFER_SIZE, 0);
        if (bytesReceived == 0) {
            std::cout << "Client disconnected.\n";
            this->error_msg = "Client disconnected";
            this->kys = true;
        } else if (bytesReceived < 0) {
            std::cerr << "recv error: " << strerror(errno) << std::endl;
            this->error_msg = "error: recv: " + std::string(strerror(errno));
            this->kys = true;
        } else {
            std::cout << "Bytes: ";
            for (int x = 0; x < bytesReceived; x++) std::cout << "[" << (int)(unsigned char)tcp_buffer[x] << "]";
            std::cout << std::endl;
            std::vector<unsigned char> unsigned_buffer(tcp_buffer, tcp_buffer + bytesReceived);
            this->add_to_buffer(unsigned_buffer);
        }
    } else if (sel == 0) {
        // in case of timeout sleep for a bit to give others time to access pipe mutex
        // this way large data pack would not be slowed down, as we only sleep in case of nothing arriving
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // todo replace with yield
    } else {
        std::cerr << "select error: " << strerror(errno) << std::endl;
        this->error_msg = "error: select: " + std::string(strerror(errno));
        this->kys = true;
    }
}


void TCP2Decoder::set_kys() {
    this->kys = true;
}
