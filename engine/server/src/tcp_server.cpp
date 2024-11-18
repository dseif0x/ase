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
#include "tcp_server.h"
#include "tcp_coding.h"

#include "acl.h"
#include <atomic>
#include <deque>

// into db: client_worker,


void client_worker(std::atomic<bool>& kys, int clientSocket, std::mutex &pipe_lock, ACL &acl, ByteDecoder &decoder) {
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


void connect_client(std::atomic<bool>& kys, int clientSocket, ACL &acl) {
    char buffer[BUFFER_SIZE];
    ssize_t bytesReceived;
    std::atomic<bool> kys_worker{false};

    ByteDecoder decoder{&kys_worker};
    std::mutex pipe_lock;


    std::thread state(client_worker, std::ref(kys_worker), clientSocket, std::ref(pipe_lock), std::ref(acl), std::ref(decoder));

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

void TCPServer::accept_clients(std::atomic<bool> &kys, ACL &acl, int port, int max_clients) {
    // Create server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket.\n";
        return;
        //return false;
    }

    // Configure server
    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    // Bind the socket
    if (bind(serverSocket, reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind to port " << port << ".\n";
        close(serverSocket);
        //return false;
        return;
    }

    // Start listening for incoming connections
    if (listen(serverSocket, std::min(SOMAXCONN, max_clients)) < 0) {
        std::cerr << "Failed to listen on socket.\n";
        close(serverSocket);
        //return false;
        return;
    }

    // temp output todo remove
    std::cout << "Server listening on port " << port << "...\n";

    // Vector to hold client threads
    std::vector<std::thread> clientThreads;
    std::atomic<bool> kill_all{false};

    struct timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    fd_set read_fds;
    // Accept connections in a loop
    while (!kys) {
        FD_ZERO(&read_fds);
        FD_SET(serverSocket, &read_fds);

        int activity = select(serverSocket + 1, &read_fds, nullptr, nullptr, &timeout);

        if (activity < 0 && errno != EINTR) {
            std::cerr << "Select error: " << strerror(errno) << std::endl;
            break;
        }

        if (FD_ISSET(serverSocket, &read_fds)) {
            struct sockaddr_in clientAddress{};
            socklen_t clientAddressLength = sizeof(clientAddress);

            // Accept an incoming connection
            int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr *>(&clientAddress), &clientAddressLength);
            if (clientSocket < 0) {
                std::cerr << "Failed to accept client connection: " << strerror(errno) << std::endl;
            } else {
                // temp output todo remove
                std::cout << "Connected to client: " << inet_ntoa(clientAddress.sin_addr)
                          << ":" << ntohs(clientAddress.sin_port) << "\n";

                // start thread for the connection
                clientThreads.emplace_back(std::thread(connect_client, std::ref(kill_all), clientSocket, std::ref(acl)));
            }
        }
    }
    kill_all = true;
    for (auto &x : clientThreads) x.join();

    close(serverSocket);
}

TCPServer::TCPServer(int port, int max_clients): port(port), max_clients(max_clients) {
}



void TCPServer::start() {
    this->client_acceptor = std::thread(TCPServer::accept_clients, std::ref(this->kill_acceptor), std::ref(this->acl), this->port, this->max_clients);
}

void TCPServer::stop() {
    this->kill_acceptor = true;
    this->client_acceptor.join();
}
