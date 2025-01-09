/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#if defined(UNIX)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#elif defined(WIN)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "encoder.h"
#include <gtest/gtest.h>
#include "tcp2_server.h"
#include "TestCoding.h"
#include "acl.h"

#include <string>
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
        {
            DB db{};
            db.destroy();
        }
    }

    void SetUp() override {
        this->db = new DB();
        ServerPort::instance().increment();
        std::cout << "PORT:" << ServerPort::instance().get() << std::endl;
        this->server = std::make_unique<TCP2Server>(ServerPort::instance().get(), this->max_clients);
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

    bool save_acl(const std::string &path) {
        this->server->stop();
        bool succ = this->server->save_acl_to_disk(path);
        this->server->start();
        return succ;
    }

    bool read_acl(const std::string &path) {
        this->server->stop();
        bool succ = this->server->load_acl_from_disk(path);
        this->server->start();
        return succ;
    }

private:
    ASE::ACL acl{};
    DB* db;
    int max_clients;

    std::unique_ptr<TCP2Server> server;
};

class IntegrationTests_TCPServerManual : public ::testing::Test {
public:
    IntegrationTests_TCPServerManual() : max_clients {63}, db(nullptr) {
        {
            DB db{};
            db.destroy();
        }
    }

    void SetUp() override {
        this->db = new DB();
        ServerPort::instance().increment();
        std::cout << "PORT:" << ServerPort::instance().get() << std::endl;
        this->server = std::make_unique<TCP2Server>(ServerPort::instance().get(), this->max_clients);
    }

    void TearDown() override {
        this->db->destroy();
        delete this->db;
    }

    void server_start() {
        this->server->start();
    }

    void server_stop() {
        this->server->stop();
    }

    DB* get_db() {
        return this->db;
    }

    bool save_acl(const std::string &path) {
        return this->server->save_acl_to_disk(path);
    }

    bool read_acl(const std::string &path) {
        return this->server->load_acl_from_disk(path);
    }

private:
    ASE::ACL acl{};
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

    bool is_connection_open();

    bool send_message(const std::vector<unsigned char>& message) const;

    std::vector<unsigned char> receive_message(int timeout_ms) const;

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

#if defined(UNIX)
bool DumbTCPClient::is_connection_open() {
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

bool DumbTCPClient::send_message(const std::vector<unsigned char>& message) const {
    if (send(sock_fd_, message.data(), message.size(), 0) == -1) {
        perror("Send failed");
        return false;
    }
    return true;
}

std::vector<unsigned char> DumbTCPClient::receive_message(int timeout_ms) const {
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
#elif defined(WIN)
bool DumbTCPClient::is_connection_open() {
    safe_sleep(100); // extra time for pipeline

    short flag = POLLRDNORM | POLLRDBAND;

    WSAPOLLFD pfd;
    pfd.fd = this->sock_fd_;
    pfd.events = flag; // Equivalent to POLLIN for normal data
    pfd.revents = 0;

    int result = WSAPoll(&pfd, 1, 0); // Non-blocking poll

    if (result > 0 && (pfd.revents & flag)) {
        char buffer[1];
        int bytes = recv(this->sock_fd_, buffer, sizeof(buffer), MSG_PEEK);
        if (bytes == 0) {
            return false; // Server closed the connection
        }
    }

    if (pfd.revents & (POLLERR | POLLHUP)) return false;

    return result >= 0; // Connection is open if WSAPoll did not fail
}

bool DumbTCPClient::send_message(const std::vector<unsigned char>& message) const {
    int result = send(sock_fd_, reinterpret_cast<const char*>(message.data()), message.size(), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

std::vector<unsigned char> DumbTCPClient::receive_message(int timeout_ms) const {
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

        // Set up fd_set for select
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock_fd_, &read_fds);

        // Set up timeout for select
        struct timeval timeout;
        timeout.tv_sec = remaining_time_ms / 1000;
        timeout.tv_usec = (remaining_time_ms % 1000) * 1000;

        // Check for readability
        int activity = select(0, &read_fds, nullptr, nullptr, &timeout); // "0" is ignored in Winsock

        if (activity == SOCKET_ERROR) {
            std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
            return {};
        } else if (activity == 0) {
            // No data ready, but time still remains
            continue;
        }

        // Socket is ready to read
        std::vector<unsigned char> temp_buffer(chunk_size, 0);
        int bytes_received = recv(sock_fd_, reinterpret_cast<char*>(temp_buffer.data()), temp_buffer.size(), 0);
        if (bytes_received == SOCKET_ERROR) {
            std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
            return {};
        } else if (bytes_received == 0) {
            break; // Socket was closed by the peer
        }

        // Append received data to the main buffer
        buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.begin() + bytes_received);
    }

    return buffer;
}
#endif

constexpr int wait_time_success = 150;
constexpr int wait_time_error = wait_time_success + 250; // the server will wait for 250 seconds, therefore we have to also wait 250 more

TEST_F(IntegrationTests_TCPServer, login_admin) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());


    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};

    ASSERT_TRUE(client.send_message(client_information_package));

    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);

    // check actual data: ack + server information package
    std::vector<unsigned char> expected_server_info_pack{0xFF, 0x01, 0x04, 0x01, 0x08, 0x04, 0x40, 0x00, 0x40, 0x00};

    print_vec(expected_server_info_pack);
    print_vec(server_information_package);

    ASSERT_TRUE(vec_equ(expected_server_info_pack, server_information_package));

    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");
    ASSERT_TRUE(client.send_message(user_info_pack));

    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    client.close_connection();
}


