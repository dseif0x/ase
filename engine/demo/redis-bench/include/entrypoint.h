/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <fstream>
#include <test_data.h>

// output file format: test; data; set; get;

int main(int argc, char **argv) {
    std::optional<std::ofstream> out;
    if (argc == 2) {
        out.emplace(argv[1], std::ios::app);
        if (!out->is_open()) return 1;
    } else {
        out = std::nullopt;
    }

    run_test("32 Times 32 Times 16 Two Deep",
        [](const int process){return get_test_data_32_time_32_times_16_two_deep();}, out);
    run_test("16 Times 1024 One Deep",
        [](const int process){return get_test_data_16_time_1024_one_deep();}, out);
    run_test("1024 Times 16 One Deep",
        [](const int process){return get_test_data_1024_time_16_one_deep();}, out);
    run_test("16384 Flat",
        [](const int process){return get_test_data_16384_flat();}, out);
    run_test("16384 random keys, max-depth: 5",
        [](const int process){return get_unique_keys(process, 16384, 5);}, out);
    run_test("65536 random keys; max-depth: 10",
        [](const int process){return get_unique_keys(process, 65536, 10);}, out);
    //run_test("1048576 Flat", get_test_data_1048576_flat, out);
}
