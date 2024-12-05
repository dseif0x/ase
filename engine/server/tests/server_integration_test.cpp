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
#include <utils.h>

class TCPServerEnvironment final : public ::testing::Environment {
public:

    // TODO add acl and db to constructor
    TCPServerEnvironment(const int port, const int max_clients) : port { port }, max_clients { max_clients } {
        this->server = new TCP2Server {acl, db, port, max_clients};
    }

    void SetUp() override {
        server->start();
    }

    void TearDown() override {
        server->stop();
    }

private:
    ACL acl{};
    DB db;
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

    bool send_message(const std::vector<unsigned char>& message) const {
        if (send(sock_fd_, message.data(), message.size(), 0) == -1) {
            perror("Send failed");
            return false;
        }
        return true;
    }

    std::vector<unsigned char> receive_message(int timeout_ms) const {
        std::vector<unsigned char> buffer;
        const int chunk_size = 1024; // Size of each read
        auto start_time = std::chrono::steady_clock::now();

        while (true) {
            // Calculate remaining time
            auto elapsed_time = std::chrono::steady_clock::now() - start_time;
            int remaining_time_ms = timeout_ms - std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count();
            if (remaining_time_ms <= 0) {
                break; // Timeout expired
            }

            // Set up select with the remaining time
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(sock_fd_, &read_fds);

            struct timeval timeout;
            timeout.tv_sec = remaining_time_ms / 1000;
            timeout.tv_usec = (remaining_time_ms % 1000) * 1000;

            int activity = select(sock_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);

            if (activity == -1) {
                perror("Select failed");
                return {};
            } else if (activity == 0) {
                // No data ready, but time still remains
                continue;
            }

            // Socket is ready to read
            std::vector<unsigned char> temp_buffer(chunk_size, 0);
            int bytes_received = recv(sock_fd_, temp_buffer.data(), temp_buffer.size(), 0);
            if (bytes_received < 0) {
                perror("Receive failed");
                return {};
            } else if (bytes_received == 0) {
                break; // Socket was closed by the peer
            }

            // Append received data to the main buffer
            buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.begin() + bytes_received);
        }

        return buffer;
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
TEST(IntegrationTests_TCPServer, login_admin) {
    // the test needs to await the start of the server to pass, but with sleep the pipeline may get 'locked'
    //usleep(1000000);

    DumbTCPClient client("127.0.0.1", 12345);

    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};

    ASSERT_TRUE(client.send_message(client_information_package));

    auto server_information_package = client.receive_message(100);
    ASSERT_TRUE(server_information_package.size() == 10);

    // check actual data: ack + server information package
    std::vector<unsigned char> expected_server_info_pack{0xFF, 0x01, 0x04, 0x01, 0x08, 0x04, 0x40, 0x00, 0x40, 0x00};

    print_vec(expected_server_info_pack);
    print_vec(server_information_package);

    ASSERT_TRUE(vec_equ(expected_server_info_pack, server_information_package));

    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");
    ASSERT_TRUE(client.send_message(user_info_pack));

    auto logged_in = client.receive_message(100);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    client.close_connection();
}
