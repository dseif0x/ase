/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "db.h"

#include <queue>
#include <sstream>

auto init_db = [](shmem& mem) {
    mem.construct<db_key_map>(mem);
};

DB::DB() : mem{init_db} {

}

DB::~DB() {

}

void DB::destroy() {
    mem.destroy();
}

offset_ptr<db_key_map> DB::get_entrypoint() {
    return {mem.get_management_offset()};
}

std::queue<std::string> split(const char* input) {
    std::queue<std::string> result;
    std::string str(input);

    // Remove leading dot, if present
    if (!str.empty() && str[0] == '.') {
        str = str.substr(1);
    }

    std::stringstream ss(str);
    std::string token;

    // Split by dot and store each part in the vector
    while (std::getline(ss, token, '.')) {
        if (!token.empty()) { // Ignore empty tokens
            result.push(token);
        }
    }

    return result;
}

std::optional<CopiedValue> get_key(db_map& map, std::queue<std::string>& keys) {
    if (keys.empty())
        return std::optional<CopiedValue>{};
    auto key = keys.front();
    std::optional<std::reference_wrapper<Value>> res{};
    map.transaction([&](locked_db_map* map) {
        if (map->has(key.c_str())) {
            res.emplace(*map->get(key.c_str()));
        }
    });
    if (!res.has_value())
        return std::make_optional<CopiedValue>();
    if (keys.size() == 1)
        return std::make_optional(res->get());
    keys.pop();
    auto& newMap = *res;
    if (newMap.get().type != ValueType::MAP)
        throw std::runtime_error("Unexpected Leaf encountered when resolving tree");
    return get_key(*std::get<Value::shmem_map_t>(newMap.get().data), keys);
}

std::optional<CopiedValue> DB::get(const char *key) {
    auto entrypoint_p = get_entrypoint();
    auto key_split = split(key);
    std::optional<CopiedValue> res {};
    entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
        auto val = get_key(*entrypoint, key_split);
        if (val.has_value())
            res = std::move(val);
    });
    return res;
}

void set_key(offset_ptr<db_map> map, shmem& mem, std::queue<std::string>& keys, offset_ptr<Value>& new_value) {
    if (keys.empty())
        return;
    auto key = keys.front();
    if (keys.size() == 1) {
        db_key_map::set(map, mem, key.c_str(), new_value);
        return;
    }
    std::optional<std::reference_wrapper<Value>> res{};
    map.get(mem)->transaction([&](locked_db_map* map) {
        if (map->has(key.c_str())) {
            res.emplace(*map->get(key.c_str()));
        }
    });
    if (!res.has_value())
        return;
    keys.pop();
    auto& newMap = *res;
    if (newMap.get().type != ValueType::MAP)
        throw std::runtime_error("Unexpected Leaf encountered when resolving tree");
    return set_key(mem.relative_to_offset(std::get<Value::shmem_map_t>(newMap.get().data)), mem, keys, new_value);
}

void DB::set(const char *key, const CopiedValue& value) {
    auto entrypoint_p = get_entrypoint();
    offset_ptr<Value> new_value = Value::create_value(mem, value);
    auto keys = split(key);
    set_key(entrypoint_p, mem, keys, new_value);
}
