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
#include "custom_map.h"
#include "relative_offset_ptr.h"

struct Value;
struct CopiedValue;

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
    MAP //!< This is for hierarchical data structures
};

/**
 * @brief Represents an array type in the database.
 *
 * This structure holds an array of values, the type of values in the array,
 * and the size of the array.
 */
struct ArrayType {
    ValueType type;
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
    Value() = delete;
    Value(shmem& mem, const CopiedValue& value);

    ValueType type;
    union {
        bool bool_value;
        int int_value;
        relative_offset_ptr<char> string_value{}; // For now this will also be used for storing references
        double double_value;
        // TODO semaphore sem_value;
        ArrayType array_value;
        std::pair<relative_offset_ptr<Value>, relative_offset_ptr<Value>> pair_value;
        db_map<Value>* map_value;
    };
};


// These types are for returning values from the database as we cannot return a view we need to copy data
/**
 * @brief Represents a copied array type for returning from the database.
 *
 * This structure holds an array of copied values, their type, and the size of the array.
 */
struct CopiedArrayType {
    ValueType type;
    CopiedValue* array_value;
    size_t size;
};

/**
 * @brief Represents a copied version of a value for returning from the database.
 *
 * This structure is used to return a copy of a value from the database, as views are not supported.
 */
struct CopiedValue {
    CopiedValue() = default;
    CopiedValue(const Value& value);
    ValueType type;
    union {
        bool bool_value;
        int int_value;
        const char* string_value; // For now this will also be used for storing references
        double double_value;
        // TODO semaphore sem_value;
        CopiedArrayType array_value;
        // TODO std::pair<CopiedValue, CopiedValue> pair_value;
        // TODO
        db_map<Value>* map_value;
    };
};