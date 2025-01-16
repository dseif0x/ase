/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include "tcp2_utils.h"
#include "tcp2_coding.h"
#include "encoder.h"

#include <fstream>


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
    if (val.length() != 0) this->add_to_buffer(data);
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
    this->from_byte(*reinterpret_cast<unsigned char*>(&val));
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
    if (val == 0) data.push_back(0x00); // if empty, add one empty byte
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
                    data.push_back(face);
                    data.push_back(cur);
                }
                add_all_now = true;
            }
            face <<= 1;
        }
    }
    this->add_to_buffer(data);
}

bool GenericEncoder::from_general(CopiedValue &val) {
    try {
        this->from_complex_general(val);
        return true;
    } catch (const std::exception& e) {
        // Catch standard exceptions derived from std::exception
        std::cerr << "Caught exception: " << e.what() << '\n';
        return false;
    } catch (...) {
        // error most likely because of std::get getting a value with incompatible type
        return false;
    }
}

void GenericEncoder::from_list_t(DB::list_t &val) {
    unsigned char info{};
    info |= info_block::map_bit;
    this->from_byte(info);
    if (!val) {
        this->from_expanding_unsigned(0);
    } else {
        this->from_expanding_unsigned(val->size());
        for (const auto& [key, value] : *val) {
            this->from_string(key);
            unsigned char val_type = info_block::get_complex_byte(value.first);

            if (std::holds_alternative<std::monostate>(value.second)) val_type |= info_block::type_bit;
            this->from_byte(val_type);

            if (std::holds_alternative<bool>(value.second)) this->from_bool(std::get<bool>(value.second));
            else if (std::holds_alternative<int>(value.second)) this->from_int(std::get<int>(value.second));
            else if (std::holds_alternative<std::string>(value.second)) this->from_string(std::get<std::string>(value.second));
            else if (std::holds_alternative<double>(value.second)) this->from_double(std::get<double>(value.second));
            // add other types here if allowed by list_t type

        }
    }
}


void GenericEncoder::from_basic_general(CopiedValue &val) {
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
        int temp = std::get<int>(val.data);
        this->from_int(temp);
    } else if (val.type == ValueType::REFERENCE) {
        std::string temp = std::get<std::string>(val.data);
        this->from_string(temp);
    } else {
        throw std::invalid_argument("Invalid datatype");
    }
}

void GenericEncoder::from_complex_general(CopiedValue &val) {
    unsigned char info{};
    if (val.type == ValueType::MAP) {
        this->from_byte(info_block::map_bit);
        auto &temp = std::get<CopiedValue::copied_map_t>(val.data);
        this->from_expanding_unsigned(temp.size()); // add key length
        for (auto& [key, value] : temp) {
            // add keys and values
            this->from_string(key);
            this->from_general(*value);
        }
    } else if (val.type == ValueType::ARRAY) {
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
            if (type == ValueType::UNKNOWN) this->from_general(x);
            else this->from_basic_general(x);
        }
    } else if (val.type == ValueType::PAIR) {
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

FileEncoder::FileEncoder(std::string &file_path): GenericEncoder(), file_path(file_path) {
}

bool FileEncoder::write_data() {
    std::ofstream file(file_path, std::ios::binary);

    if (!file) {
        throw std::ios_base::failure("Error: Unable to open file for writing.");
    }

    int block_size = 64;
    while (!this->is_empty()) {
        auto data = this->get_bytes(block_size);
        file.write(reinterpret_cast<const char*>(data.first.get()), data.second);
        if (!file) {
            throw std::ios_base::failure("Error: Failed to write array data to file.");
        }
    }

    file.close();
    if (!file.good()) {
        throw std::ios_base::failure("Error: File stream encountered an error on close.");
    }
    return true;
}

