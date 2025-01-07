/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#ifndef READ_WRITE_CLIENT_H
#define READ_WRITE_CLIENT_H

#include <string>
#include "encoder.h"
#include "decoder.h"

class ReadWriteClient {
    int client_socket;
    std::string server_ip;
    int server_port;

    std::unique_ptr<TCP2Decoder> decoder;
    std::unique_ptr<TCP2Encoder> encoder;

    std::thread reader;

    static void read_data(TCP2Decoder &decoder);

public:
    ReadWriteClient(const std::string &ip, int port);
    ~ReadWriteClient();
    bool simple_set(const std::string &key, CopiedValue &val);
    std::optional<CopiedValue> simple_get(const std::string &key);

    void clear_decoder();
    void disconnect();

    void send_buffer() {
        while (!this->encoder->is_empty()) this->encoder->write_data(); // send command
    }
};

#endif //READ_WRITE_CLIENT_H
