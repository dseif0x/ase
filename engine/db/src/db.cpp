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
    mem.construct<DB_Meta>(mem);
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

std::optional<CopiedValue> get_key_rec(db_map<Value>* map, std::queue<std::string>& keys) {
    auto key = keys.front();
    std::optional<CopiedValue> res{};
    map->transaction([&](locked_db_map<Value>* map) {
        if (map->has(key.c_str()))
            res.emplace(map->get(key.c_str()));
    });
    if (!res.has_value() || keys.size() == 1)
        return res;
    keys.pop();
    auto newMap = *res;
    if (newMap.type != ValueType::MAP)
        throw std::runtime_error("Unexpected Leaf encountered when resolving tree");
    return get_key_rec(res->map_value, keys);
}

std::optional<CopiedValue> get_key(db_key_map* map, std::queue<std::string>& keys) {
    if (keys.empty())
        return std::optional<CopiedValue>{};
    auto key = keys.front();
    std::optional<CopiedValue> res{};
    map->transaction([&](locked_db_map<relative_offset_ptr<Value>>* map) {
        if (map->has(key.c_str()))
            res.emplace(*map->get(key.c_str()));
    });
    if (!res.has_value() || keys.size() == 1)
        return res;
    keys.pop();
    auto newMap = *res;
    if (newMap.type != ValueType::MAP)
        throw std::runtime_error("Unexpected Leaf encountered when resolving tree");
    return get_key_rec(res->map_value, keys);
}

std::optional<CopiedValue> DB::get(const char *key) {
    auto entrypoint_p = get_entrypoint();
    auto key_split = split(key);
    std::optional<CopiedValue> res;
    entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
        res = get_key(entrypoint, key_split);
    });
    return res;
}

void DB::set(const char *key, CopiedValue value) {
    auto entrypoint_p = get_entrypoint();
    offset_ptr<Value> new_value = mem.construct<Value>(mem, value);
    db_key_map::set(entrypoint_p, mem, key, new_value.get_relative(mem));
}
