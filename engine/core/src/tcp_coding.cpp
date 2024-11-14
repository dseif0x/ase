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
    while (!(*this->kys)) {
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
        if (this->buffer.front().size() <= this->start) {
            this->buffer.pop_front();
            this->start = 0;
        }
        if (bytes == 0) break;
    }
    this->buffer_lock.unlock();
    return res;
}

ByteDecoder::ByteDecoder(std::atomic<bool> *kys): kys(kys) {
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

std::pair<std::unique_ptr<unsigned char[]>, int> ByteEncoder::get_bytes(size_t bytes) {
    size_t cur_size{};
    std::unique_ptr<unsigned char[]> res(new unsigned char[bytes]);
    this->buffer_lock.lock();
    while (true) {
        std::cout << "front:";
        for (int x = 0; x < this->buffer.front().size(); x++) std::cout << "[" << (int)(unsigned char)this->buffer.front()[x] << "]";
        std::cout << std::endl;
        if (this->buffer.empty()) break;
        auto bytes_to_copy = std::min(bytes, this->buffer.front().size() - this->start);
        std::copy(this->buffer.front().begin() + this->start,
            this->buffer.front().begin() + this->start + bytes_to_copy, res.get()+cur_size);
        bytes -= bytes_to_copy;
        cur_size += bytes_to_copy;
        this->start += bytes_to_copy;
        if (this->buffer.front().size() <= this->start) {
            this->buffer.pop_front();
            this->start = 0;
        }
        std::cout << "ret:";
        for (int x = 0; x < cur_size; x++) std::cout << "[" << (int)res.get()[x] << "]";
        std::cout << std::endl;
        if (bytes == 0) break;
    }
    this->buffer_lock.unlock();
    return std::make_pair(std::move(res), cur_size);
}

void ByteEncoder::add_to_buffer(std::vector<unsigned char> &block) {
    std::lock_guard<std::mutex> lock(this->buffer_lock);
    this->buffer.emplace_back(std::move(block));
}

bool ByteEncoder::empty() {
    std::lock_guard<std::mutex> lock(this->buffer_lock);
    return this->buffer.empty();
}

void ByteEncoder::encode_acknak(bool val) {
    std::vector<unsigned char> ret{};
    ret.push_back(val ? 0xFF : 0x00);
    this->add_to_buffer(ret);
}

void ByteEncoder::encode_unsigned(int bytes, unsigned long long val) {
    std::vector<unsigned char> res{};
    for (int x = 0; x < bytes; x++) {
        res.push_back(static_cast<unsigned char>(0xFF & val));
        val >>= 8;
    }
    this->add_to_buffer(res);
}


