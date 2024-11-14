/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
#include "db_types.h"

CopiedValue::CopiedValue(const Value &value) {
    type = value.type;
    if (type == ValueType::INT) {
        int_value = value.int_value;
    } else if (type == ValueType::BOOLEAN) {
        bool_value = value.bool_value;
    }  else if (type == ValueType::DOUBLE) {
        double_value = value.double_value;
    } /* TODO else if (type == ValueType::ARRAY) {
        array_value = {value.array_value.type, value.array_value.array_value.get()};
    } else if (type == ValueType::PAIR) {
        pair_value = {value.pair_value.first.get(), value.pair_value.second.get()};
    }*/ else if (type == ValueType::MAP) {
        map_value = value.map_value; // TODO possibly deep copy
    }/* else if (type == ValueType::STRING) {
        string_value = value.string_value.get();
    }*/
}

Value::Value(shmem& mem, const CopiedValue &value) {
    type = value.type;
    if (type == ValueType::INT) {
        int_value = value.int_value;
    } else if (type == ValueType::BOOLEAN) {
        bool_value = value.bool_value;
    }  else if (type == ValueType::DOUBLE) {
        double_value = value.double_value;
    } /* TODO else if (type == ValueType::ARRAY) {
        array_value = {value.array_value.type, mem.construct_unsafe<Value>(value.array_value.array_value), value.array_value.size};
    } else if (type == ValueType::PAIR) {
        pair_value = {mem.construct_unsafe<Value>(value.pair_value.first), mem.construct_unsafe<Value>(value.pair_value.second)};
    } else if (type == ValueType::MAP) {
        map_value = mem.construct_unsafe<db_map<Value>>(mem);
    } else if (type == ValueType::STRING) {
        string_value = mem.construct_unsafe<char>(value.string_value);
    }*/
}