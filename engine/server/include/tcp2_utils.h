/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */


#ifndef TCP2_UTILS_H
#define TCP2_UTILS_H
#include "acl.h"
#include <atomic>
#include <db.h>

#include "tcp2_coding.h"
#include "decoder.h"
#include "encoder.h"

// todo replace symbol with correct one
#define UNIX

struct CommandInfo {
    ACLUser *user;
    ACL &acl;
    DB &db;

    CommandInfo(ACLUser *userPtr, ACL &aclRef, DB &dbRef)
        : user(userPtr), acl(aclRef), db(dbRef) {}
};

namespace tcp2_utils {
    // client_acceptor needs a system dependent implementation but keeps the same signature
    void client_acceptor(std::atomic<bool> &kys, ACL &acl, DB &db, int port, int max_clients);
#ifdef UNIX
    // following function needs system dependent implementation as well as possibly different signatures
    void client_coding_setup(std::atomic<bool> &kys, int client_socket, ACL &acl, DB &db);
#elifdef WIN32
#endif

    // following functions are system independent
    /**
     * @brief Main thread managing a single client connection.
     *
     * @param kys Atomic boolean signaling termination.
     * @param decoder Reference to the TCP2Decoder instance for decoding client data.
     * @param encoder Reference to the TCP2Encoder instance for encoding responses.
     * @param acl Reference to the ACL managing user authentication and permissions.
     */
    void client_main_thread(std::atomic<bool>& kys, TCP2Decoder &decoder, TCP2Encoder &encoder, ACL &acl, DB &db);
    /**
     * @brief Manages the client state machine, handling authentication and client commands.
     *
     * @param decoder Reference to the TCP2Decoder instance for decoding client data.
     * @param encoder Reference to the TCP2Encoder instance for encoding responses.
     * @param acl Reference to the ACL (Access Control List) managing user authentication and permissions.
     */
    void client_state_thread(TCP2Decoder &decoder, TCP2Encoder &encoder, ACL &acl, DB &db);
    /**
     * @brief Processes client commands and executes the appropriate function based on the command code.
     *
     * @param user Pointer to the authenticated user.
     * @param decoder Reference to the TCP2Decoder instance for decoding client data.
     * @param encoder Reference to the TCP2Encoder instance for encoding responses.
     * @param self_perms Permissions associated with the client connection.
     * @return int Status code indicating the result:
     *         - -1: Non-critical error occurred, command not executed, decoder cleared.
     *         -  0: Command executed successfully.
     *         -  1: Soft shutdown.
     *         -  2: Hard shutdown.
     */
    int client_command_switch(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &ci, unsigned char self_perms);

    std::optional<std::vector<std::string>> check_and_create_key(std::string &key);

    namespace command {
        /**
         * @brief Executes the STOP command, signaling no errors.
         *
         * @param user Pointer to the authenticated user.
         * @param decoder Reference to the TCP2Decoder instance for decoding client data.
         * @param encoder Reference to the TCP2Encoder instance for encoding responses.
         * @return int Always returns 0.
         */
        int stop(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info);
        /**
         * @brief Executes the PING command, echoing back a provided value.
         *
         * @param user Pointer to the authenticated user.
         * @param decoder Reference to the TCP2Decoder instance for decoding client data.
         * @param encoder Reference to the TCP2Encoder instance for encoding responses.
         * @return int Always returns 0.
         */
        int ping(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info);
        /**
         * @brief Executes the DISCONNECT command, sending a goodbye message.
         *
         * @param user Pointer to the authenticated user.
         * @param decoder Reference to the TCP2Decoder instance for decoding client data.
         * @param encoder Reference to the TCP2Encoder instance for encoding responses.
         * @return int Always returns 1 to signal a soft shutdown.
         */
        int disconnect(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info);

        int get(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info);
        int update_scope(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info);

        /**
         * @brief Executes a test command for debugging and verification purposes.
         *
         * @param user Pointer to the authenticated user.
         * @param decoder Reference to the TCP2Decoder instance for decoding client data.
         * @param encoder Reference to the TCP2Encoder instance for encoding responses.
         * @return int Always returns 0.
         */
        int testing(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info);
    }

    namespace permissions {
        constexpr unsigned char none = 0x00;
        constexpr unsigned char read = 0x01;
        constexpr unsigned char write = 0x02;
        constexpr unsigned char del = 0x04;
        constexpr unsigned char admin = 0x08;
        constexpr unsigned char read_write = read | write;
        constexpr unsigned char read_write_del = read_write | del;
        constexpr unsigned char all = read | write | del;
    }
}

#endif //TCP2_UTILS_H
