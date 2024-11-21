/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <utility>
#include <shared_mutex>
#include <cstring>
#include "shrwlock.h"
#include "shmem.h"
#include "lock_utils.h"
#include "db_types.h"

#include <cstring>

class locked_db_map {
    std::shared_lock<shrwlock> lock;
    db_map* map;
public:
    locked_db_map(db_map* map);
    relative_offset_ptr<Value> get(const char* key);
    bool has(const char* key);
};

/**
 * @brief A database-like key-value store backed by shared memory.
 *
 * This template class provides a simple in-memory key-value store with
 * support for atomic access to stored values. It allows checking for keys,
 * retrieving values, and inserting new key-value pairs into the map. The map
 * operates in shared memory and supports concurrent access through a read-write lock.
 *
 */
class db_map {
public:
    using map_entry_type = std::pair<offset_ptr<char>, offset_ptr<Value>>;

    db_map(shmem& mem);

    ~db_map();

    //! @onlyInAtomicAccess
    std::vector<std::string> keys();

    //! @onlyInAtomicAccess
    bool atomic_has(const char* key); // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK


    relative_offset_ptr<Value> atomic_get(const char* key); // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK

    template<typename Lambda>
    void transaction(Lambda &&cb) {
        static_assert(std::is_invocable_v<Lambda, locked_db_map*>, "Lambda must be invocable with locked_db_map*");
        auto locked_map = locked_db_map{this};
        cb(&locked_map);
    }

    static void set(offset_ptr<db_map>& map, shmem& mem, const char* key, offset_ptr<Value> value) { // NEVER CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK (it will deadlock)
        bool found = false;
        size_t used_size;
        size_t alloc_size;
        offset_ptr<Value> old_value;
        mem.get_meta_mutex().lock_shared();
        map.get(mem)->lock.lock();

        map.atomic_access(mem, [&](db_map* map) {
            std::pair<offset_ptr<char>, offset_ptr<Value>>* data = map->data.get(mem);
            for (size_t i = 0; i < map->used_size; i++) {
                if (std::strcmp(data[i].first.get(mem), key) == 0) {
                    old_value = data[i].second;
                    data[i].second = value;
                    found = true;
                    map->lock.unlock();
                    return;
                }
            }
            used_size = map->used_size;
            alloc_size = map->alloc_size;
        }, true);
        mem.get_meta_mutex().unlock_shared();

        if (found) {
            Value::destroy_value(mem, old_value);
            return;
        }

        // Not found, add new
        if (used_size == alloc_size) {
            resize(map, mem);
        }
        size_t str_len = std::strlen(key);
        offset_ptr<char> new_key = mem.construct_array<char>(str_len + 1);
        new_key.atomic_access(mem, [&](char* ptr) {
            auto* map_p = map.get(mem);
            auto* data_p = map_p->data.get(mem);
            strncpy(ptr, key, str_len);
            ptr[str_len] = '\0';
            data_p[used_size++] = std::make_pair(new_key, value);
            map_p->used_size = used_size;
            map_p->lock.unlock();
        });
    }

private:
    friend class locked_db_map;
    bool has(const char* key); // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK

    relative_offset_ptr<Value> get(const char* key); // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK

    shmem& mem;
    static void resize(offset_ptr<db_map>& map, shmem& mem) {
        size_t alloc_size;
        offset_ptr<map_entry_type> old_data;
        map.atomic_access(mem, [&](db_map* map) {
            alloc_size = map->alloc_size;
            old_data = map->data;
        });
        size_t new_size = std::max(16ul, alloc_size) * 2;
        auto new_data = mem.construct_array<map_entry_type>(new_size);
        if (alloc_size > 0) {
            map.atomic_access(mem, [&](db_map *map) {
                memcpy(new_data.get(mem), old_data.get(mem), alloc_size * sizeof(map_entry_type));
            });
            mem.destroy_array(old_data);
        }
        map.atomic_access(mem, [&](db_map* map) {
            map->data = new_data;
            map->alloc_size = new_size;
        });
    }

    offset_ptr<map_entry_type> data{-1};
    size_t used_size;
    size_t alloc_size;
    shrwlock lock;
};