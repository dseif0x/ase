/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "test_data.h"
#include <db.h>
#include "read_write_client.h"

template<typename T>
int run_test(const char* name, T&& generate_test_data, std::optional<std::ofstream> &out) {
    if (out && out->is_open()) *out << "TCP2;" << name << ";";
    std::cout << "Running TCP2 - ASE Test: " << name << std::endl;
    auto test_data = generate_test_data(0);
    for (auto [x,y] : test_data) std::cout << x << " : " << y << std::endl;
    ReadWriteClient client{"127.0.0.1", 42069, "admin", "rootpwd"};
    std::this_thread::sleep_for(std::chrono::microseconds(100)); // time for old connections to clear
    stopwatch s;
    int ind = 0;
    for (const auto& [key, value] : test_data) {
        std::cout << ind++ << ": " << key << " - " << value << std::endl;
        CopiedValue newval{};
        newval.type = ValueType::STRING;
        newval.data.emplace<std::string>(value);
        client.simple_set(key, newval);
    }

    print_stopwatch(s, "\tSet Time: ", out);
    //client.clear_decoder();
    //std::cout << "\tUsed Memory: " << db.get_used_memory() / 1024. / 1024. << " Mib" << std::endl;
    s.reset();
    ind = 0;
    for (const auto& [key, _] : test_data) {
        //std::cout << ind++ << ": " << key << std::endl;
        auto val = client.simple_get(key);
    }
    print_stopwatch(s, "\tGet Time: ", out);
    if (out && out->is_open()) *out << std::endl;

    client.disconnect();
    return 0;
}

#include "entrypoint.h"