TEST_F(IntegrationTests_TCPServer, login_unknown) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 04 616D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto res = client.receive_message(wait_time_error);
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
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto res = client.receive_message(wait_time_error);
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
    auto res = client.receive_message(wait_time_error);
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
    auto res = client.receive_message(wait_time_error);
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
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // logged in

    ASSERT_TRUE(client.send_message({0xFE}));
    auto res = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(res, {0xFF}));
    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_admin_and_ping) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // logged in

    ASSERT_TRUE(client.send_message({0xFD, 0x01, 0x02, 0x03, 0x04}));
    auto res = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(res, {0xFF, 0x01, 0x02, 0x03, 0x04}));
    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_admin_and_disconnect) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // logged in

    ASSERT_TRUE(client.send_message({0xFC}));
    auto res = client.receive_message(wait_time_success);
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
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    DumbTCPClient client2("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client2.connect_to_server());

    ASSERT_TRUE(client2.send_message(client_information_package));
    auto server_information_package2 = client2.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package2.size() == 10);
    ASSERT_TRUE(server_information_package2[0] == 0xFF);
    ASSERT_TRUE(client2.send_message(user_info_pack));
    auto res = client2.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x04); // already connected
}

TEST_F(IntegrationTests_TCPServer, login_admin_update_scope) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 61646D696E 09 246F72662E6E657773 03")));
    auto res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x00); // invalid key

    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 11646D696E 09 2E6F72662E6E657773 03")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x02); // user not found

    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 61646D696E 09 2E6F72662E6E657773 03")));
    res = client.receive_message(wait_time_success);
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
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
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
    auto res = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(res, correct_get));

    // invalid key
    ASSERT_TRUE(client.send_message(make_hex_vector("01 0d 2E246573742E7375622E76616C")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x00); // invalid key

    // invalid key
    ASSERT_TRUE(client.send_message(make_hex_vector("01 0d 2E676573742E7375622E76616C")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x01); // key not found

    // remove all permissions
    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 61646D696E 01 2e 00")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res[0] == 0xFF); // ack

    ASSERT_TRUE(client.send_message(make_hex_vector("01 0d 2E676573742E7375622E76616C")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x00); // permission denied

    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_admin_self_permission) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x00};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    ASSERT_TRUE(client.send_message(make_hex_vector("01 0d 2E246573742E7375622E76616C")));
    auto res = client.receive_message(wait_time_error);
    print_vec(res);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0xF0); // self perm denied
}

