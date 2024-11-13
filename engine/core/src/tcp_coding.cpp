/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "tcp_coding.h"

#include <thread>
#include <vector>

//todo here may be performance improvements made
std::vector<unsigned char> ByteDecoder::get_bytes(size_t bytes) {
    std::vector<unsigned char> res;
    res.reserve(bytes);
    this->buffer_lock.lock();
    while (true) {
        if (this->buffer.empty()) {
            this->buffer_lock.unlock();
            // free lock to allow new data to be written into the buffer
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "waiting on data" << std::endl;
            this->buffer_lock.lock();
            continue;
        }
        auto bytes_to_copy = std::min(bytes, this->buffer.front().size() - this->start);
        res.insert(res.end(), this->buffer.front().begin() + this->start,
            this->buffer.front().begin() + this->start + bytes_to_copy);
        bytes -= bytes_to_copy;
        this->start += bytes_to_copy;
        if (this->buffer.front().size() <= this->start) this->buffer.pop_front();
        if (bytes == 0) break;
    }
    this->buffer_lock.unlock();
    return res;
}

void ByteDecoder::add_to_buffer(std::vector<unsigned char> &block) {
    std::lock_guard<std::mutex> lock(this->buffer_lock);
    this->buffer.emplace_back(std::move(block));
}

bool ByteDecoder::empty() {
    std::lock_guard<std::mutex> lock(this->buffer_lock);
    return this->buffer.empty();
}

bool ByteDecoder::decode_acknak() {
    auto data = this->get_bytes(1);
    if (data[0] == 0xFF) return true;
    else if (data[1] == 0x00) return false;
    else throw 1; //todo throw something useful
}

unsigned long long ByteDecoder::decode_unsigned(int bytes) {
    auto data = this->get_bytes(bytes);
    unsigned long long val{};
    for (auto &x : data) {
        val <<= 8;
        val |= x;
    }
    return val;
}


