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

offset_ptr<db_key_map> DB::get_entrypoint() {
    return {mem.get_management_offset()};
}

std::optional<CopiedValue> DB::get(const char *key) {
    auto entrypoint_p = get_entrypoint();
    std::optional<CopiedValue> res{};
    entrypoint_p.atomic_access(mem, [&](db_key_map* entrypoint) {
        auto* tmp = entrypoint->get(key);
        if (tmp) {
            // TODO Traverse the hierarchy until no dots in the key remain and all values on the way are type map except for the last one
            res.emplace(*tmp->get(mem)); // Copy the value into the optional
        }
    });

    return res;
}