TEST_F(IntegrationTests_TCPServer, login_admin_set) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    auto db = get_db();

    //correct hex: "02 0d 2E746573742E7375622E76616C 03 07 7465737476616C"

    // invalid key
    ASSERT_TRUE(client.send_message(make_hex_vector("02 0d 2E246573742E7375622E76616C 03 07 7465737476616C")));
    auto res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x00); // invalid key

    // correct access, set value
    ASSERT_TRUE(client.send_message(make_hex_vector("02 0d 2E746573742E7375622E76616C 03 07 7465737476616C")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(res, {0xFF}));
    auto val_db = db->get(".test.sub.val");
    ASSERT_TRUE(val_db);
    ASSERT_TRUE(val_db->type == ValueType::STRING);
    ASSERT_TRUE(std::get<std::string>(val_db->data).compare("testval") == 0);

    // try write again different value on same key without force
    ASSERT_TRUE(client.send_message(make_hex_vector("02 0d 2E746573742E7375622E76616C 03 08 746573743276616C")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF); // unsafe write -> not detectable currently

    auto val_db2 = db->get(".test.sub.val");
    ASSERT_TRUE(val_db2);
    ASSERT_TRUE(val_db2->type == ValueType::STRING);
    ASSERT_TRUE(std::get<std::string>(val_db2->data).compare("testval") == 0);

    // remove all permissions
    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 61646D696E 01 2e 00")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF); // ack

    ASSERT_TRUE(client.send_message(make_hex_vector("02 0d 2E746573742E7375622E76616C 03 07 7465737476616C")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x00); // permission denied

    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_admin_forced_set) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    auto db = get_db();

    //correct hex: "02 0d 2E746573742E7375622E76616C 03 07 7465737476616C"

    // correct access, set value
    ASSERT_TRUE(client.send_message(make_hex_vector("03 0d 2E746573742E7375622E76616C 03 07 7465737476616C")));
    auto res = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(res, {0xFF}));
    auto val_db = db->get(".test.sub.val");
    ASSERT_TRUE(val_db);
    ASSERT_TRUE(val_db->type == ValueType::STRING);
    ASSERT_TRUE(std::get<std::string>(val_db->data).compare("testval") == 0);

    // try write again different value on same key without force
    ASSERT_TRUE(client.send_message(make_hex_vector("03 0d 2E746573742E7375622E76616C 03 08 746573743276616C")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF); // unsafe write -> not detectable currently

    auto val_db2 = db->get(".test.sub.val");
    ASSERT_TRUE(val_db2);
    ASSERT_TRUE(val_db2->type == ValueType::STRING);
    ASSERT_TRUE(std::get<std::string>(val_db2->data).compare("test2val") == 0);

    client.close_connection();
}

TEST_F(IntegrationTests_TCPServer, login_admin_del) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    auto db = get_db();

    {
        CopiedValue newval{};
        newval.type = ValueType::INT;
        newval.data.emplace<int>(42);
        db->set(".top.sub.val1", std::move(newval), false);
    }
    {
        CopiedValue newval{};
        newval.type = ValueType::INT;
        newval.data.emplace<int>(69);
        db->set(".top.sub.val2", std::move(newval), false);
    }

    // key hex:2E746F702E7375622E76616C31 (len13)

    // invalid key
    ASSERT_TRUE(client.send_message(make_hex_vector("04 0d 2E246F702E7375622E76616C31")));
    auto res = client.receive_message(wait_time_error);
    print_vec(res);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x00); // invalid key

    // correct key, delete value
    ASSERT_TRUE(client.send_message(make_hex_vector("04 0d 2E746F702E7375622E76616C31")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(res, {0xFF}));
    auto val_db = db->get(".top.sub.val1");
    ASSERT_FALSE(val_db);

    // try delete again, should work
    ASSERT_TRUE(client.send_message(make_hex_vector("04 0d 2E746F702E7375622E76616C31")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF); // currently not detectable that key does not exist anymore

    // remove all permissions
    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 61646D696E 01 2e 00")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF); // ack

    ASSERT_TRUE(client.send_message(make_hex_vector("04 0d 2E746F702E7375622E76616C32")));
    res = client.receive_message(wait_time_error);
    print_vec(res);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x00); // permission denied

    client.close_connection();
}


