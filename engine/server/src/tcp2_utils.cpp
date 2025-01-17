/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "decoder.h"
#include <iostream>
#include <thread>
#include "tcp2_utils.h"

#include <stopwatch.h>

#include "tcp2_server.h"
#include "acl.h"

// returns -1: some non-critical error occurred, not executed and decoder is cleared
// returns 0: command executed fine
// returns 1: soft shutdown
// returns 2: hard shutdown
int tcp2_utils::client_command_switch(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &ci) {
    int (*func)(TCP2Decoder&, TCP2Encoder&, CommandInfo&);
    unsigned char perms{};
    uint16_t error = 0;

    auto temp = decoder.to_unsigned(1);
#ifdef DEBUG
    std::cout << "> current command: " << temp << std::endl;
#endif
    switch (temp) {
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
        case 0x06: //SAFE-FOLLOW
            func = command::safe_follow;
            perms = permissions::read;
            break;
        case 0x07: //FORCED-FOLLOW
            func = command::forced_follow;
            perms = permissions::read | permissions::admin;
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
        case 0x82: // Delete User
            func = command::delete_user;
            perms = permissions::admin;
            break;
        case 0x83: // Change User pwd
            func = command::change_pwd;
            perms = permissions::none;
            break;
        case 0x84: // Get User-Perms
            func = command::get_user_perms;
            perms = permissions::none; // additional admin check in the
            break;
        default:
            error = errors::unknown_command;
            break;
    }
    if (error == 0) {
        if ((ci.self_perms | perms) != ci.self_perms) {
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
        return func(decoder, encoder, ci);
    } else {
        return error;
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
    return -9;
}

int tcp2_utils::command::get(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string key_str = decoder.to_string();
    auto key = tcp2_utils::check_and_create_key(key_str);

    if (!key) {
        return errors::invalid_key;
    }
    if (!info.user->has_permission(*key, permissions::read)) {
        return errors::permission_denied;
    }
    auto res = info.db.get(key_str.c_str());
    if (!res) {
        return errors::key_not_found;
    }
    encoder.from_ack();
    if (!encoder.from_general(*res)) {
        // this should normally never happen, as the data here is extracted directly from the db and therefore correct
        encoder.clear_buffer();
        encoder.from_nak();
        encoder.encode_error(errors::general_server_error);
        return -3;
    }
    return 0;
}

int tcp2_utils::command::update_scope(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string username = decoder.to_string();
    std::string key_str = decoder.to_string();
    unsigned char permission = decoder.to_unsigned(1);
    auto key = tcp2_utils::check_and_create_key(key_str);

    if (!key) {
        return errors::invalid_key;
    }

    if (!info.acl.has_user(username)) {
        return errors::user_not_found;
    }

    if (!info.acl.update_key_permission(username, *key, permission)) {
        // this should never happen
        return errors::general_server_error;
    }
    encoder.from_ack();

    return 0;
}

int tcp2_utils::command::get_user_perms(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    auto len = decoder.to_expanding_unsigned();
    std::vector<std::string> usernames;
    bool user_admin = (info.self_perms & permissions::admin) > 0 && info.user->is_admin();
    if (len == 0) {
        if (user_admin) {
            // get all usernames
            usernames = info.acl.get_all_usernames();
        } else {
            // only self
            usernames.push_back(info.self_name);
        }
    } else {
        // read usernames from decoder
        for (int x = 0; x < len; x++) {
            std::string name = decoder.to_string();
            if ((info.self_perms & permissions::admin) <= 0 && info.self_name != name) {
                encoder.clear_buffer();
                encoder.from_nak();
                encoder.encode_error(errors::permission_denied_self);
                return -3; // not easily recoverable, as other names may follow
            }
            if (!info.user->is_admin() && info.self_name != name) {
                encoder.clear_buffer();
                encoder.from_nak();
                encoder.encode_error(errors::permission_denied);
                return -3; // not easily recoverable, as other names may follow
            }
            if (!info.acl.has_user(name)) {
                encoder.clear_buffer();
                encoder.from_nak();
                encoder.encode_error(errors::user_not_found);
                return -3;
            }
            usernames.push_back(name);
        }
    }

    // now all names in usernames are allowed to be read
    CopiedValue res{};
    res.type = ValueType::MAP;
    auto &map = res.data.emplace<CopiedValue::copied_map_t>();
    for (auto &username : usernames) {
        auto perms = info.acl.get_user_permissions_pair(username);
        if (!perms) continue;
        map.emplace(username, std::make_unique<CopiedValue>(std::move(*perms)));
    }
    encoder.from_ack();
    encoder.from_general(res);
    return 0;
}

int tcp2_utils::command::create_user(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string username = decoder.to_string();
    std::string pwd = decoder.to_string();
    unsigned char options = decoder.to_byte();

    if (info.acl.has_user(username)) {
        return errors::username_already_exists;
    }

    if (options > 1) {
        return errors::unknown_option;
    }

    bool is_admin = (options & 0x01) > 0;
    if (!info.acl.create_user(username, pwd, is_admin)) {
        // should never happen
        return errors::general_server_error;
    }

    encoder.from_ack();
    return 0;
}

int tcp2_utils::command::delete_user(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string username = decoder.to_string();

    if (!info.acl.has_user(username)) {
        return errors::user_not_found;
    }

    if (info.acl.user_is_connected(username)) {
        return errors::user_connected;
    }

    if (!info.acl.delete_user(username)) {
        // should never happen
        return errors::general_server_error;
    }

    encoder.from_ack();
    return 0;
}

int tcp2_utils::command::change_pwd(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string new_pwd = decoder.to_string();

    info.user->update_pwd(new_pwd);
    encoder.from_ack();
    return 0;
}


int tcp2_utils::command::safe_follow(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string key_str = decoder.to_string();
    unsigned long long max_jumps = decoder.to_expanding_unsigned();
    if (max_jumps == 0) {
        return errors::invalid_value;
    }
    std::optional<CopiedValue> res = std::nullopt;

    for (int jumps = 0; jumps <= max_jumps; jumps++) {
        auto key = check_and_create_key(key_str);
        if (!key) {
            return errors::invalid_key;
        }

        if (!info.user->has_permission(*key, permissions::read)) {
            return errors::permission_denied;
        }

        res = info.db.get(key_str.c_str());
        if (!res) {
            return errors::key_not_found;
        }

        if (res->type == ValueType::REFERENCE) {
            if (!std::holds_alternative<std::string>(res->data)) {
                // this should never happen
                return errors::general_server_error;
            }
            key_str = std::get<std::string>(res->data);
        } else {
            break;
        }
    }

    encoder.from_ack();
    if (!encoder.from_general(*res)) {
        // this should normally never happen, as the data here is extracted directly from the db and therefore correct
        encoder.clear_buffer();
        encoder.from_nak();
        encoder.encode_error(errors::general_server_error);
        return -3;
    }
    return 0;
}

int tcp2_utils::command::forced_follow(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string key_str = decoder.to_string();
    unsigned long long jumps = decoder.to_expanding_unsigned();

    auto key = check_and_create_key(key_str);

    if (!key) {
        return errors::invalid_key;
    }

    if (!info.user->has_permission(*key, permissions::read)) {
        return errors::permission_denied;
    }

    auto res = info.db.follow(key_str.c_str(), jumps);
    if (!res) {
        return errors::key_not_found;
    }

    encoder.from_ack();
    if (!encoder.from_general(*res)) {
        // this should normally never happen, as the data here is extracted directly from the db and therefore correct
        encoder.clear_buffer();
        encoder.from_nak();
        encoder.encode_error(errors::general_server_error);
        return -3;
    }
    return 0;
}

int set_helper(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info, bool force) {
    std::string key_str = decoder.to_string();
    auto val = decoder.to_general();
    auto key = tcp2_utils::check_and_create_key(key_str);

    if (!key) {
        return errors::invalid_key;
    }

    if (!info.user->has_permission(*key, force ? tcp2_utils::permissions::write_del : tcp2_utils::permissions::write)) {
        return errors::permission_denied;
    }

    if (!val) {
        return errors::invalid_value;
    }

    try {
        info.db.set(key_str.c_str(), std::move(*val), force);
        encoder.from_ack();
    } catch (...) {
        return errors::general_server_error;
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
        return errors::invalid_key;
    }
    if (!info.user->has_permission(*key, permissions::del)) {
        return errors::permission_denied;
    }

    try {
        info.db.remove(key_str.c_str());
        encoder.from_ack();
    } catch (...) {
        // can this even happen?
        return errors::general_server_error;
    }
    return 0;
}

int tcp2_utils::command::list(TCP2Decoder &decoder, TCP2Encoder &encoder, CommandInfo &info) {
    std::string key_str = decoder.to_string();
    auto key = check_and_create_key(key_str);

    if (!key) {
        return errors::invalid_key;
    }

    if (!info.user->has_permission(*key, permissions::read)) {
        return errors::permission_denied;
    }

    auto ret = info.db.list(key_str.c_str());
    encoder.from_ack();
    encoder.from_list_t(ret);
    return 0;
}

void tcp2_utils::client_state_thread(TCP2Decoder &decoder, TCP2Encoder &encoder, ASE::ACL &acl, DB &db) {
    std::string username;
    ACLUser *user{nullptr};
    unsigned char self_perms{};

    std::unique_ptr<CommandInfo> ci;
    unsigned long long c_success = 0;
    unsigned long long c_recovered = 0;
    unsigned long long c_fail = 0;

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
            ci = std::make_unique<CommandInfo>(user, acl, db, username, self_perms);
        } else if (state == 2) {
            // logged in and ready for commands
            int temp_ret = client_command_switch(decoder, encoder, *ci);
#ifdef DEBUG
            std::cout << "> command result: " << temp_ret << std::endl;
#endif
            switch (temp_ret) {
                case -99: // hard shutdown
                    decoder.set_kys();
                    continue; // directly returns to loop and breaks free
                case -9: // soft shutdown
                    state = -1;
                    break; // allows to write before breaking free
                case -3: // error, internal server problem, encoder is destroyed and was already cleared
                    // this might in many cases not be recoverable
                    // batching must be stopped here
                    break;
                case -2: // error, not recovered; can not trust decoder
                    decoder.clear_buffer();
                    c_fail++;
                    break;
                case -1: // error, but recovered
                    c_recovered++;
                    break;
                case 0: // executed fine
                    c_success++;
                    break;
                default:
                    if (temp_ret > 0 && temp_ret <= 0xFFFF) {
                        // returns the error message code
                        // can be recovered, same as return value -1 except that nak and the error message still have to be written
                        encoder.from_nak();
                        encoder.encode_error(temp_ret);
                        c_recovered++;
                    } else {
                        // undefined -> soft shutdown
                        state = -1;
                    }
            }
        }
        // send buffer
        while (!encoder.is_empty()) {
            // stop is only valid if the connection is achieved and the user is logged in
            if (state == 2 && decoder.is_stop()) {
                //decoder.clear_buffer();
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
    std::cout << "SUCCESSFUL: " << c_success << std::endl;
    std::cout << "RECOVERED: " << c_recovered << std::endl;
    std::cout << "FAILED: " << c_fail << std::endl;
    acl.disconnect(username);
    decoder.set_kys();
}


void tcp2_utils::client_main_thread(std::atomic<bool>& kys, TCP2Decoder &decoder, TCP2Encoder &encoder, ASE::ACL &acl, DB &db) {
    std::thread state(tcp2_utils::client_state_thread, std::ref(decoder), std::ref(encoder), std::ref(acl), std::ref(db));

    while (!kys && decoder.is_healthy()) {
        decoder.read_data();
    }

    decoder.set_kys();
    state.join();
}

