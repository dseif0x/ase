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
    mem.construct<db_key_map>();
};

DB::DB() : mem{init_db} {

}

DB::~DB() {

}

void DB::destroy() {
    shmem::destroy_data();
}

offset_ptr<db_key_map> DB::get_entrypoint() {
    return {mem.get_management_offset()};
}

std::deque<std::string> split(const char* input) {
    std::deque<std::string> result;
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
            result.push_back(token);
        }
    }

    return result;
}

template<typename Lambda>
void get_key(db_map& map, shmem& mem, std::deque<std::string>& keys, Lambda&& cb) {
    static_assert(std::is_invocable_v<Lambda, std::optional<relative_offset_ptr<Value>>>, "Lambda must be invocable with std::optional<relative_offset_ptr<Value>>");

    std::shared_lock<shrwlock> lock_guard(map.get_lock());

    if (keys.empty()) {
        cb(std::nullopt);
        return;
    }
    auto key = keys.front();
    relative_offset_ptr<Value> res;
    if (map.has(mem, key.c_str())) {
        res = map.get(mem, key.c_str());;
    } else {
        cb(std::nullopt);
        return;
    }
    if (keys.size() == 1) {
        cb(res);
        return;
    }
    keys.pop_front();
    if (res->type != ValueType::MAP) {
        throw std::runtime_error("Unexpected Leaf encountered when resolving tree");
    }
    auto& newMap = std::get<Value::shmem_map_t>(res->data);
    get_key(*newMap, mem, keys, cb);
}

std::optional<CopiedValue> DB::get(const char *key) {
    auto entrypoint_p = get_entrypoint();
    auto key_split = split(key);
    std::optional<CopiedValue> res {};
    entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
        get_key(*entrypoint, mem, key_split, [&](std::optional<relative_offset_ptr<Value>> val) {
            if (val.has_value())
                res.emplace(*val.value(), mem);
        });
    });
    return res;
}

std::optional<CopiedValue> DB::follow(const char* key, uint16_t max_chained_follows) {
    auto entrypoint_p = get_entrypoint();
    std::optional<CopiedValue> res{};
    std::string next_key = std::string(key);
    entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
        while(true) {
            auto split_keys = split(next_key.c_str());
            bool should_break = false;
            get_key(*entrypoint, mem, split_keys, [&](std::optional<relative_offset_ptr<Value>> nextNode) {
                if (!nextNode.has_value()) {
                    should_break = true;
                    return;
                }
                auto nextVal = nextNode.value();
                if (nextVal->type == ValueType::REFERENCE && max_chained_follows-- > 0) {
                    auto& str = std::get<Value::shmem_string_t>(nextVal->data);
                    next_key = str.get();
                } else {
                    res.emplace(*nextVal, mem);
                    should_break = true;
                    return;
                }
            });
            if (should_break)
                break;
        }
    });
    return res;
}

CopiedValue generate_subdomain(std::deque<std::string>& keys, CopiedValue&& new_value) {

    std::unordered_map<std::string, std::unique_ptr<CopiedValue>> mep;
    mep.emplace(keys.back(), std::make_unique<CopiedValue>(std::move(new_value)));

    keys.pop_back();
    while (!keys.empty()) {
        auto key = keys.back();

        auto new_map = std::unordered_map<std::string, std::unique_ptr<CopiedValue>>();

        CopiedValue new_val {};
        new_val.type = ValueType::MAP;
        new_val.data.emplace<CopiedValue::copied_map_t>(std::move(mep));

        new_map.emplace(key, std::make_unique<CopiedValue>(std::move(new_val)));

        mep = std::move(new_map);

        keys.pop_back();
    }

    CopiedValue new_val {};
    new_val.type = ValueType::MAP;
    new_val.data.emplace<CopiedValue::copied_map_t>(std::move(mep));
    return new_val;
}

void set_key_(offset_ptr<db_map> map, shmem& mem, std::deque<std::string>& keys, CopiedValue&& new_value, const bool force, std::unique_lock<shrwlock>& shmemlock) {
    if (keys.empty())
        return;
    auto entrypoint = map.get(mem);
    auto &lock = map.get(mem)->get_lock();
    lock.lock();
    auto key = keys.front();
    bool exists = false;
    if (keys.size() == 1) {
        if (!force && entrypoint->has(mem, key.c_str())) {
            exists = true;
        }
        // no force check needed, the condition only does not hold when force is false
        if (!exists) {
            auto tmp = Value::create_value(mem, new_value, true);
            db_key_map::set(map, mem, key.c_str(), tmp);
        }
        {
            auto &lock2 = map.get(mem)->get_lock();
            lock2.unlock();
        }
        return;
    }
    keys.pop_front();
    if (entrypoint->has(mem, key.c_str())) {
        auto newMap = *entrypoint->get(mem, key.c_str());
        if (newMap.type != ValueType::MAP) {
            lock.unlock();
            throw std::runtime_error("Unexpected Leaf encountered when resolving tree");
        }
        offset_ptr<db_map> ptr = mem.relative_to_offset(std::get<Value::shmem_map_t>(newMap.data));
        set_key_(ptr, mem, keys, std::move(new_value), force, shmemlock);
    } else {
        auto subdomain = generate_subdomain(keys, std::move(new_value));
        auto tmp = Value::create_value(mem, subdomain, true);
        db_key_map::set(map, mem, key.c_str(), tmp);
    }
    {
        auto &lock2 = map.get(mem)->get_lock();
        lock2.unlock();
    }
}