TEST_F(IntegrationTests_TCPServer, login_admin_list) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in
    /*
    Add data to db
                 a2
               /    \
              b1     b2
             /         \
          "lol"     (3, "test)
     */
    auto db = this->get_db();
    {
        CopiedValue t{};
        t.type = ValueType::STRING;
        t.data = "lol";
        db->set(".a2.b1", std::move(t));
    }
    {
        auto ta = std::make_unique<CopiedValue>();
        ta->type = ValueType::INT;
        ta->data.emplace<int>(3);

        auto tb = std::make_unique<CopiedValue>();
        tb->type = ValueType::STRING;
        tb->data.emplace<std::string>("test");

        CopiedValue::copied_pair_t tp;
        tp.first = std::move(ta);
        tp.second = std::move(tb);

        CopiedValue t{};
        t.type = ValueType::PAIR;
        t.data.emplace<CopiedValue::copied_pair_t>(std::move(tp));
        db->set(".a2.b2", std::move(t));
    }
    // listing key .a2 -> expecting {b1: "lol"; b2: <pair>} [10 02 02 6231 03 03 6c6f6c 02 6232 48]
    std::string result = "FF 10 02 02 6231 03 03 6c6f6c 02 6232 48"; // ACK {b1: "lol"; b2: <pair>}
    std::vector<unsigned char> data = make_hex_vector(result.c_str());

    ASSERT_TRUE(client.send_message(make_hex_vector("05 03 2E2432")));
    auto res = client.receive_message(wait_time_error);
    print_vec(res);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x00); // invalid key

    ASSERT_TRUE(client.send_message(make_hex_vector("05 03 2E6132")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == data.size());
    std::ranges::sort(data);
    std::ranges::sort(res);
    ASSERT_TRUE(vec_equ(data, res));
}

