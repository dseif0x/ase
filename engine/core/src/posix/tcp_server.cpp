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
#include <deque>



void client_worker(int clientSocket, ACL &acl, ByteDecoder &decoder) {
    // Expecting client information
    auto client_version = decoder.decode_unsigned(2);
    auto client_perms = decoder.decode_unsigned(1);
    std::cout << "client is running version: " << client_version << std::endl;
    std::cout << "client opens connection with intention to: " <<((client_perms & 0x01) > 0 ? "read, " : "") <<
            ((client_perms & 0x02) > 0 ? "write, " : "") << ((client_perms & 0x04) > 0 ? "delete, " : "") << "[" << client_perms << "]" << std::endl;

}


void connect_client(int clientSocket, ACL &acl) {
    char buffer[BUFFER_SIZE];
    ssize_t bytesReceived;

    ByteDecoder decoder{};
    std::mutex pipe_lock;

    std::thread state(client_worker, clientSocket, std::ref(acl), std::ref(decoder));
    //std::thread state(justAtest, 1, 2, 3, &queue_lock);

    fd_set readfds;
    struct timeval timeout{};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    bool done = false;
    while (!done) {
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
                for (int x = 0; x < BUFFER_SIZE; x++) std::cout << "[" << (int)(unsigned char)buffer[x] << "]";
                std::cout << std::endl;
                std::vector<unsigned char> unsigned_buffer(buffer, buffer + bytesReceived);
                decoder.add_to_buffer(unsigned_buffer);
            }
        } else if (sel == 0) {
            // no data
        } else {
            std::cerr << "select error: " << strerror(errno) << std::endl;
            done = true;
        }
    }
    state.join();

    close(clientSocket);
}

int start_tcp_server(int port, ACL &acl) {
    // Create server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket.\n";
        return -1;
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
        return -1;
    }

    // Start listening for incoming connections
    if (listen(serverSocket, SOMAXCONN) < 0) {
        std::cerr << "Failed to listen on socket.\n";
        close(serverSocket);
        return -1;
    }

    // temp output todo remove
    std::cout << "Server listening on port " << port << "...\n";

    // Vector to hold client threads
    std::vector<std::thread> clientThreads;

    // Accept connections in a loop
    while (true) {
        struct sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);

        // Accept an incoming connection
        int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr *>(&clientAddress), &clientAddressLength);
        if (clientSocket < 0) {
            std::cerr << "Failed to accept client connection.\n";
            continue;
        }

        // temp output todo remove
        std::cout << "Connected to client: " << inet_ntoa(clientAddress.sin_addr)
                  << ":" << ntohs(clientAddress.sin_port) << "\n";

        // start thread for the connection
        clientThreads.emplace_back(std::thread(connect_client, clientSocket, std::ref(acl)));
    }

    // Close the server socket
    close(serverSocket);

    return 0;
}