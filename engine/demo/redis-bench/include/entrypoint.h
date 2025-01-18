/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

int main() {
    run_test("32 Time 32 Times 16 Two Deep", get_test_data_32_time_32_times_16_two_deep);
    run_test("16 Time 1024 One Deep", get_test_data_16_time_1024_one_deep);
    run_test("1024 Time 16 One Deep", get_test_data_1024_time_16_one_deep);
    run_test("16384 Flat", get_test_data_16384_flat);
    run_test("1048576 Flat", get_test_data_1048576_flat);
}