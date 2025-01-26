/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "db.h"

#include <queue>
#include <sstream>
#include <fstream>

auto init_db = [](shmem& mem) {
    mem.construct<db_key_map>();
};

DB::DB() : mem{init_db} {

}

DB::~DB() {

}

bool DB::dump_data(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Get the data (metadata + vector of bytes)
    auto dat = mem.dump_data();
    const auto& meta = dat.first;
    const auto& data = dat.second;

    // Write the meta data
    file.write(reinterpret_cast<const char*>(&meta), sizeof(meta));
    if (!file.good()) {
        return false;
    }

    // Write the raw bytes of the vector
    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        if (!file.good()) {
            return false;
        }
    }

    return true;
}

bool DB::load_data(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Read the meta data first
    dump_meta meta{};
    file.read(reinterpret_cast<char*>(&meta), sizeof(meta));
    if (!file.good()) {
        return false;
    }

    // Prepare a vector of the correct size, read raw bytes into it
    std::vector<uint8_t> data(meta.alloc_size);
    if (meta.alloc_size > 0) {
        file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(meta.alloc_size));
        if (!file.good()) {
            return false;
        }
    }

    // Now pass the reconstructed dump to Memory::load_data()
    shmem::dump_t dat(meta, data);
    mem.load_data(dat);

    return true;
}

void DB::destroy() {
    shmem::destroy_data();
}

std::size_t DB::get_used_memory() {
    return mem.get_used_memory();
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
        cb(std::nullopt);
        return;
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

template<typename T>
std::optional<T> max(db_map& map, shmem& mem, const char* field, const std::vector<const char*>& followFields, ValueType VT)
{
    std::optional<T> res {};
    std::shared_lock<shrwlock> lock_guard(map.get_lock());
    if (map.has(mem, field)) {
        auto val = map.get(mem, field).get();
        if (val->type != VT)
            return res;
        res.emplace(std::get<T>(val->data));
        for (auto& followField : followFields) {
            if (map.has(mem, followField)) {
                auto next = map.get(mem, followField).get();
                if (next->type == ValueType::MAP) {
                    auto nextVal = max<T>(*std::get<Value::shmem_map_t>(next->data).get(), mem, field, followFields, VT);
                    if (nextVal.has_value())
                        res.emplace(std::max(res.value(), nextVal.value()));
                }
            }
        }
    }
    return res;
}

template<typename T>
std::optional<T> min(db_map& map, shmem& mem, const char* field, const std::vector<const char*>& followFields, ValueType VT)
{
    std::optional<T> res {};
    std::shared_lock<shrwlock> lock_guard(map.get_lock());
    if (map.has(mem, field)) {
        auto val = map.get(mem, field).get();
        if (val->type != VT)
            return res;
        res.emplace(std::get<T>(val->data));
        for (auto& followField : followFields) {
            if (map.has(mem, followField)) {
                auto next = map.get(mem, followField).get();
                if (next->type == ValueType::MAP) {
                    auto nextVal = min<T>(*std::get<Value::shmem_map_t>(next->data).get(), mem, field, followFields, VT);
                    if (nextVal.has_value())
                        res.emplace(std::min(res.value(), nextVal.value()));
                }
            }
        }
    }
    return res;
}

template<typename T>
std::optional<std::pair<T, int>> avg(db_map& map, shmem& mem, const char* field, const std::vector<const char*>& followFields, ValueType VT)
{
    std::optional<std::pair<T, int>> res {};
    std::shared_lock<shrwlock> lock_guard(map.get_lock());
    if (map.has(mem, field)) {
        auto val = map.get(mem, field).get();
        if (val->type != VT)
            return res;
        res.emplace(std::make_pair(std::get<T>(val->data), 1));
        for (auto& followField : followFields) {
            if (map.has(mem, followField)) {
                auto next = map.get(mem, followField).get();
                if (next->type == ValueType::MAP) {
                    auto nextVal = avg<T>(*std::get<Value::shmem_map_t>(next->data).get(), mem, field, followFields, VT);
                    if (nextVal.has_value()) {
                        res.value().first += nextVal.value().first;
                        res.value().second += nextVal.value().second;
                    }
                }
            }
        }
    }
    return res;
}

template<typename T>
std::optional<T> sum(db_map& map, shmem& mem, const char* field, const std::vector<const char*>& followFields, ValueType VT)
{
    std::optional<T> res {};
    std::shared_lock<shrwlock> lock_guard(map.get_lock());
    if (map.has(mem, field)) {
        auto val = map.get(mem, field).get();
        if (val->type != VT)
            return res;
        res.emplace(std::get<T>(val->data));
        for (auto& followField : followFields) {
            if (map.has(mem, followField)) {
                auto next = map.get(mem, followField).get();
                if (next->type == ValueType::MAP) {
                    auto nextVal = sum<T>(*std::get<Value::shmem_map_t>(next->data).get(), mem, field, followFields, VT);
                    if (nextVal.has_value())
                        res.emplace(res.value() + nextVal.value());
                }
            }
        }
    }
    return res;
}

template<>
std::optional<CopiedValue> DB::aggregate<ValueType::DOUBLE>(Aggregation agg, const char* entrypoint, const char* field, const std::vector<const char*>& followFields)
{
    auto entrypoint_p = get_entrypoint();
    std::optional<CopiedValue> res {};
    auto key_split = split(entrypoint);
    entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
        get_key(*entrypoint, mem, key_split, [&](std::optional<relative_offset_ptr<Value>> val) {
        if (val.has_value()) {
            auto derefVal = val->get();
            if (derefVal->type == ValueType::MAP) {
                auto mapVal = std::get<Value::shmem_map_t>(derefVal->data);
                std::optional<double> aggVal;
                switch (agg)
                {
                case Aggregation::AVERAGE:
                    {
                        std::optional<std::pair<double, int>> avgPair = avg<double>(*mapVal.get(), mem, field, followFields, ValueType::DOUBLE);
                        if (avgPair.has_value()) {
                            aggVal = std::make_optional(avgPair.value().first / avgPair.value().second);
                        } else {
                            aggVal = std::nullopt;
                        }
                        break;
                    }
                case Aggregation::SUM:
                    {
                        aggVal = sum<double>(*mapVal.get(), mem, field, followFields, ValueType::DOUBLE);
                        break;
                    }
                case Aggregation::MIN:
                    {
                        aggVal = min<double>(*mapVal.get(), mem, field, followFields, ValueType::DOUBLE);
                        break;
                    }
                case Aggregation::MAX:
                    {
                        aggVal = max<double>(*mapVal.get(), mem, field, followFields, ValueType::DOUBLE);
                        break;
                    }
                default:
                    throw std::runtime_error("Aggregation type not supported");
                }
                if (aggVal.has_value()){
                    CopiedValue resVal {};
                    resVal.type = ValueType::DOUBLE;
                    resVal.data.emplace<double>(aggVal.value());
                    res.emplace(std::move(resVal));
                }
            }
        }
        });
    });
    return res;
}

