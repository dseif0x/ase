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
        data.emplace<int>(get<int>(value.data));
    } else if (type == ValueType::BOOLEAN) {
        data.emplace<bool>(get<bool>(value.data));
    }  else if (type == ValueType::DOUBLE) {
        data.emplace<double>(get<double>(value.data));
    } else if (type == ValueType::ARRAY) {
        auto& array = get<ArrayType>(value.data);
        auto& local_array = data.emplace<CopiedArrayType>();
        local_array.type = array.type;
        local_array.size = array.size;
        local_array.array_value = static_cast<CopiedValue *>(operator new[](array.size * sizeof(CopiedValue)));
        for (size_t i = 0; i < array.size; i++) {
            new(&local_array.array_value[i]) CopiedValue((array.array_value.get())[i]); // Construct CopiedValue in place using the Value in the shmem array
        }
    } else if (type == ValueType::PAIR) {
        auto& pair = get<Value::shmem_pair_t>(value.data);
        data.emplace<CopiedValue::copied_pair_t>() = {std::make_unique<CopiedValue>(*pair.first), std::make_unique<CopiedValue>(*pair.second)};
    } else if (type == ValueType::MAP) {
        auto& map = get<Value::shmem_map_t>(value.data);
        auto& local_map = data.emplace<CopiedValue::copied_map_t>();
        for (auto& key : map->keys()) {
            auto& val = *map->get(key.c_str());
            local_map.emplace(key, std::make_unique<CopiedValue>(val));
        }
    } else if (type == ValueType::STRING || type == ValueType::REFERENCE) {
        data.emplace<std::string>(get<Value::shmem_string_t>(value.data).get());
    } else if (type == ValueType::SEMAPHORE) {
        // TODO
    }
}

Value::Value(shmem& mem, const CopiedValue &value) {
    type = value.type;
    if (type == ValueType::INT) {
        data.emplace<int>(get<int>(value.data));
    } else if (type == ValueType::BOOLEAN) {
        data.emplace<bool>(get<bool>(value.data));
    }  else if (type == ValueType::DOUBLE) {
        data.emplace<double>(get<double>(value.data));
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