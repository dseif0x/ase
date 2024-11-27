/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <tcp2_server.h>
#include <thread>
#include "decoder.h"
#include <fstream>
#include <optional>


void GenericDecoder::add_to_buffer(std::vector<unsigned char> &block) {
    std::lock_guard<std::mutex> lock(this->buffer_lock);
    this->buffer.emplace_back(std::move(block));
}

bool GenericDecoder::is_empty() {
    std::lock_guard<std::mutex> lock(this->buffer_lock);
    return this->buffer.empty();
}

/**
 * Clears the buffer. Waits for a defined time to see if new data arrives which will then be deleted also.
 * A time between 200ms and 500ms is advised.
 * @param delay_ms The time in ms which must pass without new data arriving for the function to return
 */
void GenericDecoder::clear_buffer(int delay_ms) {
    while (!this->is_empty()) {
        {
            std::lock_guard<std::mutex> lock(this->buffer_lock);
            this->buffer.clear();
            this->start = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
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

bool GenericDecoder::to_bool() {
    return *reinterpret_cast<bool*>(this->to_byte());
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

unsigned char GenericDecoder::to_byte() {
    return this->get_bytes(1)[0];
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

CopiedValue GenericDecoder::to_basic_general(ValueType type) {
    auto val = CopiedValue();
    val.type = type;
    if (type == ValueType::BOOLEAN) {
        val.data = this->to_bool();
    } else if (type == ValueType::INT) {
        val.data = this->to_int();
    } else if (type == ValueType::STRING) {
        val.data = std::move(this->to_string());
    } else if (type == ValueType::DOUBLE) {
        val.data = this->to_double();
    } else if (type == ValueType::SEMAPHORE) {
        // todo implement sem
    } else if (type == ValueType::REFERENCE) {
        // todo implement ref
    } else {
        throw std::invalid_argument("unkown type");
    }
    return val;
}

std::optional<CopiedValue> GenericDecoder::to_general() {
    try {
        return std::optional<CopiedValue>{this->to_complex_general()};
    } catch (...) {
        return std::nullopt;
    }
}

CopiedValue GenericDecoder::to_complex_general() {
    auto val = CopiedValue();
    unsigned char info = this->to_byte();
    if (info_block::is_map(info)) {
        val.type = ValueType::MAP;
        CopiedValue::copied_map_t map;
        unsigned long long length = this->to_expanding_unsigned();
        for (unsigned long long x = 0; x < length; x++) {
            std::string key = this->to_string();
            map.emplace(std::move(key), std::make_unique<CopiedValue>(this->to_complex_general()));
        }
        val.data.emplace<CopiedValue::copied_map_t>(std::move(map));
    } else if (info_block::is_array(info)) {
        val.type = ValueType::ARRAY;
        unsigned long long length = this->to_expanding_unsigned();
        auto &vec = val.data.emplace<CopiedValue::copied_array_t>();
        for (unsigned long long x = 0; x < length; x++) {
            auto temp = info_block::is_complex(info) ?
                this->to_complex_general() : this->to_basic_general(info_block::get_basic_type(info));
            vec.emplace_back(std::move(temp));
        }
    } else if (info_block::is_pair(info)) {
        val.type = ValueType::PAIR;
        auto &pair = val.data.emplace<CopiedValue::copied_pair_t>();
        pair.first = (std::make_unique<CopiedValue>(std::move(info_block::is_complex(info) ? this->to_complex_general() :
            this->to_basic_general(info_block::get_basic_type(info)))));
        pair.second = std::make_unique<CopiedValue>(std::move(this->to_complex_general()));
    } else {
        // simple type
        return this->to_basic_general(info_block::get_basic_type(info));
    }
    return val;
}

bool TCP2Decoder::is_healthy() {
    return !this->kys;
}

bool TCP2Decoder::is_stop() {
    std::unique_lock<std::mutex> lock(this->buffer_lock);
    return (!this->buffer.empty() && !buffer.front().empty() && buffer.front()[0] == 0xFE);
}

std::string TCP2Decoder::get_error() {
    return this->error_msg;
}

void TCP2Decoder::set_kys() {
    this->kys = true;
}

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

FileDecoder::FileDecoder(std::string &file_path) : file_path(file_path) {
}

/**
 * Returns requested bytes from the buffer or throws an error if this is not possible.
 * This function is non-blocking if the lock is available.
 *
 * @param bytes The requested amount of bytes.
 * @return The requested bytes.
 */
std::vector<unsigned char> FileDecoder::get_bytes(size_t bytes) {
    std::vector<unsigned char> res;
    res.reserve(bytes);
    this->buffer_lock.lock();
    while (true) {
        if (bytes == 0 || this->buffer.empty()) break;
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
    if (res.size() != bytes) throw std::invalid_argument("Not enough data to extract given bytes.");
    return res;
}

void FileDecoder::read_data() {
    std::ifstream file(file_path, std::ios::binary);

    if (!file) {
        throw std::ios_base::failure("Error: Unable to open file.");
    }

    int block_size = 64;
    char buffer[block_size];
    while (true) {
        file.read(buffer, block_size);
        std::streamsize bytes_read = file.gcount();

        if (bytes_read == 0) break; // eof
        std::vector<unsigned char> unsigned_buffer(buffer, buffer + bytes_read);
        this->add_to_buffer(unsigned_buffer);

        if (file.eof()) break; // eof

        if (file.fail() && !file.eof()) throw std::ios_base::failure("Could not read file.");
    }

    file.close();
}
