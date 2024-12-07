/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <gtest/gtest.h>
#include "tcp2_server.h"
#include "encoder.h"
#include "decoder.h"
#include "TestCoding.h"

#include <string>
#include <poll.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <utils.h>

void safe_sleep(unsigned int milliseconds) {
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::milliseconds(milliseconds);
    while (std::chrono::steady_clock::now() < end_time) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
}

class ServerPort {
public:
    static ServerPort& instance() {
        static ServerPort instance;
        return instance;
    }

    void increment() { ++counter_; }
    int get() const { return counter_; }

private:
    int counter_ = 42069;
    ServerPort() = default; // Prevent external instantiation
    ServerPort(const ServerPort&) = delete;
    ServerPort& operator=(const ServerPort&) = delete;
};

class IntegrationTests_TCPServer : public ::testing::Test {
public:
    IntegrationTests_TCPServer() : max_clients {63}, db(nullptr) {
    }

    void SetUp() override {
        this->db = new DB();
        ServerPort::instance().increment();
        std::cout << "PORT:" << ServerPort::instance().get() << std::endl;
        this->server = std::make_unique<TCP2Server>(this->acl, *this->db, ServerPort::instance().get(), this->max_clients);
        this->server->start();
    }

    void TearDown() override {
        server->stop();
        this->db->destroy();
        delete this->db;
    }

    DB* get_db() {
        return this->db;
    }

private:
    ACL acl{};
    DB* db;
    int max_clients;

    std::unique_ptr<TCP2Server> server;
};


class DumbTCPClient {
public:
    DumbTCPClient(std::string host, int port) : host_(std::move(host)), port_(port), sock_fd_(-1) {}

    ~DumbTCPClient() {
        if (sock_fd_ != -1) {
            close(sock_fd_);
        }
    }

    bool connect_to_server() {
        safe_sleep(100); // extra time for pipeline
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

    bool is_connection_open() {
        safe_sleep(100); // extra time for pipeline
        struct pollfd pfd;
        pfd.fd = this->sock_fd_;
        pfd.events = POLLIN; // Monitor for incoming data

        int result = poll(&pfd, 1, 0); // Non-blocking poll

        if (result > 0 && (pfd.revents & POLLIN)) {
            char buffer[1];
            ssize_t bytes = recv(this->sock_fd_, buffer, sizeof(buffer), MSG_PEEK);
            if (bytes == 0) {
                return false; // Server closed the connection
            }
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



TEST_F(IntegrationTests_TCPServer, login_admin) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());


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


TEST_F(IntegrationTests_TCPServer, login_unknown) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 04 616D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(100);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto res = client.receive_message(100);
    ASSERT_TRUE(res.size() >= 3);
    print_vec(res);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x02); // already connected

    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_invalid_passwd) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 06 726F6F707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(100);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto res = client.receive_message(100);
    ASSERT_TRUE(res.size() >= 3);
    print_vec(res);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x03); // already connected

    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_to_much_info) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F, 0xab};

    ASSERT_TRUE(client.send_message(client_information_package));
    auto res = client.receive_message(100);
    ASSERT_TRUE(res.size() >= 3);
    print_vec(res);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0xFF); // general protocol error

    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_invalid_version) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0xFF, 0xFF, 0x0F};

    ASSERT_TRUE(client.send_message(client_information_package));
    auto res = client.receive_message(100);
    ASSERT_TRUE(res.size() >= 3);
    print_vec(res);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x01); // already connected

    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_admin_and_stop) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(100);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(100);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // logged in

    ASSERT_TRUE(client.send_message({0xFE}));
    auto res = client.receive_message(100);
    ASSERT_TRUE(vec_equ(res, {0xFF}));
    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_admin_and_ping) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(100);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(100);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // logged in

    ASSERT_TRUE(client.send_message({0xFD, 0x01, 0x02, 0x03, 0x04}));
    auto res = client.receive_message(100);
    ASSERT_TRUE(vec_equ(res, {0xFF, 0x01, 0x02, 0x03, 0x04}));
    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_admin_and_disconnect) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(100);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(100);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // logged in

    ASSERT_TRUE(client.send_message({0xFC}));
    auto res = client.receive_message(100);
    ASSERT_TRUE(vec_equ(res, {0xFF, 0x08, 'G', 'o', 'o', 'd', 'b', 'y', 'e', '!'}));
    ASSERT_FALSE(client.is_connection_open());
    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_admin_twice) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(100);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(100);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    DumbTCPClient client2("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client2.connect_to_server());

    ASSERT_TRUE(client2.send_message(client_information_package));
    auto server_information_package2 = client2.receive_message(100);
    ASSERT_TRUE(server_information_package2.size() == 10);
    ASSERT_TRUE(server_information_package2[0] == 0xFF);
    ASSERT_TRUE(client2.send_message(user_info_pack));
    auto res = client2.receive_message(100);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x04); // already connected
}

TEST_F(IntegrationTests_TCPServer, login_admin_update_scope) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(100);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(100);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 61646D696E 09 246F72662E6E657773 03")));
    auto res = client.receive_message(100);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x00); // invalid key

    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 11646D696E 09 2E6F72662E6E657773 03")));
    res = client.receive_message(100);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x02); // user not found

    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 61646D696E 09 2E6F72662E6E657773 03")));
    res = client.receive_message(100);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF);

    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_admin_get) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(100);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(100);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    auto db = get_db();

    // add value to db
    CopiedValue newval5{};
    newval5.type = ValueType::ARRAY;
    auto& vector = newval5.data.emplace<CopiedValue::copied_array_t>();

    CopiedValue arrval1{};
    arrval1.type = ValueType::INT;
    arrval1.data.emplace<int>(13);
    CopiedValue arrval2{};
    arrval2.type = ValueType::INT;
    arrval2.data.emplace<int>(12);

    vector.emplace_back(std::move(arrval1));
    vector.emplace_back(std::move(arrval2));

    TestGenericEncoder enc{};
    enc.from_ack();
    enc.from_general(newval5);
    auto correct_get = enc._get_all_bytes();

    db->set(".test.sub.val", std::move(newval5), true);

    // correct access, get value
    ASSERT_TRUE(client.send_message(make_hex_vector("01 0d 2E746573742E7375622E76616C")));
    auto res = client.receive_message(100);
    ASSERT_TRUE(vec_equ(res, correct_get));

    // invalid key
    ASSERT_TRUE(client.send_message(make_hex_vector("01 0d 2E246573742E7375622E76616C")));
    res = client.receive_message(100);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x00); // invalid key

    // invalid key
    ASSERT_TRUE(client.send_message(make_hex_vector("01 0d 2E676573742E7375622E76616C")));
    res = client.receive_message(100);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x01); // invalid key

    // remove all permissions
    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 61646D696E 01 2e 00")));
    res = client.receive_message(100);
    ASSERT_TRUE(res[0] == 0xFF); // ack

    ASSERT_TRUE(client.send_message(make_hex_vector("01 0d 2E676573742E7375622E76616C")));
    res = client.receive_message(100);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x00); // permission denied

    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_admin_self_permission) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x00};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(100);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(100);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    ASSERT_TRUE(client.send_message(make_hex_vector("01 0d 2E246573742E7375622E76616C")));
    auto res = client.receive_message(250);
    print_vec(res);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0xF0); // self perm denied
}