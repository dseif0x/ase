/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
#pragma once

#include <cstdint>
#include <variant>
#include <memory>
#include <unordered_map>
#include <utility>
#include "shrwlock.h"
#include "relative_offset_ptr.h"
#include "offset_ptr.h"

struct Value;
struct CopiedValue;
class db_map;

/**
 * @brief Enumeration representing the types of values stored in the database.
 *
 * This enum defines the possible types of values that can be stored or returned
 * from the database, such as booleans, integers, strings, and hierarchical structures like maps.
 */
enum class ValueType {
    BOOLEAN,
    INT,
    STRING,
    DOUBLE,
    SEMAPHORE,
    REFERENCE,
    ARRAY,
    PAIR,
    MAP, //!< This is for hierarchical data structures
    UNKNOWN
};

/**
 * @brief Represents an array type in the database.
 *
 * This structure holds an array of values, the type of values in the array,
 * and the size of the array.
 */
struct ArrayType {
    relative_offset_ptr<Value> array_value;
    size_t size;
};

/**
 * @brief Represents a value in the database.
 *
 * This structure is a union that can store various types of values such as
 * booleans, integers, strings, arrays, pairs, or maps.
 */
struct Value {
    Value() = default;

    /**
     * @brief Construct a new Value object
     * @param mem The shared memory
     * @param value A CopiedValue& from which to get the data
     * Only allocate this object using construct_unsafe, as it recursively allocates other objects!
     * This also requires you to have a unique lock on the meta mutex before calling construct_unsafe
     */
    static offset_ptr<Value> create_value(shmem& mem, const CopiedValue& value);
    static void destroy_value(shmem& mem, offset_ptr<Value> value, bool locks_held = false);

    using shmem_pair_t = std::pair<relative_offset_ptr<Value>, relative_offset_ptr<Value>>;
    using shmem_map = db_map;
    using shmem_map_t = relative_offset_ptr<shmem_map>;
    using shmem_string_t = relative_offset_ptr<char>;

    ValueType type = ValueType::UNKNOWN;
    // TODO semaphore sem_value
    std::variant<bool, int, shmem_string_t, double, ArrayType, shmem_pair_t, shmem_map_t> data;
};

/**
 * @brief Represents a copied version of a value for returning from the database.
 *
 * This structure is used to return a copy of a value from the database, as views are not supported.
 */
struct CopiedValue {
    CopiedValue() = default;
    CopiedValue(CopiedValue&&) = default;
    //CopiedValue(const CopiedValue&) = default;
    ~CopiedValue() = default;

    CopiedValue& operator=(CopiedValue&&) = default;


    //! @onlyInAtomicAccess
    CopiedValue(const Value& value);

    void print();
    static void print(CopiedValue &val);

    using copied_map_t = std::unordered_map<std::string, std::unique_ptr<CopiedValue>>;
    using copied_pair_t = std::pair<std::unique_ptr<CopiedValue>, std::unique_ptr<CopiedValue>>;
    using copied_array_t = std::vector<CopiedValue>;

    ValueType type = ValueType::UNKNOWN;
    // TODO possibly semaphore sem_value but since we cannot copy this we will need to handle it differently
    std::variant<bool, int, std::string, double, copied_array_t, copied_map_t, copied_pair_t> data;
};
