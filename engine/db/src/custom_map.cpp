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
    mem.destroy_array(lookup);
    mem.destroy_array(data);
}

//! @onlyInAtomicAccess
std::vector<std::string> db_map::atomic_keys() {
    std::shared_lock<shrwlock> lock_guard(lock);
    return keys();
}

//! @onlyInAtomicAccess
std::vector<std::string> db_map::keys() {
    std::vector<std::string> keys;
    map_entry_type* data = this->data.get(mem);
    for (size_t i = 0; i < used_size; i++) {
        keys.push_back(std::get<0>(data[i]).get(mem));
    }
    return keys;
}

//! @onlyInAtomicAccess
bool db_map::atomic_has(const char* key) { // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
    std::shared_lock<shrwlock> lock_guard(lock);
    return has(key);
}

//! @onlyInAtomicAccess
relative_offset_ptr<Value> db_map::atomic_get(const char* key) {
    std::shared_lock<shrwlock> lock_guard(lock);
    return get(key);
}

//! @onlyInAtomicAccess
bool db_map::has(const char* key) {
    if (this->alloc_size == 0) {
        return false;
    }
    offset_t removed_idx = hash_fn(std::string(key)) % this->alloc_size;
    auto* lookup = this->lookup.get(mem);
    return lookup[removed_idx] >= 0;
}

//! @onlyInAtomicAccess
relative_offset_ptr<Value> db_map::get(const char* key) {
    map_entry_type* data = this->data.get(mem);
    offset_t* lookup = this->lookup.get(mem);
    size_t hash = db_map::hash_fn(std::string(key));
    auto idx = lookup[hash % this->alloc_size];
    if (idx >= 0) {
        // TODO check for collision
        return std::get<1>(data[idx]).get_relative(mem);
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