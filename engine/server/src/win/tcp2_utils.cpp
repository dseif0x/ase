/*
 * Copyright 2025 Project Contributors
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
#include <winsock2.h>

#include "encoder.h"
#include "acl.h"


void tcp2_utils::client_acceptor(std::atomic<bool> &kys, ASE::ACL &acl, DB &db, int port, int max_clients) {
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        std::cerr << "Could not find any WinSock DLL.\n";
        return;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        std::cerr << "Could not find a usable WinSock DLL.\n";
        WSACleanup();
        return;
    }

    // Create server socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == SOCKET_ERROR) {
        std::cerr << "Failed to create socket. Error: " << WSAGetLastError() << std::endl;
        return;
        //return false;
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)) == SOCKET_ERROR) {
        std::cerr << "setsockopt failed. Error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        return;
    }

    // Configure server
    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    // Bind the socket
    if (bind(serverSocket, reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind to port " << port << ". Error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        //return false;
        return;
    }

    // Start listening for incoming connections
    if (listen(serverSocket, std::min(SOMAXCONN, max_clients)) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on socket. Error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
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

        int activity = select(0, &read_fds, nullptr, nullptr, &timeout);

        if (activity == SOCKET_ERROR) {
            std::cerr << "Select error: " << WSAGetLastError() << std::endl;
            break;
        }

        if (FD_ISSET(serverSocket, &read_fds)) {
            struct sockaddr_in clientAddress{};
            int  clientAddressLength = sizeof(clientAddress);

            // Accept an incoming connection
            SOCKET clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr *>(&clientAddress), &clientAddressLength);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Failed to accept client connection: " << WSAGetLastError() << std::endl;
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

    closesocket(serverSocket);
    std::cout << "server closed" << std::endl;

    WSACleanup(); // Clean up Winsock
}

void tcp2_utils::client_coding_setup(std::atomic<bool> &kys, SOCKET client_socket, ASE::ACL &acl, DB &db) {
    TCP2Decoder decoder{client_socket};
    TCP2Encoder encoder{client_socket};
    client_main_thread(kys, decoder, encoder, acl, db);
    closesocket(client_socket);
    std::cout << "close is called" << std::endl;
}
