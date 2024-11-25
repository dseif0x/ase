/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */


#include "acl.h"

#include <iostream>
#include <vector>

ACLUser::ACLUser(const std::string& password, bool isAdmin)
    : pwd(password), is_admin(isAdmin), scope() {
}

ACLUser::ACLUser() : is_admin(false), scope() {}


ACL::ACL() {
    this->users["admin"] = ACLUser("rootpwd", true);
}

constexpr bool ACLNode::has_perms(const unsigned char perms) const {
    return ((perms & this->permissions) == perms);
}

bool ACLUser::has_permission(std::vector<std::string> key, unsigned char permissions) {
    ACLNode *curNode = &scope;
    for (auto &x : key) {
        if (curNode->has_perms(permissions)) return true; // Node containing necessary perms found
        if (!curNode->subs.contains(x)) return false; // No more specific permissions exist
        curNode = &curNode->subs[x];
    }
    return curNode->has_perms(permissions);
}

bool ACLUser::check_pwd(const std::string &pwd) {
    return this->pwd == pwd;
}

std::string ACLUser::print_user_data() {
    return this->pwd + (this->is_admin ? " [admin]" : "");
}

int ACL::try_login(const std::string &user, const std::string &pwd) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    if (!users.contains(user)) return 1;
    if (open_connections.contains(user)) return 2;
    if (!users[user].check_pwd(pwd)) return 3;

    // login successful; add user to open connection
    this->open_connections.insert(user);
    return 0;
}

ACLUser* ACL::get_user(const std::string &user) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    if (!open_connections.contains(user)) return nullptr;
    return &users[user];
}

void ACL::disconnect(const std::string &user) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    if (open_connections.contains(user)) open_connections.erase(user);
}

void ACL::print_full_info() {
    std::lock_guard<std::mutex> lock(this->general_lock);
    for (auto &x : this->users) {
        std::cout << x.first << ":" << x.second.print_user_data() << std::endl;
    }
}

