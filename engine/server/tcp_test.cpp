/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <thread>
#include <unistd.h>
#include <chrono>
#include <stopwatch.h>

#include "tcp2_server.h"
#include "acl.h"


int main() {
    TCP2Server server{42069, 10};

    server.start();

    std::string temp;
    std::cin >> temp;

    server.stop();
    return 0;
}