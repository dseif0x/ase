/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "test_data.h"
#include <db.h>
#include "read_write_client.h"

template<typename T>
int run_test(const char* name, T&& generate_test_data) {
    std::cout << "Running TCP2 - ASE Test: " << name << std::endl;
    //DB::destroy();
    auto test_data = generate_test_data();
    //DB db;
    ReadWriteClient client{"127.0.0.1", 42069};
    stopwatch s;
    for (const auto& [key, value] : test_data) {
        CopiedValue newval{};
        newval.type = ValueType::STRING;
        newval.data.emplace<std::string>(value);
        db.set(key.c_str(), std::move(newval));
        client.simple_set(key, newval);
    }
    print_stopwatch(s, "\tSet Time: ");
    std::cout << "\tUsed Memory: " << db.get_used_memory() / 1024. / 1024. << " Mib" << std::endl;
    s.reset();
    for (const auto& [key, _] : test_data) {
        auto val = db.get(key.c_str());
    }
    print_stopwatch(s, "\tGet Time: ");
    return 0;
}

#include "entrypoint.h"