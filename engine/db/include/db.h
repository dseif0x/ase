/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "custom_map.h"
#include "db_types.h"
#include "shmem.h"

using db_key_map = db_map<offset_ptr<Value>>;

struct DB_Meta {
    db_map<offset_ptr<Value>> entrypoint;
};

class DB {
public:
    DB();
    ~DB();
    offset_ptr<db_key_map> get_entrypoint();
private:
    shmem mem;
};