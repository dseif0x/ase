/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "custom_map.h"

db_map::db_map(shmem& mem) : mem{mem} {
    used_size = 0;
    alloc_size = 0;
}

db_map::~db_map() {
    mem.destroy_array(data);
}

//! @onlyInAtomicAccess
std::vector<std::string> db_map::atomic_keys() {
    std::shared_lock<shrwlock> lock_guard(lock);
    std::vector<std::string> keys;
    std::pair<offset_ptr<char>, offset_ptr<Value>>* data = this->data.get(mem);
    for (size_t i = 0; i < used_size; i++) {
        keys.push_back(data[i].first.get(mem));
    }
    return keys;
}

//! @onlyInAtomicAccess
std::vector<std::string> db_map::keys() {
    std::vector<std::string> keys;
    std::pair<offset_ptr<char>, offset_ptr<Value>>* data = this->data.get(mem);
    for (size_t i = 0; i < used_size; i++) {
        keys.push_back(data[i].first.get(mem));
    }
    return keys;
}

//! @onlyInAtomicAccess
bool db_map::atomic_has(const char* key) { // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
    std::shared_lock<shrwlock> lock_guard(lock);
    std::pair<offset_ptr<char>, offset_ptr<Value>>* data = this->data.get(mem);
    for (size_t i = 0; i < used_size; i++) {
        if (std::strcmp(data[i].first.get(mem), key) == 0) {
            return true;
        }
    }
    return false;
}


relative_offset_ptr<Value> db_map::atomic_get(const char* key) { // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
    std::shared_lock<shrwlock> lock_guard(lock);
    std::pair<offset_ptr<char>, offset_ptr<Value>>* data = this->data.get(mem);
    for (size_t i = 0; i < used_size; i++) {
        if (std::strcmp(data[i].first.get(mem), key) == 0) {
            return data[i].second.get_relative(mem);
        }
    }
    throw std::runtime_error("Key not found");
}


bool db_map::has(const char* key) { // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
    std::pair<offset_ptr<char>, offset_ptr<Value>>* data = this->data.get(mem);
    for (size_t i = 0; i < used_size; i++) {
        if (std::strcmp(data[i].first.get(mem), key) == 0) {
            return true;
        }
    }
    return false;
}

relative_offset_ptr<Value> db_map::get(const char* key) { // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
    std::pair<offset_ptr<char>, offset_ptr<Value>>* data = this->data.get(mem);
    for (size_t i = 0; i < used_size; i++) {
        if (std::strcmp(data[i].first.get(mem), key) == 0) {
            return data[i].second.get_relative(mem);
        }
    }
    throw std::runtime_error("Key not found");
}

locked_db_map::locked_db_map(db_map* map) : lock(std::shared_lock<shrwlock>{map->lock}) {
    this->map = map;
}

relative_offset_ptr<Value> locked_db_map::get(const char* key) {
    return map->get(key);
}

bool locked_db_map::has(const char* key) {
    return map->has(key);
}

std::vector<std::string> locked_db_map::keys() {
    return map->keys();
}