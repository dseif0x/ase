/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "test_data.h"
#include <db.h>
#include <fstream>

template<typename T>
int run_test(const char* name, T&& generate_test_data, std::optional<std::ofstream> &out) {
    if (out && out->is_open()) *out << "DMA;" << name << ";";
    std::cout << "Running DMA Test: " << name << std::endl;
    DB::destroy();
    auto test_data = generate_test_data(0);
    std::cout << "test_data length: " << test_data.size() << std::endl;
    //for (auto [x,y] : test_data) std::cout << x << " : " << y << std::endl;
    DB db;
    stopwatch s;
    for (const auto& [key, value] : test_data) {
        //std::cout << "set on " << key << " : " << value << std::endl;
        CopiedValue newval{};
        newval.type = ValueType::STRING;
        newval.data.emplace<std::string>(value);
        db.set(key.c_str(), std::move(newval), true);
    }
    print_stopwatch(s, "\tSet Time: ", out);
    std::cout << "\tUsed Memory: " << db.get_used_memory() / 1024. / 1024. << " Mib" << std::endl;
    s.reset();
    for (const auto& [key, _] : test_data) {
        auto val = db.get(key.c_str());
    }
    print_stopwatch(s, "\tGet Time: ", out);
    if (out && out->is_open()) *out << std::endl;
    return 0;
}

#include "entrypoint.h"