/*
* Copyright 2024 Project Contributors
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

void handleSigint(int signal) {
    std::cout << "\nStopping Server...\n";
    stopFlag = true;
}

void printUsage(const char *programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -h, --help            Prints this usage message.\n"
              << "  -i, --acl-input PATH  Defines the input ACL path.\n"
              << "  -o, --acl-output PATH Defines the output ACL path.\n"
              << "      --max-clients N   Sets the maximum number of clients. Default: 10.\n"
              << "  -p, --port PORT       Sets the server port. Mandatory.\n"
              << "  -s, --save-db PATH    Defines the database save path. Default: \".\"\n"
              << "      --destroy         Trys to destroy the database before connecting to it.\n";
}

int main(int argc, char *argv[]) {
    std::signal(SIGINT, handleSigint);

    std::string aclInput;
    std::string aclOutput;
    std::string saveDbPath;
    int maxClients = 10;
    int port = -1;
    bool destroyMode = false;

    struct option longOptions[] = {
        {"help", no_argument, nullptr, 'h'},
        {"acl-input", required_argument, nullptr, 'i'},
        {"acl-output", required_argument, nullptr, 'o'},
        {"max-clients", required_argument, nullptr, 0},
        {"port", required_argument, nullptr, 'p'},
        {"save-db", required_argument, nullptr, 's'},
        {"destroy", no_argument, nullptr, 0},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    int optionIndex = 0;

    while ((opt = getopt_long(argc, argv, "hi:o:p:s:", longOptions, &optionIndex)) != -1) {
        switch (opt) {
            case 'h':
                printUsage(argv[0]);
                return 0;
            case 'i':
                aclInput = optarg;
                break;
            case 'o':
                aclOutput = optarg;
                break;
            case 'p':
                port = std::atoi(optarg);
                break;
            case 's':
                saveDbPath = optarg;
                break;
            case 0: // Handle long options without a short option
                if (std::string(longOptions[optionIndex].name) == "max-clients") {
                    maxClients = std::atoi(optarg);
                } else if (std::string(longOptions[optionIndex].name) == "destroy") {
                    destroyMode = true; // Set the destroy flag to true
                }
                break;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }

    // Check if the mandatory port option is provided
    if (port == -1) {
        std::cerr << "Error: The --port (-p) option is mandatory.\n\n";
        printUsage(argv[0]);
        return 1;
    }

    if (destroyMode) {
        DB::destroy();
    }

    std::string file_path = "test.bin";
    TCP2Server server{port, maxClients};

    if (!aclInput.empty()) {
        // load acl from disk
        if (server.load_acl_from_disk(aclInput)) std::cout << "ACL loaded from disk." << std::endl;
        else std::cout << "ACL could not be loaded from disk." << std::endl;
    }

    // start server
    server.start();
    std::cout << "Server started, stop by entering anything." << std::endl;

    int print_counter = 0;
    while (!stopFlag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (print_counter++ % 300 == 0) std::cout << "Working...    Stop with Ctrl-C\n";
    }

    // stop server
    server.stop();
    std::cout << "Server stopped." << std::endl;

    // save result to disk
    if (!aclOutput.empty()) {
        if (server.save_acl_to_disk(aclOutput)) std::cout << "ACL saved to disk." << std::endl;
        else std::cout << "ACL could not be saved to disk." << std::endl;
    }

    return 0;

}