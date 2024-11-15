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

// todo think about a better solution that to pass a atomic in the constructor
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

std::string ByteDecoder::decode_string() {
    unsigned long long length = this->decode_expanding_unsigned();
    auto data = this->get_bytes(length);
    return "not done";
}

int ByteDecoder::decode_int() {
    unsigned long long res = this->decode_unsigned(sizeof(int));
    return static_cast<int>(res);
}

double ByteDecoder::decode_double() {
    unsigned long long res = this->decode_unsigned(sizeof(double));
    return static_cast<double>(res);
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

unsigned long long ByteDecoder::decode_expanding_unsigned() {
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
    std::cout << "requesting additional bytes: " << additional_length << std::endl;
    data = this->get_bytes(additional_length);
    unsigned long long res{};
    res |= first;
    for (int x = 0; x < additional_length; x++) {
        res <<= 8;
        res |= data[x];
    }
    return res;
}

std::pair<std::unique_ptr<unsigned char[]>, int> ByteEncoder::get_bytes(size_t bytes) {
    size_t cur_size{};
    std::unique_ptr<unsigned char[]> res(new unsigned char[bytes]);
    this->buffer_lock.lock();
    while (true) {
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

// todo if ull does not have 8 Bytes, this wont work for all cases
void ByteEncoder::encode_expanding_unsigned(unsigned long long val) {
    std::vector<unsigned char> data{};
    unsigned char face{0xff};
    bool add_all_now{false};
    if ((val >> 7*8) & 0xFF) { // val is actually completely full, all 8 bytes are sent
        data.push_back(0xFF);
        add_all_now = true;
    }
    for (int x = 0; x < 8; x++) {
        unsigned char cur = val >> (7-x)*8;
        if (add_all_now) data.push_back(cur);
        else {
            if ((0xFF & cur) > 0) { // this means there is overlap
                if ((face & cur) == 0) {
                    data.push_back((face << 1) | cur);
                } else {
                    data.push_back(face << 1);
                    data.push_back(cur);
                }
                add_all_now = true;
            }
            face <<= 1;
        }
    }
    this->add_to_buffer(data);
}


