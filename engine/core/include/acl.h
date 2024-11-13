/*
* Copyright 2024 Project Contributors
*
* This source code is licensed under the MIT License found in the LICENSE file in the root directory
* of this source tree or at https://opensource.org/licenses/MIT.
*/

#ifndef ACL_H
#define ACL_H
#include <mutex>
#include <string>
#include <unordered_map>

struct ACLNode {
    // permissions:
    // 0x4: delete
    // 0x2: write
    // 0x1: read
    unsigned char permissions;
    std::unordered_map<std::string, ACLNode> subs;
};



struct ACLUser {
    std::string pwd;
    bool is_admin;

    ACLNode scope;

    ACLUser(const std::string& password, bool isAdmin);
    ACLUser();
};



class ACL {
    std::unordered_map<std::string, ACLUser> users;

    std::mutex general_lock; // todo think about locking individual users
    // todo or transfer the ability to search for permissions into the ACLUser and just transfer the struct back if changes were made to the scope

public:
    ACL(); // create default with one admin
    explicit ACL(std::string path); // load acl from disk

    bool has_permission(const std::string &user, std::vector<std::string> key, unsigned char permissions);
    bool successful_login(const std::string &user, const std::string &pwd);

    void print_full_info();
};


#endif //ACL_H
