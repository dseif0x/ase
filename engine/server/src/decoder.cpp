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

std::unique_ptr<CopiedValue> GenericDecoder::to_basic_general(ValueType type) {
    auto val = std::make_unique<CopiedValue>();
    val->type = type;
    if (type == ValueType::BOOLEAN) {
        val->data = this->to_bool();
    } else if (type == ValueType::INT) {
        val->data = this->to_int();
    } else if (type == ValueType::STRING) {
        val->data = this->to_string();
    } else if (type == ValueType::DOUBLE) {
        val->data = this->to_double();
    } else if (type == ValueType::SEMAPHORE) {
        // todo implement sem
    } else if (type == ValueType::REFERENCE) {
        // todo implement ref
    } else {
        // todo think about error management
    }
    return val;
}

std::unique_ptr<CopiedValue> GenericDecoder::to_general() {
    auto val = std::make_unique<CopiedValue>();
    unsigned char info = this->to_byte();
    std::cout << "info:" << (int)info << std::endl;
    if (info_block::is_map(info)) {
        std::cout << "general: map" << std::endl;
        val->type = ValueType::MAP;
        CopiedValue::copied_map_t map;
        unsigned long long length = this->to_expanding_unsigned();
        for (unsigned long long x = 0; x < length; x++) {
            std::string key = this->to_string();
            map.emplace(key, std::move(this->to_general()));
        }
        val->data.emplace<CopiedValue::copied_map_t>(std::move(map));
    } else if (info_block::is_array(info)) {
        std::cout << "general: array" << std::endl;
        val->type = ValueType::ARRAY;
        unsigned long long length = this->to_expanding_unsigned();
        CopiedArrayType array{};
        array.size = length;
        array.array_value = new CopiedValue[length];
        for (unsigned long long x = 0; x < length; x++) {
            auto temp = info_block::is_complex(info) ?
                this->to_general() : this->to_basic_general(info_block::get_basic_type(info));
            array.array_value[x] = std::move(*temp);
        }
        val->data.emplace<CopiedArrayType>(array);
    } else if (info_block::is_pair(info)) {
        std::cout << "general: pair" << std::endl;
        val->type = ValueType::PAIR;
        CopiedValue::copied_pair_t pair;
        pair.first = std::move(info_block::is_complex(info) ? this->to_general() :
            this->to_basic_general(info_block::get_basic_type(info)));
        pair.second = std::move(this->to_general());
    } else {
        std::cout << "general: basic" << std::endl;
        // simple type
        return this->to_basic_general(info_block::get_basic_type(info));
    }
    return val;
}
