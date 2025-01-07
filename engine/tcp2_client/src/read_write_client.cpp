//
// Created by Tobias Panek on 06.01.25.
//

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "read_write_client.h"

void ReadWriteClient::read_data(TCP2Decoder &decoder) {
    while (decoder.is_healthy()) decoder.read_data();
}

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

    this->decoder = std::make_unique<TCP2Decoder>(this->client_socket);
    this->encoder = std::make_unique<TCP2Encoder>(this->client_socket);

    this->reader = std::thread(ReadWriteClient::read_data, std::ref(*this->decoder));

    // login admin - rootpwd
    this->encoder->from_unsigned(2, 0);
    this->encoder->from_byte(0x0f);
    this->send_buffer();

    if (this->decoder->to_acknak() != 1) throw std::runtime_error("Failed to send client information");
    this->decoder->to_unsigned(5);
    this->decoder->to_unsigned(4);
    this->encoder->from_ack();
    this->encoder->from_string("admin");
    this->encoder->from_string("rootpwd");
    this->send_buffer();

    if (this->decoder->to_acknak() != 1) throw std::runtime_error("Failed to send user information");
    //std::cout << "Successfully logged in!" << std::endl;
}

ReadWriteClient::~ReadWriteClient() {
    this->decoder->set_kys();
    this->reader.join();
    close(this->client_socket);
}

bool ReadWriteClient::simple_set(const std::string &key, CopiedValue &val) {
    this->encoder->from_byte(0x02); // set command
    this->encoder->from_string(key);
    this->encoder->from_general(val);
    this->send_buffer();
    if (this->decoder->to_acknak() != 1) return false;
    return true;
}

std::optional<CopiedValue> ReadWriteClient::simple_get(const std::string &key) {
    this->encoder->from_byte(0x01); // get command
    this->encoder->from_string(key);
    send_buffer();
    if (this->decoder->to_acknak() != 1) return std::nullopt;
    auto val = this->decoder->to_general();
    if (!val) return std::nullopt;
    return val;
}

void ReadWriteClient::clear_decoder() {
    this->decoder->clear_buffer();
}

void ReadWriteClient::disconnect() {
    this->encoder->from_byte(0x0fc); //disconnect command
    this->send_buffer();
}
