/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "test_data.h"
#include <db.h>

void do_run(const std::string& name, const std::unordered_map<std::string, std::string>& test_data, std::optional<std::ofstream> &out) {
    DB db;
    stopwatch s;
    for (const auto& [key, value] : test_data) {
        CopiedValue newval{};
        newval.type = ValueType::STRING;
        newval.data.emplace<std::string>(value);
        db.set(key.c_str(), std::move(newval));
    }
    print_stopwatch(s, std::string("\t") + name + " Set Time: ", out);
    s.reset();
    for (const auto& [key, _] : test_data) {
        auto val = db.get(key.c_str());
    }
    print_stopwatch(s, std::string("\t") + name + " Get Time: ", out);
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
        threads.emplace_back(std::make_unique<std::thread>(do_run, std::string("Thread ") + std::to_string(i), test_data, std::ref(out)));
    }

    for (auto& thread : threads) {
        thread->join();
    }
    return 0;
}

#include "entrypoint.h"