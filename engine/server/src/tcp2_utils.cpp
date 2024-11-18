/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "tcp2_utils.h"
#include <iostream>
#include <tcp2_server.h>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/_select.h>


void tcp2_utils::client_state_thread(std::atomic<bool>& kys, int clientSocket, std::mutex &pipe_lock, ACL &acl, ByteDecoder &decoder) {
    ByteEncoder encoder{};
    std::string username;

    int state = 0;
    while (!kys) {
        std::cout << "state:" << state << std::endl;
        if (state < 0) {
            if (encoder.empty()) break;
        } else if (state == 0) {
            // Expecting client information
            auto client_version = decoder.decode_unsigned(2);
            auto client_perms = decoder.decode_unsigned(1);
            std::cout << "client is running version: " << client_version << std::endl;
            std::cout << "client opens connection with intention to: " <<((client_perms & 0x01) > 0 ? "read, " : "") <<
                    ((client_perms & 0x02) > 0 ? "write, " : "") << ((client_perms & 0x04) > 0 ? "delete, " : "") << "[" << client_perms << "]" << std::endl;
            if (!decoder.empty()) {
                state = -1;
                encoder.encode_nak();
                encoder.encode_error(0xFFFF);
                continue;
            }
            if (client_version != 0) {
                state = -1;
                encoder.encode_nak();
                encoder.encode_error(0x8000);
                continue;
            }
            encoder.encode_ack();
            encoder.encode_unsigned(1, 1); // size of bool
            encoder.encode_unsigned(1, sizeof(int)); // size of int
            encoder.encode_unsigned(1, sizeof(char)); // size of char
            encoder.encode_unsigned(1, sizeof(double)); // size of double
            encoder.encode_unsigned(1, sizeof(int)); // size of semaphore // todo send actual size
            state = 1;
        } else if (state == 1) {
            if (decoder.decode_acknak() != 1) {
                state = -1;
                encoder.encode_error(0xFFFF);
                continue;
            }
            username = decoder.decode_string();
            std::string pwd = decoder.decode_string();
            int login_res = acl.try_login(username, pwd);
            if (login_res != 0) {
                state = -1;
                encoder.encode_nak();
                if (login_res == 1) encoder.encode_error(0x8001);
                else if (login_res == 2) encoder.encode_error(0x8003);
                else if (login_res == 3) encoder.encode_error(0x8002);
                continue;
            }
            encoder.encode_ack();
        }
        // send buffer
        while (!encoder.empty()) {
            auto data = encoder.get_bytes(BUFFER_SIZE);
            std::cout << "sending data:";
            for (int x = 0; x < data.second; x++) std::cout << "[" << (int)(unsigned char)data.first.get()[x] << "]";
            std::cout << std::endl;
            {
                std::cout << "requesting lock" << std::endl;
                std::lock_guard<std::mutex> lock(pipe_lock);
                send(clientSocket, data.first.get(), data.second, 0);
            }
            std::cout << "data sent" << std::endl;
            // todo add checks if a stop command has come in
        }
    }
    acl.disconnect(username);
}


void tcp2_utils::client_main_thread(std::atomic<bool>& kys, int clientSocket, ACL &acl) {
    char buffer[BUFFER_SIZE];
    ssize_t bytesReceived;
    std::atomic<bool> kys_worker{false};

    ByteDecoder decoder{&kys_worker};
    std::mutex pipe_lock;


    std::thread state(tcp2_utils::client_state_thread, std::ref(kys_worker), clientSocket, std::ref(pipe_lock), std::ref(acl), std::ref(decoder));

    fd_set readfds;
    struct timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    bool done = false;
    while (!done && !kys) {
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        int sel;
        {
            std::lock_guard<std::mutex> lock(pipe_lock);
            sel = select(clientSocket + 1, &readfds, nullptr, nullptr, &timeout);
        }
        if (sel > 0) {
            bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            if (bytesReceived == 0) {
                std::cout << "Client disconnected.\n";
                done = true;
            } else if (bytesReceived < 0) {
                std::cerr << "recv error: " << strerror(errno) << std::endl;
                done = true;
            } else {
                std::cout << "Bytes: ";
                for (int x = 0; x < bytesReceived; x++) std::cout << "[" << (int)(unsigned char)buffer[x] << "]";
                std::cout << std::endl;
                std::vector<unsigned char> unsigned_buffer(buffer, buffer + bytesReceived);
                decoder.add_to_buffer(unsigned_buffer);
                //send(clientSocket, buffer, bytesReceived, 0);
            }
        } else if (sel == 0) {
            // in case of timeout sleep for a bit to give others time to access pipe mutex
            // this way large data pack would not be slowed down, as we only sleep in case of nothing arriving
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // todo replace with yield
        } else {
            std::cerr << "select error: " << strerror(errno) << std::endl;
            done = true;
        }
    }

    // tell worker to kill itself and join thread
    kys_worker = true;
    state.join();

    close(clientSocket);
}

