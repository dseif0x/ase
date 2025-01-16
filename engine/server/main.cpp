/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <thread>
#include <getopt.h>
#include <csignal>

#include "tcp2_server.h"
#include "acl.h"


std::atomic<bool> stopFlag(false);

void print_usage() {
    std::cout << "Usage: " << "ase_server" << " [options]\n"
              << "Options:\n"
              << "  -h, --help              Prints this usage message.\n"
              << "  -p, --port <PORT>       Sets the server port. Mandatory.\n"
              << "  -a, --acl <PATH>        Defines the ACL path.\n"
              << "      --acl-output <PATH> Use if acl shall be written to different location.\n"
              << "  -d, --db <PATH>         Defines the database save path.\n"
              << "      --db-output <PATH>  Use if db shall be written to different location.\n"
              << "      --max-clients <N>   Sets the maximum number of clients. Default: 15.\n"
              << "      --destroy           Trys to destroy the database before connecting to it.\n";
}

int print_error(const std::string &message, int error) {
    std::cout << "error: " << message << "\n" << std::endl;
    print_usage();
    return error;
}

void print_info(const std::string &message) {
    std::cout << ">> " << message << std::endl;
}

void handleSigint(int signal) {
    std::cout << "... stopping server ..." << std::endl;
    stopFlag = true;
}

int main(int argc, char *argv[]) {
    std::signal(SIGINT, handleSigint);

    std::string acl_input;
    std::string acl_output;
    std::string db_input;
    std::string db_output;
    int max_clients = 15;
    int port = -1;
    bool destroy_mode = false;

    struct option longOptions[] = {
        {"help", no_argument, nullptr, 'h'},
        {"acl", required_argument, nullptr, 'a'},
        {"acl-output", required_argument, nullptr, 0},
        {"max-clients", required_argument, nullptr, 0},
        {"port", required_argument, nullptr, 'p'},
        {"db", required_argument, nullptr, 'd'},
        {"db-output", required_argument, nullptr, 0},
        {"destroy", no_argument, nullptr, 0},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    int optionIndex = 0;

    while ((opt = getopt_long(argc, argv, "ha:p:d:", longOptions, &optionIndex)) != -1) {
        switch (opt) {
            case 'h':
                print_usage();
                return 0;
            case 'a':
                if (acl_input.empty()) return print_error("ACL path is already set.", 1);
                acl_input = optarg;
                break;
            case 'p':
                if (port != -1) return print_error("Port is already set.", 1);
                try {
                    port = std::stoi(optarg);
                    if (port < 0 || port >= 65536) return print_error("Invalid port number.", 1);
                } catch (...) {
                    return print_error("Port must be a number.", 1);
                }
                break;
            case 'd':
                if (db_input.empty()) return print_error("DB path is already set.", 1);
                db_input = optarg;
                break;
            case 0: // Handle long options without a short option
                if (std::string(longOptions[optionIndex].name) == "max-clients") {
                    try {
                        max_clients = std::stoi(optarg);
                    } catch (...) {
                        return print_error("Max clients must be a number.", 1);
                    }
                } else if (std::string(longOptions[optionIndex].name) == "destroy") {
                    if (destroy_mode) return print_error("Destroy mode was already set.", 1);
                    destroy_mode = true;
                } else if (std::string(longOptions[optionIndex].name) == "acl-output") {
                    if (!acl_output.empty()) return print_error("ACL output path is already set.", 1);
                    acl_output = optarg;
                } else if (std::string(longOptions[optionIndex].name) == "db-output") {
                    if (!acl_output.empty()) return print_error("DB output path is already set.", 1);
                    db_output = optarg;
                }
                break;
            default:
                print_usage();
                return 1;
        }
    }

    // Check if the mandatory port option is provided
    if (port == -1) {
        return print_error("The port must be provided.", 1);
    }

    if (acl_output.empty()) acl_output = acl_input;
    if (db_output.empty()) db_output = db_input;

    if (destroy_mode) {
        DB::destroy();
    }

    TCP2Server server{port, max_clients};

    if (!acl_input.empty()) {
        // load acl from disk
        if (server.load_acl_from_disk(acl_input)) print_info("ACL loaded from disk.");
        else print_info("Could not load ACL from disk.");
    }

    if (!db_input.empty()) {
        // load db from disk
        if (server.load_db_to_disk(db_input)) print_info("DB loaded from disk.");
        else print_info("Could not load DB from disk.");
    }

    // start server
    server.start();
    print_info("Server started! Stop with Ctrl+C");

    int print_counter = 0;
    while (!stopFlag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (print_counter++ % 100 == 0) print_info("Working.....");
    }

    // stop server
    server.stop();
    print_info("Server stopped!");

    if (!db_output.empty()) {
        // save db to disk
        if (server.save_db_to_disk(db_output)) print_info("DB saved to disk.");
        else print_info("DB could not be saved to disk.");
    }

    if (!acl_output.empty()) {
        // save acl to disk
        if (server.save_acl_to_disk(acl_output)) print_info("ACL saved to disk.");
        else print_info("ACL could not be saved to disk.");
    }

    return 0;
}