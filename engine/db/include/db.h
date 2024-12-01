/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <optional>
#include "custom_map.h"
#include "db_types.h"
#include "shmem.h"

using db_key_map = db_map;

/**
 * @brief A database class that provides key-value storage with various commands.
 *
 * This class represents a simple key-value store with additional features like
 * locks and support for graph-like queries. It allows performing basic operations
 * such as getting, setting, removing keys, and handling locks, all within shared memory.
 */
class DB {
public:
    using list_t = std::optional<std::unordered_map<std::string, std::pair<ValueType, std::variant<std::monostate, bool, int, std::string, double>>>>;
    DB();
    ~DB();

    void destroy();

    // Core Commands

    /**
     * GET command of the database
     *
     * @param key the key to retrieve
     * @return a copy of the value
     */
    std::optional<CopiedValue> get(const char* key);

    /**
     * SET command of the database
     *
     * @param key the key to retrieve
     * @param value the value to be inserted
     */
    void set(const char* key, CopiedValue&& value);

    /**
     * FOLLOW command of the database
     *
     * @param key the key of the reference
     * @param max_chained_follows The maximum of jumps that should be done when following a chain of references
     * @return a copy of the value a references is pointing to
     */
    std::optional<CopiedValue> follow(const char* key, uint16_t max_chained_follows = 1);

    /**
     * LIST command of the database
     *
     * @param key the key of the node of which the sub keys should be listed
     * @return a list of first level sub keys in a map
     */
    list_t list(const char* key);

    /**
     * DELETE command of the database, will remove a key and its value from the database
     *
     * @param key the key of the value that should be removed
     */
    void remove(const char* key);

    // Semaphore Commands
    void create_lock(const char* key, uint16_t max_locks); //!< CREATE_LOCK command of the database, will create a lock on a key
    void acquire_lock(const char* key /*, uint16 timeout*/); //!< ACQUIRE_LOCK command of the database, will acquire a lock on a key
    void release_lock(const char* key); //!< RELEASE_LOCK command of the database, will release a lock on a key

    // Graph Commands
    // TODO void graph_query(const char* query_type, const char* start_key); // GRAPH_QUERY command of the database, will execute a graph query
private:
    offset_ptr<db_key_map> get_entrypoint();

    shmem mem;
};