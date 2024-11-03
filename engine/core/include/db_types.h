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

template <typename T>
class TreeMap {
    T& get(const char* key);
    void set(const char* key, T value);

private:
    std::pair<const char*, T>* data;
    size_t used_size;
    size_t alloc_size;
    shrwlock lock;
};

enum class ValueType {
    INT,
    FLOAT,
    STRING,
    ARRAY,
    MAP
};

struct Value;
struct ArrayType {
    Value* array_value;
    size_t size;
};

struct Value {
    ValueType type;
    union {
        int int_value;
        float float_value;
        char* string_value;
        ArrayType array_value;
        TreeMap<Value> map_value;
    };
};

struct DB {
    int db_ver;
    char* metadata;
    TreeMap<Value> entrypoint;

};