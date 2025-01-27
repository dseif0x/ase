/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "custom_map.h"
#include "db_types.h"

#include <iostream>
#include <iomanip>
#include <cassert>

CopiedValue::CopiedValue(const Value &value, shmem& mem) {
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
            local_array.emplace_back(*array.array_value.get()[i].get(mem), mem);
        }
    } else if (type == ValueType::PAIR) {
        auto& pair = get<Value::shmem_pair_t>(value.data);
        data.emplace<CopiedValue::copied_pair_t>() = {std::make_unique<CopiedValue>(*pair.first, mem), std::make_unique<CopiedValue>(*pair.second, mem)};
    } else if (type == ValueType::MAP) {
        auto& map = get<Value::shmem_map_t>(value.data);
        auto& local_map = data.emplace<CopiedValue::copied_map_t>();
        for (auto& key : map->atomic_keys(mem)) {
            auto& val = *map->atomic_get(mem, key.c_str());
            local_map.emplace(key, std::make_unique<CopiedValue>(val, mem));
        }
    } else if (type == ValueType::STRING || type == ValueType::REFERENCE) {
        auto& str = get<Value::shmem_string_t>(value.data);
        data.emplace<std::string>(str.get());
    } else if (type == ValueType::SEMAPHORE) {
        auto& sem = get<Value::shmem_sem_t>(value.data);
        data.emplace<int>(sem->get());
    } else {
        throw std::runtime_error("Unknown ValueType");
    }
}

CopiedValue CopiedValue::make_deep_copy(CopiedValue &val) {
    CopiedValue res{};
    res.type = val.type;
    if (std::holds_alternative<bool>(val.data)) {
        res.data = std::get<bool>(val.data);
    } else if (std::holds_alternative<int>(val.data)) {
        res.data = std::get<int>(val.data);
    } else if (std::holds_alternative<double>(val.data)) {
        res.data = std::get<double>(val.data);
    } else if (std::holds_alternative<std::string>(val.data)) {
        res.data = std::get<std::string>(val.data);
    } else if (std::holds_alternative<copied_pair_t>(val.data)) {
        auto &pair = res.data.emplace<copied_pair_t>();
        auto &temp = std::get<copied_pair_t>(val.data);
        //pair.first = (std::make_unique<CopiedValue>(temp.first ? std::move(make_deep_copy(*temp.first)) : std::nullopt));
        //pair.second = (std::make_unique<CopiedValue>(temp.second ? std::move(make_deep_copy(*temp.second)) : std::nullopt));
        pair.first = (std::make_unique<CopiedValue>(std::move(make_deep_copy(*temp.first))));
        pair.second = (std::make_unique<CopiedValue>(std::move(make_deep_copy(*temp.second))));
    } else if (std::holds_alternative<copied_array_t>(val.data)) {
        auto &vec = res.data.emplace<copied_array_t>();
        auto &temp = std::get<copied_array_t>(val.data);
        for (auto &x : temp) {
            vec.emplace_back(std::move(make_deep_copy(x)));
        }
    } else if (std::holds_alternative<copied_map_t>(val.data)) {
        auto &map = res.data.emplace<copied_map_t>();
        auto &temp = std::get<copied_map_t>(val.data);
        for (auto &[key, value] : temp) {
            //map.emplace(key, std::make_unique<CopiedValue>(value ? std::move(make_deep_copy(*value)) : std::nullopt));
            map.emplace(key, std::make_unique<CopiedValue>(std::move(make_deep_copy(*value))));
        }
    }
    return res;
}

