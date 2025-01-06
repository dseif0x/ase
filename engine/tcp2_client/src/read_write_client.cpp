//
// Created by Tobias Panek on 06.01.25.
//

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "read_write_client.h"

ReadWriteClient::ReadWriteClient(const std::string &ip, int port):
    server_ip(ip), server_port(port), client_socket(-1) {
    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        throw std::runtime_error("Failed to create socket");
    }

    // Configure server address
    sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(server_port);

    // Convert IP address to binary format
    if (inet_pton(AF_INET, server_ip.c_str(), &serverAddress.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(client_socket);
        throw std::runtime_error("Invalid server address");
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Connection failed");
        close(client_socket);
        throw std::runtime_error("Failed to connect to server");
    }

    std::cout << "Connected to server at " << server_ip << ":" << server_port << std::endl;

    this->decoder = std::make_unique<TCP2Decoder>(this->client_socket, &this->pipe_lock);
    this->encoder = std::make_unique<TCP2Encoder>(this->client_socket, &this->pipe_lock);
}
