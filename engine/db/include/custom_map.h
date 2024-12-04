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

/**
 * @brief A smart-lock wrapper for @ref db_map.
 *
 * This class exposes some private methods of db_map, that are unsafe to be used without holding a lock.
 * It's safe to use those methods through locked_db_map as this class handles lock management automatically.
 */
class locked_db_map {
    std::shared_lock<shrwlock> lock;
    db_map* map;
public:
    locked_db_map(db_map* map);

    /**
     * Exposes @ref db_map::get
     *
     * @param key bottom level key to retrieve
     * @return the Value corresponding to the key
     */
    relative_offset_ptr<Value> get(const char* key);

    /**
     * Exposes @ref db_map::has
     *
     * @param key bottom level key to check for
     * @return rather a value corresponding to the given key exists
     */
    bool has(const char* key);

    /**
     * Exposes @ref db_map::keys
     *
     * @return a list of keys
     */
    std::vector<std::string> keys();
};

/**
 * @brief A database-like key-value store backed by shared memory.
 *
 * This template class provides a simple in-memory key-value store with
 * support for atomic access to stored values. It allows checking for keys,
 * retrieving values, and inserting new key-value pairs into the map. The map
 * operates in shared memory and supports concurrent access through a read-write lock.
 * A single object of this type represents a node in the tree-structure of the DB.
 */
class db_map {
public:
    using map_entry_type = std::pair<offset_ptr<char>, offset_ptr<Value>>;

    db_map(shmem& mem);

    ~db_map();

    /**
     * atomic version of @ref db_map::keys
     *
     * @onlyInAtomicAccess
     */
    std::vector<std::string> atomic_keys();

    /**
     * atomic version of @ref db_map::has
     *
     * @onlyInAtomicAccess
     */
    bool atomic_has(const char* key);

    /**
     * atomic version of @ref db_map::get
     *
     * @onlyInAtomicAccess
     */
    relative_offset_ptr<Value> atomic_get(const char* key);


    /**
     * This function aquires a lock for the map and then calls a callback. The callback function is the only place
     * @ref locked_db_map::get and @ref locked_db_map::has can be called from.
     *
     * @param cb c Callable function that takes a @ref locked_db_map pointer as a parameter.
     */
    template<typename Lambda>
    void transaction(Lambda &&cb) {
        static_assert(std::is_invocable_v<Lambda, locked_db_map*>, "Lambda must be invocable with locked_db_map*");
        auto locked_map = locked_db_map{this};
        cb(&locked_map);
    }

    /**
     * This function inserts a value into the shmem.
     *
     * @param map the node the value should be attached to
     * @param mem a reference to the shmem
     * @param key the bottom level key
     * @param value the value to insert
     * @neverInAtomicAccess
     */
    static void set(offset_ptr<db_map>& map, shmem& mem, const char* key, offset_ptr<Value> value) {
        bool found = false;
        size_t used_size;
        size_t alloc_size;
        offset_ptr<Value> old_value;
        std::unique_lock<shrwlock> lock_guard(mem.get_meta_mutex());

        map.atomic_access(mem, [&](db_map* map) {
            map->lock.lock();
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

        if (found) {
            Value::destroy_value(mem, old_value, true);
            return;
        }

        // Not found, add new
        if (used_size == alloc_size) {
            resize(map, mem);
        }
        size_t str_len = std::strlen(key);
        offset_ptr<char> new_key = mem.construct_array_unsafe<char>(str_len + 1);
        new_key.atomic_access(mem, [&](char* ptr) {
            auto* map_p = map.get(mem);
            auto* data_p = map_p->data.get(mem);
            strncpy(ptr, key, str_len);
            ptr[str_len] = '\0';
            data_p[used_size++] = std::make_pair(new_key, value);
            map_p->used_size = used_size;
            map_p->lock.unlock();
        }, true);
    }

    /**
     * Removes a value from the shmem
     *
     * @param map the node the value is attached to
     * @param mem the shmem
     * @param key the bottom level key to delete
     * @neverInAtomicAccess
     */
    static void remove(offset_ptr<db_map>& map, shmem& mem, const char* key) {
        // Potentially shrink the shmem
        map.atomic_access(mem, [&](db_map* map) {
            map->lock.lock();
            std::pair<offset_ptr<char>, offset_ptr<Value>>* data = map->data.get(mem);
            for (size_t i = 0; i < map->used_size; i++) {
                if (std::strcmp(data[i].first.get(mem), key) == 0) {
                    map->used_size--; // we don't delete anything, just invalidate it's memory
                    if (i < map->used_size) { // swap values with last data entry
                        std::swap(data[i], data[map->used_size]);
                        break;
                    }
                }
            }
            map->lock.unlock();
        });
    }

private:
    friend class locked_db_map;

    /**
     * Returns the keys directly following this node.
     * @return list of keys
     * @onlyInAtomicAccess
     */
    std::vector<std::string> keys();

    /**
     * Checks rather a key is contained in the map. This does not search recursively.
     * @param key the database key that should be looked up
     * @return indicates if the map contains such a key
     * @onlyInAtomicAccess
     */
    bool has(const char* key);

    /**
     * Returns the @ref Value to the corresponding key. Does not search recursively.
     * @param key the database key that should be looked up
     * @return returns the Value corresponding to the key
     * @onlyInAtomicAccess
     */
    relative_offset_ptr<Value> get(const char* key);

    shmem& mem;
    /**
     * resizes the space that is allocated for the given node.
     * @param map the node that grows
     * @param mem the shmem
     * @neverInAtomicAccess
     */
    static void resize(offset_ptr<db_map>& map, shmem& mem) {
        size_t alloc_size;
        offset_ptr<map_entry_type> old_data;
        map.atomic_access(mem, [&](db_map* map) {
            alloc_size = map->alloc_size;
            old_data = map->data;
        }, true);
        size_t new_size = std::max(16ul, alloc_size) * 2;
        auto new_data = mem.construct_array_unsafe<map_entry_type>(new_size);
        if (alloc_size > 0) {
            map.atomic_access(mem, [&](db_map *map) {
                memcpy(new_data.get(mem), old_data.get(mem), alloc_size * sizeof(map_entry_type));
            }, true);
            mem.destroy_array(old_data, true);
        }
        map.atomic_access(mem, [&](db_map* map) {
            map->data = new_data;
            map->alloc_size = new_size;
        }, true);
    }

    offset_ptr<map_entry_type> data{-1};
    size_t used_size;
    size_t alloc_size;
    shrwlock lock;
};