std::string CopiedValue::get_python_style(CopiedValue &val) {
    std::string res = "";
    if (val.type == ValueType::INT) {
        res += std::to_string(std::get<int>(val.data));
    } else if (val.type == ValueType::STRING) {
        res += "\"" + std::get<std::string>(val.data) + "\"";
    } else if (val.type == ValueType::DOUBLE) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << std::get<double>(val.data);
        res += oss.str();
    } else if (val.type == ValueType::BOOLEAN) {
        res += std::get<bool>(val.data) ? "true" : "false";
    } else if (val.type == ValueType::SEMAPHORE) {
        res += "sem<" + std::to_string(std::get<int>(val.data)) + ">";
    } else if (val.type == ValueType::REFERENCE) {
        res += "ref<" + std::get<std::string>(val.data) + ">";
    } else if (val.type == ValueType::PAIR) {
        auto &temp = std::get<CopiedValue::copied_pair_t>(val.data);
        res += "(";
        res += get_python_style(*temp.first);
        res += ", ";
        res += get_python_style(*temp.second);
        res += ")";
    } else if (val.type == ValueType::ARRAY) {
        res += "[";
        auto &temp = std::get<CopiedValue::copied_array_t>(val.data);
        for (int x = 0; x < temp.size(); x++) {
            if (x != 0) res += ", ";
            res += get_python_style(temp[x]);
        }
        res += "]";
    } else if (val.type == ValueType::MAP) {
        res += "{";
        auto &temp = std::get<CopiedValue::copied_map_t>(val.data);
        bool nfirst = false;
        for (auto& [key, value] : temp) {
            if (nfirst) res += ", ";
            nfirst = true;
            res += key + ": ";
            res += get_python_style(*value);
        }
        res += "}";
    } else {
        res += "?????";
    }
    return res;
}


void CopiedValue::print() {
    CopiedValue::print(*this);
}

void CopiedValue::print(CopiedValue &val) {
    std::cout << CopiedValue::get_python_style(val) << std::endl;
}

offset_ptr<Value> Value::create_value(shmem &mem, const CopiedValue &value, bool locks_held) {
    offset_ptr<Value> res;
    if (locks_held) {
        res = mem.construct_unsafe<Value>();
    } else {
        res = mem.construct<Value>();
    }
    res.atomic_access(mem, [&](Value* new_value) {
        new_value->type = value.type;
    }, locks_held);

    if (value.type == ValueType::INT) {
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<int>(get<int>(value.data));
        }, locks_held);
    } else if (value.type == ValueType::BOOLEAN) {
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<bool>(get<bool>(value.data));
        }, locks_held);
    }  else if (value.type == ValueType::DOUBLE) {
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<double>(get<double>(value.data));
        }, locks_held);
    } else if (value.type == ValueType::ARRAY) {
        auto& local_array = get<CopiedValue::copied_array_t>(value.data);
        offset_ptr<offset_ptr<Value>> shmem_array_data;
        if (locks_held) {
            shmem_array_data = mem.construct_array_unsafe<offset_ptr<Value>>(local_array.size());
        } else {
            shmem_array_data = mem.construct_array<offset_ptr<Value>>(local_array.size());
        }

        std::vector<offset_ptr<Value>> shmem_array_data_ptrs;
        shmem_array_data_ptrs.reserve(local_array.size());
        for (const auto& val : local_array) {
            shmem_array_data_ptrs.push_back(create_value(mem, val, locks_held));
        }

        res.atomic_access(mem, [&](Value* new_value) {
            auto& array = new_value->data.emplace<ArrayType>();
            array.size = local_array.size();
            array.array_value = shmem_array_data.get_relative(mem);

            offset_ptr<Value>* shmem_array_data_ptr = shmem_array_data.get(mem);
            for (size_t i = 0; i < array.size; i++) {
                shmem_array_data_ptr[i] = shmem_array_data_ptrs[i];
            }
        }, locks_held);
    } else if (value.type == ValueType::PAIR) {
        auto& local_pair = get<CopiedValue::copied_pair_t>(value.data);
        auto pair_v1 = create_value(mem, *local_pair.first, locks_held);
        auto pair_v2 = create_value(mem, *local_pair.second, locks_held);
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<Value::shmem_pair_t>(pair_v1.get_relative(mem), pair_v2.get_relative(mem));
        }, locks_held);
    } else if (value.type == ValueType::MAP) {
        offset_ptr<db_map> shmem_map;
        if (locks_held) {
            shmem_map = mem.construct_unsafe<db_map>();
        } else {
            shmem_map = mem.construct<db_map>();
        }
        auto& local_map = get<CopiedValue::copied_map_t>(value.data);
        for (auto& [key, val] : local_map) {
            auto shmem_value = create_value(mem, *val, locks_held);
            Value::shmem_map::set(shmem_map, mem, key.c_str(), shmem_value);
        }
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<Value::shmem_map_t>(shmem_map.get_relative(mem));
        }, locks_held);
    } else if (value.type == ValueType::STRING || value.type == ValueType::REFERENCE) {
        auto& local_string = get<std::string>(value.data);
        offset_ptr<char> shmem_string;
        if (locks_held) {
            shmem_string = mem.construct_array_unsafe<char>(local_string.size() + 1);
        } else {
            shmem_string = mem.construct_array<char>(local_string.size() + 1);
        }
        std::strcpy(shmem_string.get(mem), local_string.c_str());
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<Value::shmem_string_t>(shmem_string.get_relative(mem));
        }, locks_held);
    } else if (value.type == ValueType::SEMAPHORE) {
        auto& local_sem = get<int>(value.data);
        offset_ptr<shsem> shmem_sem;
        if (locks_held) {
            shmem_sem = mem.construct_unsafe<shsem>(local_sem);
        } else {
            shmem_sem = mem.construct<shsem>(local_sem);
        }
        res.atomic_access(mem, [&](Value* new_value) {
            new_value->data.emplace<Value::shmem_sem_t>(shmem_sem.get_relative(mem));
        }, locks_held);
    } else {
        throw std::runtime_error("Unknown ValueType");
    }
    return res;
}

