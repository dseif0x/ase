/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "tcp2_utils.h"

#include <cstring>
#include <iostream>
#include <tcp2_server.h>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "encoder.h"


void tcp2_utils::client_acceptor(std::atomic<bool> &kys, ACL &acl, DB &db, int port, int max_clients) {
    // Create server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket.\n";
        return;
        //return false;
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(serverSocket);
        return;
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
                clientThreads.emplace_back(std::thread(tcp2_utils::client_coding_setup, std::ref(kill_all), clientSocket, std::ref(acl), std::ref(db)));
            }
        }
    }
    kill_all = true;
    for (auto &x : clientThreads) x.join();
    std::cout << "all joined" << std::endl;

    close(serverSocket);
    std::cout << "server closed" << std::endl;
}

void tcp2_utils::client_coding_setup(std::atomic<bool> &kys, int client_socket, ACL &acl, DB &db) {
    std::mutex pipe_lock;
    TCP2Decoder decoder{client_socket, &pipe_lock};
    TCP2Encoder encoder{client_socket, &pipe_lock};
    client_main_thread(kys, decoder, encoder, acl, db);
    close(client_socket);
    std::cout << "close is called" << std::endl;
}
