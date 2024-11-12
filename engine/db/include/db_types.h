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

enum class ValueType {
    BOOLEAN,
    INT,
    STRING,
    DOUBLE,
    SEMAPHORE,
    REFERENCE,
    ARRAY,
    PAIR,
    MAP // This is for hierarchical data structures
};

struct ArrayType {
    ValueType type;
    relative_offset_ptr<Value> array_value;
    size_t size;
};

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

struct CopiedArrayType {
    ValueType type;
    CopiedValue* array_value;
    size_t size;
};

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
        // TODO db_map<Value> map_value;
    };
};