void Value::destroy_value(shmem &mem, offset_ptr<Value> value, bool locks_held) {
    assert(locks_held && "LOCKS MUST BE HELD");
    if (value.get(mem)->type == ValueType::MAP) {
        auto& data = std::get<shmem_map_t>(value.get(mem)->data);
        auto& shmemap = *data;
        auto* dataarr = shmemap.data.get(mem);
        for(int i = 0; i < shmemap.used_size; i++) {
            auto& key = std::get<0>(dataarr[i]);
            auto& value = std::get<1>(dataarr[i]);
            destroy_value(mem, value, locks_held);
            mem.destroy_array(key, 0, true);
        }
        if (shmemap.alloc_size > 0) {
            mem.destroy_array(shmemap.lookup, 0, true);
            mem.destroy_array(shmemap.data, 0, true);
        }
        shmemap.~db_map();
        mem.destroy(mem.relative_to_offset(data), locks_held);
    } else if (value.get(mem)->type == ValueType::ARRAY) {
        auto& data = std::get<ArrayType>(value.get(mem)->data);
        offset_ptr<Value>* data_p = data.array_value.get();
        for (size_t i = 0; i < data.size; i++) {
            destroy_value(mem, data_p[i], locks_held);
        }
        mem.destroy_array(mem.relative_to_offset(data.array_value), data.size, locks_held);
    } else if (value.get(mem)->type == ValueType::PAIR) {
        auto& data = std::get<shmem_pair_t>(value.get(mem)->data);
        destroy_value(mem, mem.relative_to_offset(data.first), locks_held);
        destroy_value(mem, mem.relative_to_offset(data.second), locks_held);
    } else if (value.get(mem)->type == ValueType::STRING || value.get(mem)->type == ValueType::REFERENCE) {
        auto& data = std::get<shmem_string_t>(value.get(mem)->data);
        mem.destroy_array(mem.relative_to_offset(data), 0, locks_held);
    } else if (value.get(mem)->type == ValueType::SEMAPHORE) {
        auto& data = std::get<shmem_sem_t>(value.get(mem)->data);
        data->~shsem();
        mem.destroy(mem.relative_to_offset(data), locks_held);
    }
    mem.destroy(value, locks_held);
}
