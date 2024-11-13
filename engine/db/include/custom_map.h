/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <cstdint>
#include <utility>
#include <shared_mutex>
#include "shrwlock.h"
#include "shmem.h"
#include "lock_utils.h"

template <typename T>
class db_map {
public:
    using map_entry_type = std::pair<offset_ptr<char>, T>;
    db_map(shmem& mem) : mem{mem} {
        used_size = 0;
        alloc_size = 0;
    }
    ~db_map() {
        mem.destroy_array(data);
    }
    bool has(const char* key) { // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
        std::shared_lock<shrwlock> lock_guard(lock);
        std::pair<offset_ptr<char>, T>* data = this->data.get(mem);
        for (size_t i = 0; i < used_size; i++) {
            if (std::strcmp(data[i].first.get(mem), key) == 0) {
                return true;
            }
        }
        return false;
    }

    T& get(const char* key) { // ONLY CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK
        std::shared_lock<shrwlock> lock_guard(lock);
        std::pair<offset_ptr<char>, T>* data = this->data.get(mem);
        for (size_t i = 0; i < used_size; i++) {
            if (std::strcmp(data[i].first.get(mem), key) == 0) {
                return data[i].second;
            }
        }
        throw std::runtime_error("Key not found");
    }
    static void set(offset_ptr<db_map>& map, shmem& mem, const char* key, T value) { // NEVER CALL THIS FROM WITHIN AN ATOMIC_ACCESS BLOCK (it will deadlock)
        bool found = false;
        size_t used_size;
        size_t alloc_size;
        shrwlock& memlock = mem.get_meta_mutex();
        shrwlock maplock;
        map.atomic_access(mem, [&](db_map* map) {
            maplock = map->lock;
        });
        shared_wrapped_mutex m(memlock);
        std::lock(m, maplock);
        map.atomic_access(mem, [&](db_map* map) {
            std::pair<offset_ptr<char>, T>* data = map->data.get(mem);
            for (size_t i = 0; i < map->used_size; i++) {
                if (std::strcmp(data[i].first.get(mem), key) == 0) {
                    data[i].second = value;
                    found = true;
                    map->lock.unlock();
                    return;
                }
            }
            used_size = map->used_size;
            alloc_size = map->alloc_size;
        }, true);
        m.unlock();

        if (found) return;

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