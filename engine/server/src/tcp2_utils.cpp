/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
#include <iostream>
#include <thread>
#include "tcp2_utils.h"

#include <stopwatch.h>

#include "tcp2_server.h"
#include "decoder.h"

// returns -1: some non-critical error occurred, not executed and decoder is cleared
// returns 0: command executed fine
// returns 1: soft shutdown
// returns 2: hard shutdown
int tcp2_utils::client_command_switch(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &ci, unsigned char self_perms) {
    int (*func)(TCP2Decoder&, TCP2Encoder&, CommandInfo&);
    unsigned char perms{};
    uint16_t error = 0;

    switch (decoder.to_unsigned(1)) {
        // CONTROL COMMANDS
        case 0xFE: //STOP
            func = command::stop;
            perms = permissions::none;
            break;
        case 0xFD: //PING
            func = command::ping;
            perms = permissions::none;
            break;
        case 0xFC: //DISCONNECT
            func = command::disconnect;
            perms = permissions::none;
            break;
        // DB Commands
        case 0x01: //GET
            func = command::get;
            perms = permissions::read;
            break;
        case 0x02: //SET
            func = command::safe_set;
            perms = permissions::write;
            break;
        case 0x03: //forced-SET
            func = command::forced_set;
            perms = permissions::write_del;
            break;
        case 0x04: //DELETE
            func = command::del;
            perms = permissions::del;
            break;
        case 0x05: //LIST
            func = command::list;
            perms = permissions::read;
            break;
        // ACL Commands
        case 0x80: // Create User
            func = command::create_user;
            perms = permissions::admin;
            break;
        case 0x81: // Update Scope
            func = command::update_scope;
            perms = permissions::admin;
            break;
        default:
            error = errors::unknown_command;
            break;
    }
    if (error == 0) {
        if ((self_perms | perms) != self_perms) {
            // check connection permissions
            error = errors::permission_denied_self;
        } else if ((perms & permissions::admin) > 0) {
            // check admin permission
            if (!ci.user->is_admin()) {
                error = errors::permission_denied;
            }
        }
    }

    if (error == 0) {
        return func(decoder, encoder, ci);;
    } else {
        encoder.from_nak();
        encoder.encode_error(error);
        return -1;
    }
}

