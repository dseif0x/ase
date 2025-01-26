/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "test_data.h"
#include <db.h>
#include "read_write_client.h"

int do_run(const std::string& name, const std::unordered_map<std::string, std::string>& test_data, ReadWriteClient &client, std::optional<std::ofstream> &out, std::string bench_type) {
    std::stringstream ss;
    ss << "TCP2 par;" << bench_type << ";";

    stopwatch s;
    int ind = 0;
    for (const auto& [key, value] : test_data) {
        //std::cout << ind++ << ": " << key << " - " << value << std::endl;
        CopiedValue newval{};
        newval.type = ValueType::STRING;
        newval.data.emplace<std::string>(value);
        client.simple_set(key, newval);
    }

    auto duration = s.peek();
    auto microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    //print_stopwatch(s, std::string("\t") + name + " Set Time: ", out);
    std::string temp = std::string("\t") + name + " Set Time: ";
    print_duration(microsduration, temp);
    ss << microsduration << ";";
    s.reset();
    ind = 0;
    for (const auto& [key, _] : test_data) {
        //std::cout << ind++ << ": " << key << std::endl;
        auto val = client.simple_get(key);
    }
    duration = s.peek();
    microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    //print_stopwatch(s, std::string("\t") + name + " Set Time: ", out);
    std::string temp2 = std::string("\t") + name + " Get Time: ";
    print_duration(microsduration, temp2);
    ss << microsduration << ";\n";
    if (out && out->is_open()) *out << ss.str();

    client.disconnect();
    return 0;
}

template<typename T>
int run_test(const char* name, T&& generate_test_data, std::optional<std::ofstream> &out) {
    constexpr int num_threads = 2;
    std::cout << "Running TCP2 Parallel - ASE Test: " << name << std::endl;
    auto test_data = generate_test_data(0);

    std::vector<std::unique_ptr<ReadWriteClient>> clients;

    // create users and connect
    ReadWriteClient client{"127.0.0.1", 42069, "admin", "rootpwd"};
    for (int x = 0; x < num_threads; x++) {
        client.create_new_admin("admin"+std::to_string(x), "pwd");
        clients.emplace_back(std::make_unique<ReadWriteClient>("127.0.0.1", 42069, "admin"+std::to_string(x), "pwd"));
    }

    std::vector<std::unique_ptr<std::thread>> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(std::make_unique<std::thread>(do_run, std::string("Thread ") + std::to_string(i), test_data, std::ref(*clients[i]), std::ref(out), name));
    }

    for (auto& thread : threads) {
        thread->join();
    }

    // delete users from server
    for (int x = 0; x < num_threads; x++) {
        client.delete_user("admin"+std::to_string(x));
    }

    client.disconnect();

    return 0;
}

#include "entrypoint.h"