TEST_F(IntegrationTests_TCPServer, login_admin_create_user) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    // create user tobi - testpwd - 0x00 -> success
    ASSERT_TRUE(client.send_message(make_hex_vector("80 04 746F6269 07 74657374707764 00")));
    auto res = client.receive_message(wait_time_error);
    ASSERT_EQ(res.size(), 1);
    ASSERT_TRUE(res[0] == 0xFF);

    // create user tobi - testpwd - 0x00 -> error username already exists
    ASSERT_TRUE(client.send_message(make_hex_vector("80 04 746F6269 07 74657374707764 00")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xff && res[2] == 0x05);

    // create user test - testpwd - 0xfa -> error invalid options
    ASSERT_TRUE(client.send_message(make_hex_vector("80 04 74657374 07 74657374707764 fa")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x80 && res[2] == 0x01);
}

TEST_F(IntegrationTests_TCPServer, login_admin_delete_user) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    // create user tobi - testpwd - 0x00 -> success
    ASSERT_TRUE(client.send_message(make_hex_vector("80 04 746F6269 07 74657374707764 00")));
    auto res = client.receive_message(wait_time_success);
    ASSERT_EQ(res.size(), 1);
    ASSERT_TRUE(res[0] == 0xFF);

    // delete user tobi -> success
    ASSERT_TRUE(client.send_message(make_hex_vector("8204 746F6269")));
    res = client.receive_message(wait_time_success);
    ASSERT_EQ(res.size(), 1);
    ASSERT_TRUE(res[0] == 0xFF);

    // delete user tobi -> failure user not found
    ASSERT_TRUE(client.send_message(make_hex_vector("82 04 746F6269")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xff && res[2] == 0x02);

    // delete user admin -> failure user currently connected
    ASSERT_TRUE(client.send_message(make_hex_vector("82 05 61646D696E")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x82 && res[2] == 0x01);
}


TEST_F(IntegrationTests_TCPServerManual, save_acl_to_disk) {
    this->server_start();


    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    // create user tobi - testpwd - 0x00 -> success
    ASSERT_TRUE(client.send_message(make_hex_vector("80 04 746F6269 07 74657374707764 00")));
    auto res = client.receive_message(wait_time_success);
    ASSERT_EQ(res.size(), 1);
    ASSERT_TRUE(res[0] == 0xFF);

    // create user test1 - testpwd - 0x01 -> success
    ASSERT_TRUE(client.send_message(make_hex_vector("80 05 7465737431 07 74657374707764 01")));
    res = client.receive_message(wait_time_success);
    ASSERT_EQ(res.size(), 1);
    ASSERT_TRUE(res[0] == 0xFF);

    // set user tobi permission read on key .sub.a
    ASSERT_TRUE(client.send_message(make_hex_vector("81 04 746F6269 06 2E7375622E61 01")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF);

    // set user tobi permission read-write on key .sub.b
    ASSERT_TRUE(client.send_message(make_hex_vector("81 04 746F6269 06 2E7375622E62 03")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF);

    // set user tobi permission write on key .sub.a.write
    ASSERT_TRUE(client.send_message(make_hex_vector("81 04 746F6269 0C 2E7375622E612E7772697465 02")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF);

    this->server_stop();

    ASSERT_TRUE(this->save_acl("temp.bin"));
}

TEST_F(IntegrationTests_TCPServerManual, load_acl_from_disk) {
    this->read_acl("temp.bin");
    this->server_start();

    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x07};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 04 746F6269 07 74657374707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // tobi logged in

    auto db = this->get_db();
    {
        CopiedValue t{};
        t.type = ValueType::INT;
        t.data = 5;
        db->set(".sub.a.read", std::move(t));
    }

    // should be able to read .sub.a.read as .sub.a has read permission
    ASSERT_TRUE(client.send_message(make_hex_vector("01 0B 2E7375622E612E72656164")));
    auto res = client.receive_message(wait_time_success);
    ASSERT_EQ(res.size(), 6);
    ASSERT_EQ(res[0], 0xFF);

    // should NOT be able to write to .sub.a.read
    ASSERT_TRUE(client.send_message(make_hex_vector("02 0B 2E7375622E612E72656164 02 00000001")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xFF && res[2] == 0x00);

    // should be able to write to .sub.a.write
    ASSERT_TRUE(client.send_message(make_hex_vector("02 0C 2E7375622E612E7772697465 02 00000001")));
    res = client.receive_message(wait_time_error);
    ASSERT_EQ(res.size(), 1);
    ASSERT_EQ(res[0], 0xFF);

    this->server_stop();
}


TEST_F(IntegrationTests_TCPServer, login_admin_change_pwd) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    // change password of admin to "123"
    ASSERT_TRUE(client.send_message(make_hex_vector("83 03 313233")));
    auto res = client.receive_message(wait_time_success);
    ASSERT_EQ(res.size(), 1);
    ASSERT_TRUE(res[0] == 0xFF);

    client.close_connection();

    safe_sleep(wait_time_error);

    // try to log in with new passwd
    DumbTCPClient client2("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client2.connect_to_server());

    std::vector<unsigned char> client_information_package2{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack2 = make_hex_vector("FF 05 61646D696E 03 313233");

    ASSERT_TRUE(client2.send_message(client_information_package2));
    auto server_information_package2 = client2.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client2.send_message(user_info_pack2));
    auto logged_in2 = client2.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

}

TEST_F(IntegrationTests_TCPServer, login_admin_follow) {
    DumbTCPClient client("127.0.0.1", ServerPort::instance().get());
    ASSERT_TRUE(client.connect_to_server());

    std::vector<unsigned char> client_information_package{0x00, 0x00, 0x0F};
    std::vector<unsigned char> user_info_pack = make_hex_vector("FF 05 61646D696E 07 726F6F74707764");

    ASSERT_TRUE(client.send_message(client_information_package));
    auto server_information_package = client.receive_message(wait_time_success);
    ASSERT_TRUE(server_information_package.size() == 10);
    ASSERT_TRUE(server_information_package[0] == 0xFF);
    ASSERT_TRUE(client.send_message(user_info_pack));
    auto logged_in = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(logged_in, {0xFF}));

    // admin logged in

    auto db = this->get_db();
    {
        // .x.a -> .x.b
        CopiedValue newval{};
        newval.type = ValueType::REFERENCE;
        newval.data.emplace<std::string>(".x.b");
        db->set(".x.a", std::move(newval), true);
    }
    {
        // .x.b -> .x.c
        CopiedValue newval{};
        newval.type = ValueType::REFERENCE;
        newval.data.emplace<std::string>(".x.c");
        db->set(".x.b", std::move(newval), true);
    }
    {
        // .x.c -> .x.d
        CopiedValue newval{};
        newval.type = ValueType::REFERENCE;
        newval.data.emplace<std::string>(".x.d");
        db->set(".x.c", std::move(newval), true);
    }
    {
        // .x.d = 5
        CopiedValue newval{};
        newval.type = ValueType::INT;
        newval.data.emplace<int>(5);
        db->set(".x.d", std::move(newval), true);
    }
    {
        // .x.start -> .y.end
        CopiedValue newval{};
        newval.type = ValueType::REFERENCE;
        newval.data.emplace<std::string>(".y.end");
        db->set(".x.start", std::move(newval), true);
    }
    {
        // .y.end = 6
        CopiedValue newval{};
        newval.type = ValueType::INT;
        newval.data.emplace<int>(6);
        db->set(".y.end", std::move(newval), true);
    }

    // remove all permissions
    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 61646D696E 01 2e 00")));
    auto res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF);

    // set read for .x
    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 61646D696E 02 2E78 01")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF);

    // safe follow on .x.a with max_jump = 15 -> success returns 5
    ASSERT_TRUE(client.send_message(make_hex_vector("06 04 2E782E61 0f")));
    res = client.receive_message(wait_time_success);
    ASSERT_EQ(res.size(), 6);
    ASSERT_TRUE(res[0] == 0xFF && res[5] == 5);

    // safe follow on .x.start with max_jump = 15 -> fail, no read on .y anything
    ASSERT_TRUE(client.send_message(make_hex_vector("06 08 2E782E7374617274 0f")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xff && res[2] == 0x00);

    // forced follow on .y.end to check permission of start key
    ASSERT_TRUE(client.send_message(make_hex_vector("07 06 2E792E656E64 0f")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0xff && res[2] == 0x00);

    // forced follow on .x.start with max_jump = 15 -> success returns 6
    ASSERT_TRUE(client.send_message(make_hex_vector("07 08 2E782E7374617274 0f")));
    auto resfstart = client.receive_message(wait_time_success);
    ASSERT_EQ(resfstart.size(), 6);
    ASSERT_TRUE(resfstart[0] == 0xFF && resfstart[5] == 6);

    // set read for .y
    ASSERT_TRUE(client.send_message(make_hex_vector("81 05 61646D696E 02 2E79 01")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(res.size() == 1);
    ASSERT_TRUE(res[0] == 0xFF);

    // safe follow on .x.start with max_jump = 15 -> success now, as permission was granted
    ASSERT_TRUE(client.send_message(make_hex_vector("06 08 2E782E7374617274 0f")));
    res = client.receive_message(wait_time_success);
    ASSERT_TRUE(vec_equ(resfstart, res));

    // safe follow on .x.a with max_jump = 0 -> error, this is stupid
    ASSERT_TRUE(client.send_message(make_hex_vector("06 04 2E782E61 00")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x02 && res[2] == 0x01);

    // safe follow on .x.a with max_jump = 1 -> key:.x.b (value: .x.c)
    ASSERT_TRUE(client.send_message(make_hex_vector("06 04 2E782E61 01")));
    res = client.receive_message(wait_time_success);
    ASSERT_EQ(res.size(), 7);
    ASSERT_TRUE(res[0] == 0xFF && res[6] == 'c');

    // forced follow on .x.a with max_jump = 1 -> key:.x.b (value: .x.c)
    ASSERT_TRUE(client.send_message(make_hex_vector("07 04 2E782E61 01")));
    res = client.receive_message(wait_time_success);
    ASSERT_EQ(res.size(), 7);
    ASSERT_TRUE(res[0] == 0xFF && res[6] == 'c');

    // safe follow on invalid key
    ASSERT_TRUE(client.send_message(make_hex_vector("06 03 616224 0f")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x00);

    // forced follow on invalid key
    ASSERT_TRUE(client.send_message(make_hex_vector("07 03 616224 0f")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x00);

    // safe follow on non existant key
    ASSERT_TRUE(client.send_message(make_hex_vector("06 06 2E782E616263 0f")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x01);

    // forced follow on non existant key
    ASSERT_TRUE(client.send_message(make_hex_vector("07 06 2E782E616263 0f")));
    res = client.receive_message(wait_time_error);
    ASSERT_TRUE(res.size() >= 3);
    ASSERT_TRUE(res[0] == 0x00 && res[1] == 0x01 && res[2] == 0x01);
}