std::optional<std::vector<std::string>> tcp2_utils::check_and_create_key(std::string &key) {
    std::vector<std::string> res{};
    std::string cur = "";
    bool first = true;
    for (char &c : key) {
        if (c == '.') {
            if (first) { //skip first dot if it is at the very beginning
                first = false;
                continue;
            }
            if (cur == "") return std::nullopt;
            res.push_back(cur);
            cur = "";
        } else {
            if (!(c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z' || c >= '0' && c <= '9')) return std::nullopt;
            cur += c;
        }
        first = false;
    }
    if (cur != "") res.push_back(cur);
    return res;
}

int tcp2_utils::command::stop(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    encoder.clear_buffer();
    encoder.from_ack();
    return 0;
}

int tcp2_utils::command::ping(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    encoder.from_ack();
    unsigned long long data = decoder.to_unsigned(4);
    encoder.from_unsigned(4, data);
    return 0;
}

int tcp2_utils::command::disconnect(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    encoder.from_ack();
    encoder.from_string("Goodbye!");
    return 1;
}

int tcp2_utils::command::get(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string key_str = decoder.to_string();
    auto key = tcp2_utils::check_and_create_key(key_str);

    if (!key) {
        encoder.from_nak();
        encoder.encode_error(errors::invalid_key);
        return -1;
    }
    if (!info.user->has_permission(*key, permissions::read)) {
        encoder.from_nak();
        encoder.encode_error(errors::permission_denied);
        return -1;
    }
    auto res = info.db.get(key_str.c_str());
    if (!res) {
        encoder.from_nak();
        encoder.encode_error(errors::key_not_found);
        return -1;
    }
    res->print();
    encoder.from_ack();
    if (!encoder.from_general(*res)) {
        // this should normally never happen, as the data here is extracted directly from the db and therefore correct
        encoder.clear_buffer();
        encoder.from_nak();
        encoder.encode_error(errors::general_server_error);
        return -1;
    }
    return 0;
}

int tcp2_utils::command::update_scope(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string username = decoder.to_string();
    std::string key_str = decoder.to_string();
    unsigned char permission = decoder.to_unsigned(1);
    auto key = tcp2_utils::check_and_create_key(key_str);

    if (!key) {
        encoder.from_nak();
        encoder.encode_error(errors::invalid_key);
        return -1;
    }

    if (!info.acl.has_user(username)) {
        encoder.from_nak();
        encoder.encode_error(errors::user_not_found);
        return -1;
    }

    if (!info.acl.update_key_permission(username, *key, permission)) {
        // this should never happen
        encoder.from_nak();
        encoder.encode_error(errors::general_server_error);
        return -1;
    }
    encoder.from_ack();

    return 0;
}

int tcp2_utils::command::create_user(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string username = decoder.to_string();
    std::string pwd = decoder.to_string();
    unsigned char options = decoder.to_byte();

    if (info.acl.has_user(username)) {
        encoder.from_nak();
        encoder.encode_error(errors::username_already_exists);
        return -1;
    }

    if (options > 1) {
        encoder.from_nak();
        encoder.encode_error(errors::unknown_option);
        return -1;
    }

    bool is_admin = (options & 0x01) > 0;
    if (!info.acl.create_user(username, pwd, is_admin)) {
        // should never happen
        encoder.from_nak();
        encoder.encode_error(errors::general_server_error);
        return -1;
    }

    encoder.from_ack();
    return 0;
}

int set_helper(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info, bool force) {
    std::string key_str = decoder.to_string();
    auto key = tcp2_utils::check_and_create_key(key_str);

    if (!key) {
        encoder.from_nak();
        encoder.encode_error(errors::invalid_key);
        return -1;
    }
    if (!info.user->has_permission(*key, force ? tcp2_utils::permissions::write_del : tcp2_utils::permissions::write)) {
        encoder.from_nak();
        encoder.encode_error(errors::permission_denied);
        return -1;
    }
    auto val = decoder.to_general();
    if (!val) {
        encoder.from_nak();
        encoder.encode_error(errors::invalid_value);
        return -1;
    }

    val->print();

    try {
        info.db.set(key_str.c_str(), std::move(*val), force);
        encoder.from_ack();
    } catch (...) {
        encoder.from_nak();
        encoder.encode_error(errors::general_server_error);
        return -1;
    }
    return 0;
}

int tcp2_utils::command::safe_set(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    return set_helper(decoder, encoder, info, false);
}

int tcp2_utils::command::forced_set(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    return set_helper(decoder, encoder, info, true);
}

int tcp2_utils::command::del(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string key_str = decoder.to_string();
    auto key = check_and_create_key(key_str);

    if (!key) {
        encoder.from_nak();
        encoder.encode_error(errors::invalid_key);
        return -1;
    }
    if (!info.user->has_permission(*key, permissions::del)) {
        encoder.from_nak();
        encoder.encode_error(errors::permission_denied);
        return -1;
    }

    try {
        info.db.remove(key_str.c_str());
        encoder.from_ack();
    } catch (...) {
        // can this even happen?
        encoder.from_nak();
        encoder.encode_error(errors::general_server_error);
        return -1;
    }
    return 0;
}

int tcp2_utils::command::list(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string key_str = decoder.to_string();
    auto key = check_and_create_key(key_str);

    if (!key) {
        encoder.from_nak();
        encoder.encode_error(errors::invalid_key);
        return -1;
    }

    if (!info.user->has_permission(*key, permissions::read)) {
        encoder.from_nak();
        encoder.encode_error(errors::permission_denied);
        return -1;
    }

    auto ret = info.db.list(key_str.c_str());
    encoder.from_ack();
    encoder.from_list_t(ret);
    return 0;
}

void tcp2_utils::client_state_thread(TCP2Decoder &decoder, TCP2Encoder &encoder, ACL &acl, DB &db) {
    std::string username;
    ACLUser *user{nullptr};
    unsigned char self_perms{};

    std::unique_ptr<CommandInfo> ci;

    int state = 0;
    while (decoder.is_healthy()) {
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
            encoder.from_unsigned(2, 16384);
            encoder.from_unsigned(2, 16384);
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
            ci = std::make_unique<CommandInfo>(user, acl, db);
        } else if (state == 2) {
            // logged in and ready for commands
            stopwatch s4{};
            s4.start();
            int temp_ret = client_command_switch(decoder, encoder, *ci, self_perms);
            switch (temp_ret) {
                case -1: // error, but recovered
                    decoder.clear_buffer();
                case 0: // executed fine
                    break;
                case 1: // soft shutdown
                    state = -3;
                    break;
                case 2: // hard shutdown
                    decoder.set_kys();
                    continue;
                    break;
                default: // undefined -> soft shutdown
                    state = -1;
            }
        }
        // send buffer
        while (!encoder.is_empty()) {
            // stop is only valid if the connection is achieved and the user is logged in
            if (state == 2 && decoder.is_stop()) {
                decoder.clear_buffer();
                encoder.clear_buffer();
                encoder.from_ack();
                break;
            }
            if (!encoder.write_data()) {
                // sending is broken, hard stop
                decoder.set_kys();
                break;
            }
        }
    }
    acl.disconnect(username);
    decoder.set_kys();
}


void tcp2_utils::client_main_thread(std::atomic<bool>& kys, TCP2Decoder &decoder, TCP2Encoder &encoder, ACL &acl, DB &db) {
    std::thread state(tcp2_utils::client_state_thread, std::ref(decoder), std::ref(encoder), std::ref(acl), std::ref(db));

    while (!kys && decoder.is_healthy()) {
        decoder.read_data();
    }

    decoder.set_kys();
    state.join();
}

