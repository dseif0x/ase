/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <chrono>
#include <stopwatch.h>

#include "tcp2_server.h"
#include "acl.h"


int main() {
    std::cout << getpid() << std::endl;


    TCP2Server server{42069, 10};

    stopwatch s{};
    server.start();

    std::string temp;
    std::cin >> temp;
    server.stop();

    auto dur = s.stop();
    std::cout << dur << std::endl;
}