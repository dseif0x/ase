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

#include <acl.h>
#include <atomic>
#include <deque>



void client_worker(std::atomic<bool>& kys, int clientSocket, std::mutex &pipe_lock, ACL &acl, ByteDecoder &decoder) {
    ByteEncoder encoder{};
    int state = 0;
    while (!kys) {
        if (state == 0) {
            // Expecting client information
            auto client_version = decoder.decode_unsigned(2);
            auto client_perms = decoder.decode_unsigned(1);
            std::cout << "client is running version: " << client_version << std::endl;
            std::cout << "client opens connection with intention to: " <<((client_perms & 0x01) > 0 ? "read, " : "") <<
                    ((client_perms & 0x02) > 0 ? "write, " : "") << ((client_perms & 0x04) > 0 ? "delete, " : "") << "[" << client_perms << "]" << std::endl;
            if (client_version != 0) {

            }
            encoder.encode_acknak(true);
            encoder.encode_unsigned(1, 1); // size of bool
            encoder.encode_unsigned(1, sizeof(int)); // size of int
            encoder.encode_unsigned(1, sizeof(char)); // size of char
            encoder.encode_unsigned(1, sizeof(double)); // size of double
            encoder.encode_unsigned(1, sizeof(int)); // size of semaphore // todo send actual size
            state = 1;
        } else if (state == -1) {
            if (encoder.empty()) break;
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
}


void connect_client(std::atomic<bool>& kys, int clientSocket, ACL &acl) {
    char buffer[BUFFER_SIZE];
    ssize_t bytesReceived;

    ByteDecoder decoder{};
    std::mutex pipe_lock;

    std::atomic<bool> kys_worker{false};
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
            std::cout << "timeout or read" << std::endl;
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
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

void TCPServer::accept_clients(std::atomic<bool> &kys, ACL &acl, int serverSocket) {
    // Vector to hold client threads
    std::vector<std::thread> clientThreads;
    std::atomic<bool> kill_all{false};

    // Accept connections in a loop
    while (true) {
        struct sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);

        // Accept an incoming connection
        int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr *>(&clientAddress), &clientAddressLength);
        if (clientSocket < 0) {
            std::cerr << "Failed to accept client connection: " << strerror(errno) << std::endl;
            continue;
        }

        // temp output todo remove
        std::cout << "Connected to client: " << inet_ntoa(clientAddress.sin_addr)
                  << ":" << ntohs(clientAddress.sin_port) << "\n";

        // start thread for the connection
        clientThreads.emplace_back(std::thread(connect_client, std::ref(kill_all), clientSocket, std::ref(acl)));
        break;
    }

    kill_all = true;
    for (auto &x : clientThreads) x.join();
}

TCPServer::TCPServer(int port, int max_clients): port(port), max_clients(max_clients) {
}



bool TCPServer::start() {
    // Create server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket.\n";
        return false;
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
        return false;
    }

    // Start listening for incoming connections
    if (listen(serverSocket, std::min(SOMAXCONN, this->max_clients)) < 0) {
        std::cerr << "Failed to listen on socket.\n";
        close(serverSocket);
        return false;
    }

    // temp output todo remove
    std::cout << "Server listening on port " << port << "...\n";
    this->server_socket = server_socket;
    this->client_acceptor = std::thread(TCPServer::accept_clients, std::ref(this->kill_acceptor), std::ref(this->acl), this->server_socket);

    return true;
}

void TCPServer::stop() {
    this->kill_acceptor = true;
    this->client_acceptor.join();
    // Close the server socket
    close(this->server_socket);
}
