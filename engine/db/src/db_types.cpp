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
    } else {
        throw std::runtime_error("Unknown ValueType");
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
    } else if (type == ValueType::ARRAY) {
        auto& local_array = get<CopiedArrayType>(value.data);
        auto& array = data.emplace<ArrayType>();
        array.type = local_array.type;
        array.size = local_array.size;
        auto shmem_array_data = offset_ptr<Value>(mem.allocate_memory_unsafe(array.size * sizeof(Value)));
        Value* shmem_array_data_ptr = shmem_array_data.get(mem);
        for (size_t i = 0; i < array.size; i++) {
            new(shmem_array_data_ptr + i) Value(mem, local_array.array_value[i]);
        }
        array.array_value = shmem_array_data.get_relative(mem);
    } else if (type == ValueType::PAIR) {
        auto& local_pair = get<CopiedValue::copied_pair_t>(value.data);
        auto pair_v1 = mem.construct_unsafe<Value>(mem, *local_pair.first);
        auto pair_v2 = mem.construct_unsafe<Value>(mem, *local_pair.second);
        data.emplace<Value::shmem_pair_t>(pair_v1.get_relative(mem), pair_v2.get_relative(mem));
    } else if (type == ValueType::MAP) {
        auto shmem_map = mem.construct_unsafe<Value::shmem_map>(mem);
        auto& local_map = get<CopiedValue::copied_map_t>(value.data);
        for (auto& [key, val] : local_map) {
            auto shmem_value = mem.construct_unsafe<Value>(mem, *val);
            Value::shmem_map::set(shmem_map, mem, key.c_str(), shmem_value.get_relative(mem));
        }
        data.emplace<Value::shmem_map_t>(shmem_map.get_relative(mem));
    } else if (type == ValueType::STRING || type == ValueType::REFERENCE) {
        auto& local_string = get<std::string>(value.data);
        auto shmem_string = mem.construct_array_unsafe<char>(local_string.size());
        std::strcpy(shmem_string.get(mem), local_string.c_str());
        data.emplace<Value::shmem_string_t>(shmem_string.get_relative(mem));
    } else if (type == ValueType::SEMAPHORE) {
        // TODO
    } else {
        throw std::runtime_error("Unknown ValueType");
    }
}