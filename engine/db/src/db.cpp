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
        return std::nullopt;
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

std::optional<CopiedValue> DB::follow(const char* key, uint16_t max_chained_follows) {
    if (max_chained_follows == 1) {
        auto start = get(key); // TODO: it's inefficient to copy more than once.
        if (!start.has_value()) return std::nullopt;
        auto& start_val = start.value();
        if (start_val.type != ValueType::REFERENCE) return start;
        return get(std::get<std::string>(start_val.data).c_str());
    }
    if (max_chained_follows == 0) {
        std::optional<CopiedValue> next;
        std::string next_key{key};
        while(true) {
            next = get(next_key.c_str());
            if (!next.has_value()) return std::nullopt;
            auto& val = next.value();
            if (val.type == ValueType::REFERENCE) {
                next_key = std::get<std::string>(val.data);
            } else {
                return next;
            }
        }
    }
    { // max_chained_follows > 1
        auto start = get(key);
        if (!start.has_value()) return std::nullopt;
        auto& val = start.value();
        if (val.type != ValueType::REFERENCE) return start;
        return follow(std::get<std::string>(val.data).c_str(), max_chained_follows - 1);
    }
}

void set_key(offset_ptr<db_map>& map, shmem& mem, std::queue<std::string>& keys, offset_ptr<Value>& new_value) {
    if (keys.empty())
        return;
    auto key = keys.front();
    if (keys.size() == 1) {
        db_key_map::set(map, mem, key.c_str(), new_value);
        return;
    }
    std::optional<std::reference_wrapper<Value>> res{};
    keys.pop();
    offset_ptr<db_map> ptr;
    map.atomic_access(mem, [&](db_key_map* entrypoint)
    {
        entrypoint->transaction([&](locked_db_map* map) {
            if (map->has(key.c_str())) {
                res.emplace(*map->get(key.c_str()));
            }
        });
        if (!res.has_value())
            return;
        auto& newMap = *res;
        if (newMap.get().type != ValueType::MAP)
            throw std::runtime_error("Unexpected Leaf encountered when resolving tree");
        ptr = mem.relative_to_offset(std::get<Value::shmem_map_t>(newMap.get().data));
    });
    if (!res.has_value())
        return;
    return set_key(ptr, mem, keys, new_value);
}

void DB::set(const char *key, const CopiedValue& value) {
    auto entrypoint_p = get_entrypoint();
    offset_ptr<Value> new_value = Value::create_value(mem, value);
    auto keys = split(key);
    set_key(entrypoint_p, mem, keys, new_value);
}
