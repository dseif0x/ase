/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "tcp2_utils.h"
#include <iostream>
#include <tcp2_server.h>
#include <thread>
#include "decoder.h"


void tcp2_utils::client_command_switch(ACLUser *user, TCP2Decoder &decoder, TCP2Encoder &encoder, unsigned char self_perms) {
    void (*func)(ACLUser*, TCP2Decoder&, TCP2Encoder&);
    unsigned char perms{};
    bool error = false;

    switch (decoder.to_unsigned(1)) {
        case 0xFE: //STOP
            func = command::stop;
            perms = permissions::none;
            break;
        default:
            return;
    }
    if ((self_perms | perms) != self_perms) {
        // check connection permissions
        encoder.from_nak();
        encoder.encode_error(errors::permission_denied_self);
    } else if ((perms & permissions::admin) > 0) {
        // check admin permission
        encoder.from_nak();
        encoder.encode_error(errors::permission_denied);
    } else {
        func(user, decoder, encoder);
    }
    // todo think about how to get all the information out of the decoder that is not needed if we dont execute
    //  the command -> run the command, but block the encoder
}

void tcp2_utils::command::stop(ACLUser *user, TCP2Decoder &decoder, TCP2Encoder &encoder) {
    return;
}

void tcp2_utils::client_state_thread(TCP2Decoder &decoder, TCP2Encoder &encoder, ACL &acl) {
    std::string username;
    ACLUser *user{nullptr};
    unsigned char self_perms{};

    int state = 0;
    while (decoder.is_healthy()) {
        std::cout << "state:" << state << std::endl;
        if (state < 0) {
            if (encoder.is_empty()) break;
        } else if (state == 0) {
            // Expecting client information
            auto client_version = decoder.to_unsigned(2);
            self_perms = decoder.to_unsigned(1);
            std::cout << "client is running version: " << client_version << std::endl;
            std::cout << "client opens connection with intention to: " <<((self_perms & 0x01) > 0 ? "read, " : "") <<
                    ((self_perms & 0x02) > 0 ? "write, " : "") << ((self_perms & 0x04) > 0 ? "delete, " : "") <<
                    ((self_perms & 0x08) > 0 ? "admin " : "") << "[" << self_perms << "]" << std::endl;
            if (!decoder.is_empty()) {
                state = -1;
                encoder.from_nak();
                encoder.encode_error(errors::general_protocol_error);
                continue;
            }
            if (client_version != 0) {
                state = -1;
                encoder.from_nak();
                encoder.encode_error(errors::invalid_version);
                continue;
            }
            encoder.from_ack();
            encoder.from_unsigned(1, 1); // size of bool
            encoder.from_unsigned(1, sizeof(int)); // size of int
            encoder.from_unsigned(1, sizeof(char)); // size of char
            encoder.from_unsigned(1, sizeof(double)); // size of double
            encoder.from_unsigned(1, sizeof(int)); // size of semaphore // todo send actual size
            state = 1;
        } else if (state == 1) {
            if (decoder.to_acknak() != 1) {
                state = -1;
                encoder.from_nak();
                encoder.encode_error(errors::general_protocol_error);
                continue;
            }
            username = decoder.to_string();
            std::string pwd = decoder.to_string();
            int login_res = acl.try_login(username, pwd);
            if (login_res != 0) {
                state = -1;
                encoder.from_nak();
                if (login_res == 1) encoder.encode_error(errors::user_not_found);
                else if (login_res == 2) encoder.encode_error(errors::already_connected);
                else if (login_res == 3) encoder.encode_error(errors::wrong_password);
                continue;
            }
            user = acl.get_user(username);
            encoder.from_ack();
            state = 2;
        } else if (state == 2) {
            // logged in and ready for commands
            client_command_switch(user, decoder, encoder, self_perms);
        }
        // send buffer
        while (!encoder.is_empty()) {
            encoder.write_data();
            // todo add checks if a stop command has come in
        }
    }
    acl.disconnect(username);
}


void tcp2_utils::client_main_thread(std::atomic<bool>& kys, TCP2Decoder &decoder, TCP2Encoder &encoder, ACL &acl) {
    std::thread state(tcp2_utils::client_state_thread, std::ref(decoder), std::ref(encoder), std::ref(acl));

    while (!kys && decoder.is_healthy()) {
        decoder.read_data();
    }

    decoder.set_kys();
    state.join();
}

