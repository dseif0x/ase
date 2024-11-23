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

struct ACLNode {
    // permissions:
    // 0x4: delete
    // 0x2: write
    // 0x1: read
    unsigned char permissions;
    std::unordered_map<std::string, ACLNode> subs;

    constexpr bool has_perms(unsigned char perms) const;
};


class ACLUser {
    std::string pwd;
    bool is_admin;

    // Tree representation of permissions
    // All elements except for the leafs are of type MAP
    // each map contains one key "$" which points to a CopiedValue of type int storing the actual permission
    CopiedValue permissions;

public:
    ACLUser(const std::string& password, bool isAdmin);
    ACLUser();

    bool has_permission(std::vector<std::string> key, unsigned char permissions);
    bool check_pwd(const std::string& pwd);

    std::string print_user_data();
};



class ACL {
    std::unordered_map<std::string, ACLUser> users;
    std::set<std::string> open_connections;

    std::mutex general_lock; // todo think about locking individual users
    // todo or transfer the ability to search for permissions into the ACLUser and just transfer the struct back if changes were made to the scope

public:
    ACL(); // create default with one admin
    explicit ACL(std::string path); // load acl from disk

    int try_login(const std::string &user, const std::string &pwd);
    ACLUser *get_user(const std::string &user);
    void disconnect(const std::string &user);

    void print_full_info();
};


#endif //ACL_H
