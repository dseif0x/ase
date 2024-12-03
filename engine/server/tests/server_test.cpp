/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <gtest/gtest.h>
#include "tcp2_server.h"

#include <string>
#include <cstring>
#include <utility>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

class TCPServerEnvironment final : public ::testing::Environment {
public:
    TCPServerEnvironment(const int port, const int max_clients) : port { port }, max_clients { max_clients } {
        this->server = new TCP2Server {port, max_clients};
    }

    void SetUp() override {
        server->start();
    }

    void TearDown() override {
        server->stop();
    }

private:
    int port;
    int max_clients;
    TCP2Server *server;
};

class DumbTCPClient final : public ::testing::Environment {
public:
    DumbTCPClient(std::string  host, int port) : host_(std::move(host)), port_(port), sock_fd_(-1) {}

    ~DumbTCPClient() override {
        if (sock_fd_ != -1) {
            close(sock_fd_);
        }
    }

    bool connect_to_server() {
        sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd_ == -1) {
            perror("Socket creation failed");
            return false;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);

        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            perror("Invalid address/Address not supported");
            return false;
        }

        if (connect(sock_fd_, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
            perror("Connection to server failed");
            return false;
        }

        return true;
    }

    bool send_message(const std::string& message) const {
        if (send(sock_fd_, message.c_str(), message.size(), 0) == -1) {
            perror("Send failed");
            return false;
        }
        return true;
    }

    std::string receive_message() const {
        char buffer[1024] = {0};
        int bytes_received = recv(sock_fd_, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            return "";
        }
        return std::string(buffer, bytes_received);
    }

    void close_connection() {
        if (sock_fd_ != -1) {
            close(sock_fd_);
            sock_fd_ = -1;
        }
    }

private:
    std::string host_;
    int port_;
    int sock_fd_;
};

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    // sets one global tcp server for all tests
    ::testing::AddGlobalTestEnvironment(new TCPServerEnvironment(12345, 10));
    return RUN_ALL_TESTS();
}

// dummy test to check if the communication between the server and the client works
TEST(TCPServerTest, SimpleClientCommunication) {
    // makes sure that the server has started before the tests run
    usleep(1000000);

    DumbTCPClient client("127.0.0.1", 12345);

    ASSERT_TRUE(client.connect_to_server());

    ASSERT_TRUE(client.send_message("Test"));

    std::string response = client.receive_message();
    ASSERT_FALSE(response.empty());

    const std::string anotherMessage = "Test 2";
    ASSERT_TRUE(client.send_message(anotherMessage));

    response = client.receive_message();
    ASSERT_FALSE(response.empty());

    client.close_connection();
}