void set_key(offset_ptr<db_map>& map, shmem& mem, std::deque<std::string>& keys, CopiedValue&& new_value, const bool force) {
    std::unique_lock<shrwlock> lock_guard(mem.get_meta_mutex());
    set_key_(map, mem, keys, std::move(new_value), force, lock_guard);
}

void DB::set(const char *key, CopiedValue&& value, const bool force) {
    auto entrypoint_p = get_entrypoint();
    std::deque<std::string> keys = split(key);
    set_key(entrypoint_p, mem, keys, std::move(value), force);
}

using list_val_data_t = std::variant<std::monostate, bool, int, std::string, double>;
using list_map_enty_t = std::pair<ValueType, list_val_data_t>;
using list_map_t = std::unordered_map<std::string, list_map_enty_t>;
DB::list_t DB::list(const char* key) {
    auto entrypoint_p = get_entrypoint();
    auto key_split = split(key);
    list_t res {};
    entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
        get_key(*entrypoint, mem, key_split, [&](std::optional<relative_offset_ptr<Value>> val) {
            if (val.has_value()) {
                auto& deref_val = val.value();
                if (deref_val->type != ValueType::MAP)
                    return;
                auto& cur_map = std::get<relative_offset_ptr<db_map>>(deref_val->data);
                list_map_t res_map{};
                cur_map->transaction([&](locked_db_map* locked_map) {
                    for(auto& key : locked_map->keys(mem)) {
                        auto original_entry = locked_map->get(mem, key.c_str());
                        list_map_enty_t entry{original_entry->type, std::monostate{}};
                        if (entry.first == ValueType::INT) {
                            entry.second.emplace<int>(std::get<int>(original_entry->data));
                        } else if (entry.first == ValueType::BOOLEAN) {
                            entry.second.emplace<bool>(std::get<bool>(original_entry->data));
                        } else if (entry.first == ValueType::DOUBLE) {
                            entry.second.emplace<double>(std::get<double>(original_entry->data));
                        } else if (entry.first == ValueType::STRING) {
                            auto& string_val = std::get<Value::shmem_string_t>(original_entry->data);
                            entry.second.emplace<std::string>(string_val.get());
                        }
                        res_map.emplace(key, entry);
                    }
                });
                res.emplace(std::move(res_map));
            }
        });
    });
    return res;
}

void DB::remove(const char* key) {
    auto entrypoint_p = get_entrypoint();
    auto key_split = split(key);
    auto last_key = key_split.back();
    key_split.pop_back();
    std::optional<offset_ptr<db_map>> res {};
    if (key_split.empty()) { // top level key
        res.emplace(entrypoint_p);
    } else {
        entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
            get_key(*entrypoint, mem, key_split, [&](std::optional<relative_offset_ptr<Value>> val) {
                if (val.has_value()) {
                    auto extracted_val = *val;
                    if (extracted_val->type == ValueType::MAP) {
                        res.emplace(mem.relative_to_offset(std::get<Value::shmem_map_t>(extracted_val->data)));
                    }
                }
            });
        });
    }
    if (res.has_value()) {
        db_map::remove(res.value(), mem, last_key.c_str());
    }
}

void DB::create_lock(const char* key, int max_locks) {
    CopiedValue new_val{};
    new_val.type = ValueType::SEMAPHORE;
    new_val.data.emplace<int>(max_locks);
    this->set(key, std::move(new_val), true);
}

void DB::acquire_lock(const char* key) {
    auto entrypoint_p = get_entrypoint();
    std::deque<std::string> keys = split(key);
    std::optional<offset_ptr<db_map>> res {};
    entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
        get_key(*entrypoint, mem, keys, [&](std::optional<relative_offset_ptr<Value>> val) {
            if (val.has_value()) {
                auto& extracted_val = *val;
                if (extracted_val->type == ValueType::SEMAPHORE) {
                    auto& sem = std::get<Value::shmem_sem_t>(extracted_val->data);
                    res.emplace(mem.relative_to_offset(sem));
                }
            }
        });
    });
    if (res.has_value()) {
        auto sem = res.value();
        shsem::acquire(sem, mem);
    }
}

void DB::release_lock(const char *key) {
    auto entrypoint_p = get_entrypoint();
    std::deque<std::string> keys = split(key);
    std::optional<offset_ptr<db_map>> res {};
    entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
        get_key(*entrypoint, mem, keys, [&](std::optional<relative_offset_ptr<Value>> val) {
            if (val.has_value()) {
                auto& extracted_val = *val;
                if (extracted_val->type == ValueType::SEMAPHORE) {
                    auto& sem = std::get<Value::shmem_sem_t>(extracted_val->data);
                    sem->release();
                }
            }
        });
    });
}