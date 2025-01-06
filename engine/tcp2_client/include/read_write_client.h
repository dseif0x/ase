/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#ifndef READ_WRITE_CLIENT_H
#define READ_WRITE_CLIENT_H

#include <string>

class ReadWriteClient {
    int val = 4;

public:
    ReadWriteClient(std::string url, int port);
};

inline ReadWriteClient::ReadWriteClient(std::string url, int port) {
}

#endif //READ_WRITE_CLIENT_H
