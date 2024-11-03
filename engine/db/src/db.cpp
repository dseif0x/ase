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
