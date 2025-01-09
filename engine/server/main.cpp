/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <thread>

#include "tcp2_server.h"


int main() {
    DB::destroy();
    std::string file_path = "test.bin";
    TCP2Server server{42069, 10};

    // load acl from disk
    if (server.load_acl_from_disk(file_path)) std::cout << "acl loaded from disk" << std::endl;
    else std::cout << "could not load from disk" << std::endl;

    // start server
    server.start();

    std::string temp;
    std::cin >> temp;

    // stop server
    server.stop();

    // save result to disk
    server.save_acl_to_disk(file_path);

    return 0;
}