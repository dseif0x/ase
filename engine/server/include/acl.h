/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#ifndef ACL_H
#define ACL_H
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "db_types.h"

class ACLUser;

class ACL {
    std::unordered_map<std::string, ACLUser> users;
    std::set<std::string> open_connections;

    std::mutex general_lock; // todo think about locking individual users
    // todo or transfer the ability to search for permissions into the ACLUser and just transfer the struct back if changes were made to the scope

public:
    ACL(); // create default with one admin

    bool load_from_disk(std::string path); // load acl from disk
    bool save_to_disk(std::string path);

    int try_login(const std::string &user, const std::string &pwd);
    ACLUser *get_user(const std::string &user);
    void disconnect(const std::string &user);

    bool has_user(std::string &username);

    bool update_key_permission(std::string user, std::vector<std::string> key, unsigned char permissions);

    void print_full_info();
};

class ACLUser {
    //todo might have to add a lock for access of userdata
    std::string pwd;
    bool admin;

    // Tree representation of permissions
    // All elements except for the leafs are of type MAP
    // each map contains one key "$" which points to a CopiedValue of type int storing the actual permission
    CopiedValue permissions;

public:
    ACLUser(const std::string& password, bool isAdmin);
    ACLUser();

    bool has_permission(std::vector<std::string> key, unsigned char permissions);
    bool check_pwd(const std::string& pwd);
    bool is_admin();

    void set_permissions(CopiedValue &value);
    void serialize_user(std::vector<CopiedValue> &value);

    std::string print_user_data();

    void update_key_permission(std::vector<std::string> key, unsigned char permissions);
};


#endif //ACL_H
