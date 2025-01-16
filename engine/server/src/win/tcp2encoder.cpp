/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <winsock2.h>
#include "encoder.h"
#include "tcp2_coding.h"
#include <thread>

TCP2Encoder::TCP2Encoder(const SOCKET client_socket) : GenericEncoder(),
                                                                           client_socket(client_socket) {
}

bool TCP2Encoder::write_data() {
    auto data = this->get_bytes(ENCODER_BUFFER_SIZE);
    std::cout << "sending data:";
    for (int x = 0; x < data.second; x++) std::cout << "[" << (int)(unsigned char)data.first.get()[x] << "]";
    std::cout << std::endl;
    {
        if (this->client_socket == INVALID_SOCKET) return false;
        if (send(this->client_socket, reinterpret_cast<const char*>(data.first.get()), data.second, 0) == SOCKET_ERROR) return false;
    }
    return true;
}