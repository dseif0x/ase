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
    auto test_data = generate_test_data();
    ReadWriteClient client{"127.0.0.1", 42069};
    std::this_thread::sleep_for(std::chrono::microseconds(100)); // time for old connections to clear
    stopwatch s;

    auto res = client.buffered_set(test_data);
    // check if all results were successfully and returned true
    if (!std::all_of(res.begin(), res.end(), [](bool b) { return b; })) std::cout << "error" << std::endl;


    print_stopwatch(s, "\tSet Time: ");
    //client.clear_decoder();
    //std::cout << "\tUsed Memory: " << db.get_used_memory() / 1024. / 1024. << " Mib" << std::endl;
    s.reset();

    for (const auto& [key, _] : test_data) {
        //std::cout << ind++ << ": " << key << std::endl;
        auto val = client.simple_get(key);
    }
    print_stopwatch(s, "\tGet Time: ");

    client.disconnect();
    return 0;
}

#include "entrypoint.h"