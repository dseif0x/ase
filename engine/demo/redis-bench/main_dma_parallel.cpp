/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "test_data.h"
#include <db.h>
#include <fstream>
#include <unordered_map>
#include <string>
#include <stopwatch.h>
#include <iostream>

void do_run(const std::string& name, const std::unordered_map<std::string, std::string>& test_data, std::optional<std::ofstream> &out, std::string bench_type) {
    DB db;
    std::stringstream ss;
    ss << "DMA par;" << bench_type << ";";
    stopwatch s;
    for (const auto& [key, value] : test_data) {
        CopiedValue newval{};
        newval.type = ValueType::STRING;
        newval.data.emplace<std::string>(value);
        db.set(key.c_str(), std::move(newval));
    }
    auto duration = s.peek();
    auto microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    //print_stopwatch(s, std::string("\t") + name + " Set Time: ", out);
    std::string temp = std::string("\t") + name + " Set Time: ";
    print_duration(microsduration, temp);
    ss << microsduration << ";";
    s.reset();
    for (const auto& [key, _] : test_data) {
        auto val = db.get(key.c_str());
    }
    duration = s.peek();
    microsduration = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    //print_stopwatch(s, std::string("\t") + name + " Set Time: ", out);
    std::string temp2 = std::string("\t") + name + " Get Time: ";
    print_duration(microsduration, temp2);
    ss << microsduration << ";\n";
    if (out && out->is_open()) *out << ss.str();
    //print_stopwatch(s, std::string("\t") + name + " Get Time: ", out);
}

template<typename T>
int run_test(const char* name, T&& generate_test_data, std::optional<std::ofstream> &out) {
    std::cout << "Running DMA Parallel Test: " << name << std::endl;
    DB::destroy();
    auto test_data = generate_test_data();

    constexpr int num_threads = 2;
    std::vector<std::unique_ptr<std::thread>> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(std::make_unique<std::thread>(do_run, std::string("Thread ") + std::to_string(i), test_data, std::ref(out), name));
    }

    for (auto& thread : threads) {
        thread->join();
    }
    return 0;
}

#include "entrypoint.h"