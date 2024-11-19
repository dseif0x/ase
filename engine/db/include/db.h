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

using db_key_map = db_map<relative_offset_ptr<Value>>;

struct DB_Meta {
    db_key_map entrypoint;
    DB_Meta(shmem& mem) : entrypoint{mem} {}
};

/**
 * @brief A database class that provides key-value storage with various commands.
 *
 * This class represents a simple key-value store with additional features like
 * locks and support for graph-like queries. It allows performing basic operations
 * such as getting, setting, removing keys, and handling locks, all within shared memory.
 */
class DB {
public:
    DB();
    ~DB();

    void destroy();

    // Core Commands
    std::optional<CopiedValue> get(const char* key); //!< GET command of the database, will return a copy of the value
    void set(const char* key, CopiedValue value); //!< SET command of the database, will set a key to a value

    std::optional<CopiedValue> follow(const char* key); //!< FOLLOW command of the database, will return a copy of the value a references is pointing to
    std::vector<std::string> list(const char* key); //!< LIST command of the database, will return a list of first level sub keys in a map
    void remove(const char* key); //!< DELETE command of the database, will remove a key and its value from the database

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