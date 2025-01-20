/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <db.h>

int main() {
    DB::destroy();
    std::cout << "DB clean up finished" << std::endl;
    return 0;
}