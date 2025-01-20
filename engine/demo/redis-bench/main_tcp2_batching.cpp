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
int run_test(const char* name, T&& generate_test_data) {
    std::cout << "Running TCP2 Locally-Buffered - ASE Test: " << name << std::endl;
    auto test_data = generate_test_data();
    ReadWriteClient client{"127.0.0.1", 42069, "admin", "rootpwd"};
    client.set_mode_batching();

    std::this_thread::sleep_for(std::chrono::microseconds(100)); // time for old connections to clear
    stopwatch s;

    // generate data array
    std::vector<std::pair<std::string, CopiedValue>> data;
    data.reserve(test_data.size());
    std::transform(test_data.begin(), test_data.end(), std::back_inserter(data),
        [](const auto &pair) {
            CopiedValue newval{};
            newval.type = ValueType::STRING;
            newval.data.emplace<std::string>(pair.second);
            return std::make_pair(pair.first, std::move(newval));
        });

    // call all sets
    auto res = client.buffered_set(data);

    // check if all results were successfully and returned true
    if (!std::all_of(res.begin(), res.end(), [](bool b) { return b; })) std::cout << "ERROR" << std::endl;


    print_stopwatch(s, "\tSet Time: ");
    s.reset();

    // create a list of keys
    std::vector<std::string> keys;
    keys.reserve(data.size());

    std::transform(data.begin(), data.end(), std::back_inserter(keys),
        [](const auto& pair) {
            return pair.first;
        });

    // request all data at once
    auto vals = client.buffered_get(keys);

    // check if all data was received correctly
    bool error = false;
    if (vals.size() != data.size()) error = true;
    else {
        for (auto i = 0; i < vals.size(); i++) {
            const auto &a = vals[i];
            const auto &b = data[i].second; // could be used to make a few more checks; but some of the results are maps
            if (!a) {
                std::cout << "no value could be found for key " << data[i].first << std::endl;
                error = true;
                break;
            }
        }
    }
    if (error) std::cout << "ERROR" << std::endl;

    print_stopwatch(s, "\tGet Time: ");

    client.set_mode_normal();
    client.disconnect();
    return 0;
}

#include "entrypoint.h"