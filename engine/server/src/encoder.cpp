//
// Created by Tobias Panek on 21.11.24.
//


#include "encoder.h"

#include <iostream>
#include <sys/socket.h>

#include "tcp2_coding.h"

std::pair<std::unique_ptr<unsigned char[]>, int> GenericEncoder::get_bytes(size_t bytes) {
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

void GenericEncoder::add_to_buffer(std::vector<unsigned char> &block) {
    std::lock_guard<std::mutex> lock(this->buffer_lock);
    this->buffer.emplace_back(std::move(block));
}

bool GenericEncoder::is_empty() {
    std::lock_guard<std::mutex> lock(this->buffer_lock);
    return this->buffer.empty();
}

void GenericEncoder::from_string(std::string val) {
    std::vector<unsigned char> data(val.begin(), val.end());
    this->from_expanding_unsigned(data.size());
    this->add_to_buffer(data);
}

void GenericEncoder::from_int(int val) {
    this->from_unsigned(sizeof(int), *reinterpret_cast<unsigned long long*>(&val));
}

void GenericEncoder::from_double(double val) {
    this->from_unsigned(sizeof(double), *reinterpret_cast<unsigned long long*>(&val));
}

void GenericEncoder::from_acknak(bool val) {
    std::vector<unsigned char> ret{};
    ret.push_back(val ? 0xFF : 0x00);
    this->add_to_buffer(ret);
}

void GenericEncoder::from_ack() {
    this->from_acknak(true);
}

void GenericEncoder::from_nak() {
    this->from_acknak(false);
}

void GenericEncoder::from_unsigned(int bytes, unsigned long long val) {
    std::vector<unsigned char> res{};
    for (int x = 0; x < bytes; x++) {
        res.push_back(static_cast<unsigned char>(0xFF & val));
        val >>= 8;
    }
    this->add_to_buffer(res);
}

void GenericEncoder::from_expanding_unsigned(unsigned long long val) {
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

void GenericEncoder::from_general() {
    return;
}

TCP2Encoder::TCP2Encoder(const int client_socket, std::mutex *pipe_lock) : GenericEncoder(),
client_socket(client_socket), pipe_lock(pipe_lock) {
}

void TCP2Encoder::write_data() {
    auto data = this->get_bytes(ENCODER_BUFFER_SIZE);
    std::cout << "sending data:";
    for (int x = 0; x < data.second; x++) std::cout << "[" << (int)(unsigned char)data.first.get()[x] << "]";
    std::cout << std::endl;
    {
        std::cout << "requesting lock" << std::endl;
        std::lock_guard<std::mutex> lock(*this->pipe_lock);
        // todo think about error handling here
        send(this->client_socket, data.first.get(), data.second, 0);
    }
    std::cout << "data sent" << std::endl;
}

void TCP2Encoder::encode_error(unsigned long long error_id) {
    this->from_unsigned(2, error_id);
    if (errors::messages.contains(error_id)) this->from_string(errors::messages[error_id]);
    else this->from_string("Unknown Error!");
}

void TCP2Encoder::encode_nak_error(unsigned long long error_id) {
    this->from_acknak(false);
    this->encode_error(error_id);
}


