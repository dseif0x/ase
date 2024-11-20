/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "db.h"

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

std::optional<CopiedValue> DB::get(const char *key) {
    auto entrypoint_p = get_entrypoint();
    std::optional<CopiedValue> res{};
    entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
        if (entrypoint->has(key)) {
            auto tmp = entrypoint->get(key);
            // TODO Traverse the hierarchy until no dots in the key remain and all values on the way are type map except for the last one
            res.emplace(*tmp); // Copy the value into the optional
        }
    });

    return res;
}

void DB::set(const char *key, const CopiedValue& value) {
    auto entrypoint_p = get_entrypoint();
    offset_ptr<Value> new_value = mem.construct<Value>(mem, value);
    db_key_map::set(entrypoint_p, mem, key, new_value.get_relative(mem));
}
