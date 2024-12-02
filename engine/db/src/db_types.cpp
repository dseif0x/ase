/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
#include "custom_map.h"
#include "db_types.h"

#include <iostream>

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
        auto& local_array = data.emplace<copied_array_t>();
        for (size_t i = 0; i < array.size; i++) {
            local_array.emplace_back(array.array_value.get()[i]);
        }
    } else if (type == ValueType::PAIR) {
        auto& pair = get<Value::shmem_pair_t>(value.data);
        data.emplace<CopiedValue::copied_pair_t>() = {std::make_unique<CopiedValue>(*pair.first), std::make_unique<CopiedValue>(*pair.second)};
    } else if (type == ValueType::MAP) {
        auto& map = get<Value::shmem_map_t>(value.data);
        auto& local_map = data.emplace<CopiedValue::copied_map_t>();
        for (auto& key : map->atomic_keys()) {
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

void CopiedValue::print() {
    CopiedValue::print(*this);
}

void CopiedValue::print(CopiedValue &val) {
    if (val.type == ValueType::INT) {
        std::cout << std::get<int>(val.data);
    } else if (val.type == ValueType::STRING) {
        std::cout << "\"" << std::get<std::string>(val.data) << "\"";
    } else if (val.type == ValueType::DOUBLE) {
        std::cout << std::get<double>(val.data);
    } else if (val.type == ValueType::BOOLEAN) {
        std::cout << std::get<bool>(val.data);
    } else if (val.type == ValueType::PAIR) {
        auto &temp = std::get<CopiedValue::copied_pair_t>(val.data);
        std::cout << "(";
        print(*temp.first);
        std::cout << ", ";
        print(*temp.second);
        std::cout << ")";
    } else if (val.type == ValueType::ARRAY) {
        std::cout << "[";
        auto &temp = std::get<CopiedValue::copied_array_t>(val.data);
        for (int x = 0; x < temp.size(); x++) {
            if (x != 0) std::cout << ", ";
            print(temp[x]);
        }
        std::cout << "]";
    } else if (val.type == ValueType::MAP) {
        std::cout << "{";
        auto &temp = std::get<CopiedValue::copied_map_t>(val.data);
        bool nfirst = false;
        for (auto& [key, value] : temp) {
            if (nfirst) std::cout << ", ";
            nfirst = true;
            std::cout << key << ": ";
            print(*value);
        }
        std::cout << "}";
    } else {
        std::cout << "Value found, but type (" << (int)val.type << ") not handeled" << std::endl;
    }
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
        auto& local_array = get<CopiedValue::copied_array_t>(value.data);
        auto shmem_array_data = mem.construct_array<Value>(local_array.size());

        std::vector<offset_ptr<Value>> shmem_array_data_ptrs;
        shmem_array_data_ptrs.reserve(local_array.size());
        for (const auto& val : local_array) {
            shmem_array_data_ptrs.push_back(create_value(mem, val));
        }

        res.atomic_access(mem, [&](Value* new_value) {
            auto& array = new_value->data.emplace<ArrayType>();
            array.size = local_array.size();
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

void Value::destroy_value(shmem &mem, offset_ptr<Value> value, bool locks_held) {
    if (value.get(mem)->type == ValueType::MAP) {
        auto& data = std::get<shmem_map_t>(value.get(mem)->data);
        data->transaction([&](locked_db_map* map) {
            for(auto& key : map->keys()) {
                auto child = map->get(key.c_str());
                destroy_value(mem, mem.relative_to_offset(child), locks_held);
            }
        });
    } else if (value.get(mem)->type == ValueType::ARRAY) {
        auto& data = std::get<ArrayType>(value.get(mem)->data);
        mem.destroy_array(mem.relative_to_offset(data.array_value), data.size, locks_held);
    } else if (value.get(mem)->type == ValueType::PAIR) {
        auto& data = std::get<shmem_pair_t>(value.get(mem)->data);
        destroy_value(mem, mem.relative_to_offset(data.first), locks_held);
        destroy_value(mem, mem.relative_to_offset(data.second), locks_held);
    }
    mem.destroy(value, locks_held);
}
