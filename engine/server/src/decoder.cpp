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


void GenericDecoder::add_to_buffer(std::vector<unsigned char> &block) {
    std::lock_guard<std::mutex> lock(this->buffer_lock);
    this->buffer.emplace_back(std::move(block));
}

bool GenericDecoder::is_empty() {
    std::lock_guard<std::mutex> lock(this->buffer_lock);
    return this->buffer.empty();
}

std::string GenericDecoder::to_string() {
    unsigned long long length = this->to_expanding_unsigned();
    auto data = this->get_bytes(length);
    return {data.begin(), data.end()};
}

int GenericDecoder::to_int() {
    unsigned long long res = this->to_unsigned(sizeof(int));
    return *reinterpret_cast<int*>(&res);
}

double GenericDecoder::to_double() {
    unsigned long long res = this->to_unsigned(sizeof(double));
    return *reinterpret_cast<double*>(&res);
}

int GenericDecoder::to_acknak() {
    auto data = this->get_bytes(1);
    if (data[0] == 0xFF) return 1;
    if (data[1] == 0x00) return 0;
    return -1;
}

unsigned long long GenericDecoder::to_unsigned(int bytes) {
    auto data = this->get_bytes(bytes);
    unsigned long long val{};
    for (auto &x : data) {
        val <<= 8;
        val |= x;
    }
    return val;
}

unsigned long long GenericDecoder::to_expanding_unsigned() {
    auto data = this->get_bytes(1);
    unsigned char first = data[0];
    int additional_length = 0;
    for (int x = 0; x < 8; x++) {
        if ((first & 0x80) > 0) {
            additional_length++;
            first <<= 1;
        }
    }
    first >>= additional_length;
    data = this->get_bytes(additional_length);
    unsigned long long res{};
    res |= first;
    for (int x = 0; x < additional_length; x++) {
        res <<= 8;
        res |= data[x];
    }
    return res;
}

void GenericDecoder::to_general() {
    return;
}
