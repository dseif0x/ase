/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include "encoder.h"
#include "tcp2_coding.h"

TCP2Encoder::TCP2Encoder(const int client_socket, std::mutex *pipe_lock) : GenericEncoder(),
    client_socket(client_socket), pipe_lock(pipe_lock) {
}

bool TCP2Encoder::write_data() {
    auto data = this->get_bytes(ENCODER_BUFFER_SIZE);
    std::cout << "sending data:";
    for (int x = 0; x < data.second; x++) std::cout << "[" << (int)(unsigned char)data.first.get()[x] << "]";
    std::cout << std::endl;
    {
        std::lock_guard<std::mutex> lock(*this->pipe_lock);
        if (fcntl(this->client_socket, F_GETFD) == -1) return false;
        if (send(this->client_socket, data.first.get(), data.second, MSG_NOSIGNAL) < 0) return false;
    }
    return true;
}
