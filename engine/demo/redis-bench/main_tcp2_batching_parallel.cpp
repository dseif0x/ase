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
    ss << "TCP2 bat par;" << bench_type << ";";

    client.set_mode_batching();

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


    auto duration = s.peek();
    auto microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    //print_stopwatch(s, std::string("\t") + name + " Set Time: ", out);
    std::string temp = std::string("\t") + name + " Set Time: ";
    print_duration(microsduration, temp);
    ss << microsduration << ";";
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
            if (false && !a) { // there are keys in the dataset which will overwrite subtrees, therefore it is not possible to find all answers; therefore this is deactivated
                std::cout << "no value could be found for key " << data[i].first << std::endl;
                error = true;
                break;
            }
        }
    }
    if (error) std::cout << "ERROR" << std::endl;

    duration = s.peek();
    microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    //print_stopwatch(s, std::string("\t") + name + " Set Time: ", out);
    std::string temp2 = std::string("\t") + name + " Get Time: ";
    print_duration(microsduration, temp2);
    ss << microsduration << ";\n";
    if (out && out->is_open()) *out << ss.str();

    client.set_mode_normal();

    client.disconnect();

    return 0;
}

template<typename T>
int run_test(const char* name, T&& generate_test_data, std::optional<std::ofstream> &out) {
    constexpr int num_threads = 2;
    std::cout << "Running TCP2 Locally-Buffered Parallel - ASE Test: " << name << std::endl;
    //auto test_data = generate_test_data(0);

    std::vector<std::unordered_map<std::string, std::string>> test_data;
    for (int x = 0; x < num_threads; x++) test_data.push_back(generate_test_data(x));

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
        threads.emplace_back(std::make_unique<std::thread>(do_run, std::string("Thread ") + std::to_string(i), test_data[i], std::ref(*clients[i]), std::ref(out), name));
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