template<>
std::optional<CopiedValue> DB::aggregate<ValueType::INT>(Aggregation agg, const char* entrypoint, const char* field, const std::vector<const char*>& followFields)
{
    auto entrypoint_p = get_entrypoint();
    std::optional<CopiedValue> res {};
    auto key_split = split(entrypoint);
    entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
        get_key(*entrypoint, mem, key_split, [&](std::optional<relative_offset_ptr<Value>> val) {
        if (val.has_value())
        {
            auto derefVal = val->get();
            if (derefVal->type == ValueType::MAP)
            {
                auto mapVal = std::get<Value::shmem_map_t>(derefVal->data);
                std::optional<int> aggVal;
                switch (agg)
                {
                case Aggregation::AVERAGE:
                    {
                        std::optional<std::pair<int, int>> avgPair = avg<int>(*mapVal.get(), mem, field, followFields, ValueType::INT);
                        if (avgPair.has_value()) {
                            CopiedValue resVal {};
                            resVal.type = ValueType::DOUBLE;
                            resVal.data.emplace<double>(avgPair.value().first / static_cast<double>(avgPair.value().second));
                            res.emplace(std::move(resVal));
                            return;
                        }
                        aggVal = std::nullopt;
                        break;
                    }
                case Aggregation::SUM:
                    {
                        aggVal = sum<int>(*mapVal.get(), mem, field, followFields, ValueType::INT);
                        break;
                    }
                case Aggregation::MIN:
                    {
                        aggVal = min<int>(*mapVal.get(), mem, field, followFields, ValueType::INT);
                        break;
                    }
                case Aggregation::MAX:
                    {
                        aggVal = max<int>(*mapVal.get(), mem, field, followFields, ValueType::INT);
                        break;
                    }
                default:
                    throw std::runtime_error("Aggregation type not supported");
                }
                if (aggVal.has_value()){
                    CopiedValue resVal {};
                    resVal.type = ValueType::INT;
                    resVal.data.emplace<int>(aggVal.value());
                    res.emplace(std::move(resVal));
                }
            }
        }
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

void set_key_(offset_ptr<db_map> map, shmem& mem, std::deque<std::string>& keys, CopiedValue&& new_value, const bool force) {
    if (keys.empty())
        return;
    auto entrypoint = map.get(mem);
    // TODO If we dont have a unique lock on the entire shmem this should handle locking more granularly
    //auto &lock = map.get(mem)->get_lock();
    //lock.lock();
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
        //{
        //    auto &lock2 = map.get(mem)->get_lock();
        //    lock2.unlock();
        //}
        return;
    }
    keys.pop_front();
    if (entrypoint->has(mem, key.c_str())) {
        auto newMap = *entrypoint->get(mem, key.c_str());
        if (newMap.type != ValueType::MAP) {
            if (force) { // if its not a map we can overwrite it with one
                auto subdomain = generate_subdomain(keys, std::move(new_value));
                auto tmp = Value::create_value(mem, subdomain, true);
                db_key_map::set(map, mem, key.c_str(), tmp);
            }
        } else {
            offset_ptr<db_map> ptr = mem.relative_to_offset(std::get<Value::shmem_map_t>(newMap.data));
            set_key_(ptr, mem, keys, std::move(new_value), force);
        }
    } else {
        auto subdomain = generate_subdomain(keys, std::move(new_value));
        auto tmp = Value::create_value(mem, subdomain, true);
        db_key_map::set(map, mem, key.c_str(), tmp);
    }
    //{
    //    auto &lock2 = map.get(mem)->get_lock();
    //    lock2.unlock();
    //}
}

void set_key(offset_ptr<db_map>& map, shmem& mem, std::deque<std::string>& keys, CopiedValue&& new_value, const bool force) {
    std::unique_lock<shrwlock> lock_guard(mem.get_meta_mutex());
    set_key_(map, mem, keys, std::move(new_value), force);
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
        list_map_t res_map{};
        auto collect_keys_cb = [&](locked_db_map* locked_map) {
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
        };
        if (key_split.empty()) { // get root
            entrypoint->transaction(collect_keys_cb);
            res.emplace(std::move(res_map));
            return;
        }
        get_key(*entrypoint, mem, key_split, [&](std::optional<relative_offset_ptr<Value>> val) {
            if (val.has_value()) {
                auto& deref_val = val.value();
                if (deref_val->type != ValueType::MAP)
                    return;
                auto& cur_map = std::get<relative_offset_ptr<db_map>>(deref_val->data);
                cur_map->transaction(collect_keys_cb);
                res.emplace(std::move(res_map));
            }
        });
    });
    return res;
}

