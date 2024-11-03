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
#include "offset_ptr.h"

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

struct Value;
struct ArrayType {
    ValueType type;
    offset_ptr<Value> array_value;
    size_t size;
};

struct Value {
    ValueType type;
    union {
        bool bool_value;
        int int_value;
        offset_ptr<char> string_value; // For now this will also be used for storing references
        double double_value;
        // TODO semaphore sem_value;
        ArrayType array_value;
        std::pair<offset_ptr<Value>, offset_ptr<Value>> pair_value;
        db_map<Value> map_value;
    };
};