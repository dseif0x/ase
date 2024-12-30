/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <utility>
#include <shared_mutex>
#include <iostream>
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
    db_map *map;
public:
    locked_db_map(db_map *map);

    /**
     * Exposes @ref db_map::get
     *
     * @param key bottom level key to retrieve
     * @return the Value corresponding to the key
     */
    relative_offset_ptr<Value> get(shmem& mem, const char *key);

    /**
     * Exposes @ref db_map::has
     *
     * @param key bottom level key to check for
     * @return rather a value corresponding to the given key exists
     */
    bool has(shmem& mem, const char *key);

    /**
     * Exposes @ref db_map::keys
     *
     * @return a list of keys
     */
    std::vector<std::string> keys(shmem& mem);
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
    using map_entry_type = std::tuple<offset_ptr<char>, offset_ptr<Value>, offset_t>;

    static inline std::hash<std::string> hash_fn;

    db_map();

    ~db_map();

    shrwlock& get_lock() {
        return lock;
    }

    /**
     * atomic version of @ref db_map::keys
     *
     * @onlyInAtomicAccess
     */
    std::vector<std::string> atomic_keys(shmem& mem);

    /**
     * atomic version of @ref db_map::has
     *
     * @onlyInAtomicAccess
     */
    bool atomic_has(shmem& mem, const char *key);

    /**
     * atomic version of @ref db_map::get
     *
     * @onlyInAtomicAccess
     */
    relative_offset_ptr<Value> atomic_get(shmem& mem, const char *key);


    /**
     * This function aquires a lock for the map and then calls a callback. The callback function is the only place
     * @ref locked_db_map::get and @ref locked_db_map::has can be called from.
     *
     * @param cb c Callable function that takes a @ref locked_db_map pointer as a parameter.
     */
    template<typename Lambda>
    void transaction(Lambda &&cb) {
        static_assert(std::is_invocable_v<Lambda, locked_db_map *>, "Lambda must be invocable with locked_db_map*");
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
    static void set(offset_ptr<db_map> &map, shmem &mem, const char *key, offset_ptr<Value> value) {
        bool found = false;
        size_t used_size;
        size_t alloc_size;
        auto hash = hash_fn(std::string(key));

        map.atomic_access(mem, [&](db_map *map) {
            map_entry_type *data = map->data.get(mem);
            offset_t *lookup = map->lookup.get(mem);
            used_size = map->used_size;
            alloc_size = map->alloc_size;
            if (alloc_size == 0) return;
            auto idx = lookup[hash % alloc_size];
            if (idx >= 0) {
                auto ll_node = data[idx];
                while (strcmp(std::get<0>(ll_node).get(mem), key) != 0) {
                    idx = std::get<2>(ll_node);
                    if (idx < 0)
                        break;
                    ll_node = data[idx];
                }
                if (strcmp(std::get<0>(ll_node).get(mem), key) == 0) {
                    found = true;
                    Value::destroy_value(mem, std::get<1>(ll_node), true);
                    std::get<1>(data[idx]) = value;
                }
            }
        }, true);

        if (found) {
            return;
        }

        // Not found, add new
        if (used_size == alloc_size) {
            resize(map, mem);
        }
        size_t str_len = std::strlen(key);
        offset_ptr<char> new_key = mem.construct_array_unsafe<char>(str_len + 1);
        new_key.atomic_access(mem, [&](char *ptr) {
            auto *map_p = map.get(mem);
            auto *data_p = map_p->data.get(mem);
            auto *lookup_p = map_p->lookup.get(mem);
            strncpy(ptr, key, str_len);
            ptr[str_len] = '\0';
            auto next = lookup_p[hash % map_p->alloc_size];
            lookup_p[hash % map_p->alloc_size] = used_size;
            data_p[used_size++] = std::make_tuple(new_key, value, next);
            map_p->used_size = used_size;
        }, true);
    }

    void print_data(shmem& mem) {
        map_entry_type* data = this->data.get(mem);
        for (size_t i = 0; i < used_size; i++) {

            auto key = std::get<0>(data[i]).get(mem);
            auto value = std::get<1>(data[i]).get(mem);
            auto next = std::get<2>(data[i]);

            std::cout << key << " : " << value << " : " << (int)value->type << " : " << next << std::endl;
        }
    }

    /**
     * Removes a value from the shmem
     *
     * @param map the node the value is attached to
     * @param mem the shmem
     * @param key the bottom level key to delete
     * @neverInAtomicAccess
     */
    static void remove(offset_ptr<db_map> &map, shmem &mem, const char *key) {
        // Potentially shrink the shmem
        map.atomic_access(mem, [&](db_map *map) {
            map_entry_type *data = map->data.get(mem);
            auto *lookup = map->lookup.get(mem);
            offset_t removed_idx = hash_fn(std::string(key)) % map->alloc_size;

            auto pos = findPositionInLL(map, mem, removed_idx, key);
            if (pos.second < 0) // key not found
                return;
            auto& deleted_node = data[pos.second];
            if (pos.first < 0) { // no predecessor
                lookup[removed_idx] = std::get<2>(deleted_node);
            } else { // predecessor
                auto& predecessor_node = data[pos.first];
                std::get<2>(predecessor_node) = std::get<2>(deleted_node);
            }
            if (pos.second == --map->used_size) { // deleting the last element in data
                Value::destroy_value(mem, std::get<1>(deleted_node), true);
                mem.destroy_array(std::get<0>(deleted_node), 0, true);
            } else { // deleting a node in the middle
                auto& moved_node = data[map->used_size];
                auto moved_key = std::get<0>(moved_node).get(mem);
                auto moved_hash = hash_fn(moved_key) % map->alloc_size;
                auto moved_node_predecessor = findPositionInLL(map, mem, moved_hash, moved_key).first;
                std::swap(moved_node, deleted_node);
                if (moved_node_predecessor < 0) { // moved node had no predecessor
                    lookup[moved_hash] = pos.second;
                } else {
                    std::get<2>(data[moved_node_predecessor]) = pos.second;
                }
                Value::destroy_value(mem, std::get<1>(moved_node), true);
                mem.destroy_array(std::get<0>(moved_node), 0, true);
            }
        }, true);
    }

private:
    friend class locked_db_map;

    friend void Value::destroy_value(shmem &mem, offset_ptr<Value> value, bool locks_held);

public:
    /**
     * Returns the keys directly following this node.
     * @return list of keys
     * @onlyInAtomicAccess
     */
    std::vector<std::string> keys(shmem& mem);

    /**
     * Checks rather a key is contained in the map. This does not search recursively.
     * @param key the database key that should be looked up
     * @return indicates if the map contains such a key
     * @onlyInAtomicAccess
     */
    bool has(shmem& mem, const char *key);

    /**
     * Returns the @ref Value to the corresponding key. Does not search recursively.
     * @param key the database key that should be looked up
     * @return returns the Value corresponding to the key
     * @onlyInAtomicAccess
     */

    relative_offset_ptr<Value> get(shmem& mem, const char *key);

private:
    /**
     * Just a little herper function to find the position of a node and its predecessor
     * Should only be called when all locks (shmem and map) are acquired.
     *
     * @param map the node the value is attached to
     * @param mem the shmem
     * @param key the bottom level key to find
     * @return a pair containing the position of the predecessor and the node. returns -1 for any of those that don't exist.
     */
    static std::pair<offset_t, offset_t> findPositionInLL(db_map *map, shmem &mem, offset_t hash, const char *key) {
        hash %= map->alloc_size; // make sure it's not out of range
        std::pair<offset_t, offset_t> res{-1, -1};
        map_entry_type *data = map->data.get(mem);
        auto* lookup = map->lookup.get(mem);
        offset_t entrypoint = lookup[hash];
        if (entrypoint >= 0) { // we found an entrypoint for the keys hash
            auto ll_node = data[entrypoint];
            if (strcmp(key, std::get<0>(ll_node).get(mem)) == 0) { // the entrypoint was the element we were looking for
                res.second = entrypoint;
                return res;
            }
            while (true) {
                auto prev = entrypoint;
                auto curr = std::get<2>(ll_node);
                if (curr < 0) // we reached the end without finding the key
                    return res;
                ll_node = data[curr];
                if (strcmp(key, std::get<0>(ll_node).get(mem)) == 0) { // we found the key
                    res.first = prev;
                    res.second = curr;
                    break;
                }
                entrypoint = curr;
            }
        }
        return res;
    }

    /**
     * resizes the space that is allocated for the given node.
     * @param map the node that grows
     * @param mem the shmem
     * @neverInAtomicAccess
     */
    static void resize(offset_ptr<db_map> &map, shmem &mem) {
        size_t alloc_size;
        size_t used_size;
        offset_ptr<map_entry_type> old_data;
        offset_ptr<offset_t> old_lookup;
        map.atomic_access(mem, [&](db_map *map) {
            used_size = map->used_size;
            alloc_size = map->alloc_size;
            old_data = map->data;
            old_lookup = map->lookup;
        }, true);
        size_t new_size = std::max(16ul, alloc_size) * 2;
        auto new_lookup = mem.construct_array_unsafe<offset_t>(new_size);
        map.atomic_access(mem, [&](db_map *map) {
            std::memset(new_lookup.get(mem), -1, new_size * sizeof(offset_t));
        }, true);
        auto new_data = mem.construct_array_unsafe<map_entry_type>(new_size);
        if (used_size > 0) {
            map.atomic_access(mem, [&](db_map *map) {
                // We have to adjust for new hashes
                auto new_data_ptr = new_data.get(mem);
                auto new_lookup_ptr = new_lookup.get(mem);
                memcpy(new_data_ptr, old_data.get(mem), used_size * sizeof(map_entry_type));
                for (offset_t i = 0; i < used_size; i++) {
                    auto& ll_node = new_data_ptr[i];
                    auto key = std::get<0>(ll_node).get(mem);
                    auto hash = hash_fn(key) % new_size;
                    std::get<2>(ll_node) = new_lookup_ptr[hash];
                    new_lookup_ptr[hash] = i;
                }
            }, true);
            mem.destroy_array(old_data, alloc_size, true);
            mem.destroy_array(old_lookup, alloc_size, true);
        }
        map.atomic_access(mem, [&](db_map *map) {
            map->data = new_data;
            map->lookup = new_lookup;
            map->alloc_size = new_size;
        }, true);
    }

    offset_ptr<offset_t> lookup{-1};
    offset_ptr<map_entry_type> data{-1};
    size_t used_size;
    size_t alloc_size;
    shrwlock lock;
};