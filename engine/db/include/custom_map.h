/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <cstdint>
#include <utility>
#include "shrwlock.h"
#include "shmem.h"

template <typename T>
class db_map {
public:
    using map_entry_type = std::pair<offset_ptr<char>, T>;
    db_map(shmem& mem) : mem{mem} {
        data = mem.construct_array_unsafe<map_entry_type>(16);
        used_size = 0;
        alloc_size = 16;
    }
    ~db_map() {
        mem.destroy_array(data);
    }
    bool has(const char* key) { // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
        std::pair<offset_ptr<char>, T>* data = this->data.get(mem);
        for (size_t i = 0; i < used_size; i++) {
            if (std::strcmp(data[i].first.get(mem), key) == 0) {
                return true;
            }
        }
        return false;
    }

    T& get(const char* key) { // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
        std::pair<offset_ptr<char>, T>* data = this->data.get(mem);
        for (size_t i = 0; i < used_size; i++) {
            if (std::strcmp(data[i].first.get(mem), key) == 0) {
                return data[i].second;
            }
        }
        throw std::runtime_error("Key not found");
    }
    void set(const char* key, T value) { // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
        std::pair<offset_ptr<char>, T>* data = this->data.get(mem);
        for (size_t i = 0; i < used_size; i++) {
            if (std::strcmp(data[i].first.get(mem), key) == 0) {
                data[i].second = value;
                return;
            }
        }
        // Not found, add new
        if (used_size == alloc_size) {
            resize();
        }
        size_t str_len = std::strlen(key);
        offset_ptr<char> new_key = mem.construct_array_unsafe<char>(str_len);
        strncpy(new_key.get(mem), key, str_len);
        data[used_size++] = std::make_pair(new_key, value);
    }

private:
    shmem& mem;
    void resize() {
        auto new_data = mem.construct_array_unsafe<map_entry_type>(alloc_size * 2);
        memcpy(new_data.get(mem), data.get(mem), used_size * sizeof(map_entry_type));
        mem.destroy_array(data);
        data = new_data;
        alloc_size *= 2;
    }

    offset_ptr<map_entry_type> data{-1};
    size_t used_size;
    size_t alloc_size;
    shrwlock lock;
};