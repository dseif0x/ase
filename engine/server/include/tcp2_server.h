/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include "acl.h"
#include <atomic>
#include <db.h>
#include <thread>
#include "tcp2_coding.h"


constexpr int BUFFER_SIZE = 128;


/**
 * @brief Represents a TCP server capable of handling multiple clients concurrently.
 */
class TCP2Server {
    int port;
    int max_clients;
    ASE::ACL acl = ASE::ACL{};
    DB db = DB{};
    std::atomic<bool> kill_acceptor{false};
    std::atomic<int> socket_error{0};

    std::thread client_acceptor;
    bool running = false;

public:
    TCP2Server(int port, int max_clients);
    ~TCP2Server();

    /**
    * @brief Loads an acl from file. Must only be called if server is not running.
    *
    * @param path The path of the file.
    */
    bool load_acl_from_disk(const std::string &path);

    /**
    * @brief Saves the acl to a file. Must only be called if server is not running.
    *
    * @param path The path of the file.
    */
    bool save_acl_to_disk(const std::string &path);

    /**
     * @brief Tries to write the db to disk.
     *
     * @param path The path to write the db to.
     * @return if successful
     */
    bool save_db_to_disk(const std::string &path);

    /**
     * @brief Tries to read the db from disk.
     *
     * @param path The path to read the db from.
     * @return if successful
     */
    bool load_db_to_disk(const std::string &path);

    /**
     * @brief Starts the TCP server and begins accepting client connections.
     *
     * Spawns a separate thread to handle client connections using the
     * `tcp2_utils::client_acceptor` utility function.
     */
    void start();

    /**
     * @brief Stops the TCP server.
     *
     * Signals the client acceptor thread to terminate and waits for it to join.
     */
    void stop();

};




#endif //TCP_SERVER_H
