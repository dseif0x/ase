/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */


#include "encoder.h"

#include <iostream>
#include <tcp2_utils.h>
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

void GenericEncoder::clear_buffer() {
    std::lock_guard<std::mutex> lock(this->buffer_lock);
    this->buffer.clear();
    this->start = 0;
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

void GenericEncoder::from_bool(bool val) {
    this->from_byte(*reinterpret_cast<unsigned char*>(val));
}

void GenericEncoder::from_byte(unsigned char val) {
    std::vector<unsigned char> res{};
    res.push_back(val);
    this->add_to_buffer(res);
}

void GenericEncoder::from_unsigned(int bytes, unsigned long long val) {
    std::vector<unsigned char> res{};
    for (int x = 0; x < bytes; x++) {
        res.insert(res.begin(), static_cast<unsigned char>(0xFF & val));
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


void GenericEncoder::from_basic_general(CopiedValue &val) {
    std::cout << "general basic entry" << std::endl;
    if (val.type == ValueType::BOOLEAN) {
        bool temp = std::get<bool>(val.data);
        this->from_bool(temp);
    } else if (val.type == ValueType::INT) {
        int temp = std::get<int>(val.data);
        this->from_int(temp);
    } else if (val.type == ValueType::STRING) {
        std::string temp = std::get<std::string>(val.data);
        this->from_string(temp);
    } else if (val.type == ValueType::DOUBLE) {
        double temp = std::get<double>(val.data);
        this->from_double(temp);
    } else if (val.type == ValueType::SEMAPHORE) {
        // todo implement sem
    } else if (val.type == ValueType::REFERENCE) {
        // todo implement ref
    } else {
        // todo think about error management
    }
}

void GenericEncoder::from_general(CopiedValue &val) {
    std::cout << "general normal entry" << std::endl;
    val.print();
    unsigned char info{};
    if (val.type == ValueType::MAP) {
        std::cout << "en Map" << std::endl;
        this->from_byte(info_block::map_bit);
        auto &temp = std::get<CopiedValue::copied_map_t>(val.data);
        this->from_expanding_unsigned(temp.size()); // add key length
        for (auto& [key, value] : temp) {
            // add keys and values
            this->from_string(key);
            this->from_general(*value);
        }
    } else if (val.type == ValueType::ARRAY) {
        std::cout << "en Array" << std::endl;
        info = info_block::array_bit;
        auto &temp = std::get<CopiedValue::copied_array_t>(val.data);
        auto type = ValueType::UNKNOWN;
        bool first = true;
        for (auto &x : temp) { // check if all values in array have the same type
            if (first) type = x.type;
            first = false;
            if (type != x.type) {
                type = ValueType::UNKNOWN;
                break;
            }
        }
        if (info_block::get_basic_byte(type) > 0) {
            info |= info_block::get_basic_byte(type); // if all have same basic type, add it to info
        } else {
            info |= info_block::complex_bit;
            type = ValueType::UNKNOWN; // replace possible map, pair, or array types
        }
        this->from_byte(info);
        this->from_expanding_unsigned(temp.size()); // add array size
        int i = 0;
        for (auto &x : temp) { // add values
            std::cout << "arrayloop: " << i++ << " - " << (int)info_block::get_basic_byte(type) << " - " << (type == ValueType::UNKNOWN) << std::endl;
            if (type == ValueType::UNKNOWN) this->from_general(x);
            else this->from_basic_general(x);
        }
    } else if (val.type == ValueType::PAIR) {
        std::cout << "en Pair" << std::endl;
        info = info_block::pair_bit;
        auto &temp = std::get<CopiedValue::copied_pair_t>(val.data);
        if (int basic_byte = info_block::get_basic_byte(temp.first->type); basic_byte > 0) {
            // first type is simple
            info |= basic_byte; // add basic type for first element to type
            this->from_byte(info);
            this->from_basic_general(*temp.first);
        } else {
            info |= info_block::complex_bit;
            this->from_byte(info);
            this->from_general(*temp.first);
        }
        this->from_general(*temp.second);
    } else {
        info = info_block::get_basic_byte(val.type);
        std::cout << "basic: " << (int) info << std::endl;
        this->from_byte(info);
        this->from_basic_general(val);
    }
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