template<typename Lambda>
void remove_key(db_map& map, shmem& mem, std::deque<std::string>& keys, Lambda&& cb) {
    static_assert(std::is_invocable_v<Lambda, std::optional<relative_offset_ptr<Value>>>, "Lambda must be invocable with std::optional<relative_offset_ptr<Value>>");

    std::unique_lock<shrwlock> lock_guard(map.get_lock());

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
    remove_key(*newMap, mem, keys, cb);
}

void DB::remove(const char* key) {
    auto entrypoint_p = get_entrypoint();
    auto key_split = split(key);
    auto last_key = key_split.back();
    key_split.pop_back();

    std::unique_lock<shrwlock> lock_guard(mem.get_meta_mutex());

    offset_ptr<db_map> res;
    if (key_split.empty()) { // top level key
        std::unique_lock<shrwlock> lock_guard(entrypoint_p.get(mem)->get_lock());
        db_map::remove(entrypoint_p, mem, last_key.c_str());
    } else {
        remove_key(*entrypoint_p.get(mem), mem, key_split, [&](std::optional<relative_offset_ptr<Value>> val) {
            if (val.has_value()) {
                auto extracted_val = *val;
                if (extracted_val->type == ValueType::MAP) {
                    auto tmp = mem.relative_to_offset(std::get<Value::shmem_map_t>(extracted_val->data));
                    db_map::remove(tmp, mem, last_key.c_str());
                }
            }
        });
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