/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
#include "custom_map.h"
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
        local_array.array_value = new CopiedValue[array.size];
        for (size_t i = 0; i < array.size; i++) {
            CopiedValue tmp(array.array_value.get()[i]);
            local_array.array_value[i] = std::move(tmp);
        }
    } else if (type == ValueType::PAIR) {
        auto& pair = get<Value::shmem_pair_t>(value.data);
        data.emplace<CopiedValue::copied_pair_t>() = {std::make_unique<CopiedValue>(*pair.first), std::make_unique<CopiedValue>(*pair.second)};
    } else if (type == ValueType::MAP) {
        auto& map = get<Value::shmem_map_t>(value.data);
        auto& local_map = data.emplace<CopiedValue::copied_map_t>();
        for (auto& key : map->keys()) {
            auto& val = *map->atomic_get(key.c_str());
            local_map.emplace(key, std::make_unique<CopiedValue>(val));
        }
    } else if (type == ValueType::STRING || type == ValueType::REFERENCE) {
        auto& str = get<Value::shmem_string_t>(value.data);
        data.emplace<std::string>(str.get());
    } else if (type == ValueType::SEMAPHORE) {
        // TODO
        throw std::runtime_error("Not Implemented");
    } else {
        throw std::runtime_error("Unknown ValueType");
    }
}

CopiedValue::~CopiedValue() {
    //if (type == ValueType::ARRAY) { // This is the only thing allocated on the heap
    //    auto& array = get<CopiedArrayType>(data);
    //    delete[] array.array_value;
    //}
}

offset_ptr<Value> Value::create_value(shmem &mem, const CopiedValue &value) {
    offset_ptr<Value> res = mem.construct<Value>();
    res.atomic_access(mem, [&](Value* new_value) {
        new_value->type = value.type;
    });

    if (value.type == ValueType::INT) {
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<int>(get<int>(value.data));
        });
    } else if (value.type == ValueType::BOOLEAN) {
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<bool>(get<bool>(value.data));
        });
    }  else if (value.type == ValueType::DOUBLE) {
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<double>(get<double>(value.data));
        });
    } else if (value.type == ValueType::ARRAY) {
        auto& local_array = get<CopiedArrayType>(value.data);
        auto shmem_array_data = mem.construct_array<Value>(local_array.size);

        std::vector<offset_ptr<Value>> shmem_array_data_ptrs;
        shmem_array_data_ptrs.reserve(local_array.size);
        for (size_t i = 0; i < local_array.size; i++) {
            shmem_array_data_ptrs.push_back(create_value(mem, local_array.array_value[i]));
        }

        res.atomic_access(mem, [&](Value* new_value) {
            auto& array = new_value->data.emplace<ArrayType>();
            array.type = local_array.type;
            array.size = local_array.size;
            array.array_value = shmem_array_data.get_relative(mem);

            Value* shmem_array_data_ptr = shmem_array_data.get(mem);
            for (size_t i = 0; i < array.size; i++) {
                shmem_array_data_ptr[i] = *shmem_array_data_ptrs[i].get(mem);
            }
        });
    } else if (value.type == ValueType::PAIR) {
        auto& local_pair = get<CopiedValue::copied_pair_t>(value.data);
        auto pair_v1 = create_value(mem, *local_pair.first);
        auto pair_v2 = create_value(mem, *local_pair.second);
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<Value::shmem_pair_t>(pair_v1.get_relative(mem), pair_v2.get_relative(mem));
        });
    } else if (value.type == ValueType::MAP) {
        auto shmem_map = mem.construct<Value::shmem_map>(mem);
        auto& local_map = get<CopiedValue::copied_map_t>(value.data);
        for (auto& [key, val] : local_map) {
            auto shmem_value = create_value(mem, *val);
            Value::shmem_map::set(shmem_map, mem, key.c_str(), shmem_value);
        }
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<Value::shmem_map_t>(shmem_map.get_relative(mem));
        });
    } else if (value.type == ValueType::STRING || value.type == ValueType::REFERENCE) {
        auto& local_string = get<std::string>(value.data);
        auto shmem_string = mem.construct_array<char>(local_string.size() + 1);
        std::strcpy(shmem_string.get(mem), local_string.c_str());
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<Value::shmem_string_t>(shmem_string.get_relative(mem));
        });
    } else if (value.type == ValueType::SEMAPHORE) {
        // TODO
        throw std::runtime_error("Not Implemented");
    } else {
        throw std::runtime_error("Unknown ValueType");
    }
    return res;
}

void Value::destroy_value(shmem &mem, offset_ptr<Value> value) {
    // TODO recursively destroy other values
    mem.destroy(value);
}
