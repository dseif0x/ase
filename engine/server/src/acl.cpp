/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */


#include "acl.h"

#include <iostream>
#include <tcp2_utils.h>
#include <vector>

ACLUser::ACLUser(const std::string& password, bool isAdmin)
    : pwd(password), is_admin(isAdmin) {
    CopiedValue perms;
    perms.type = ValueType::INT;
    perms.data = tcp2_utils::permissions::all;
    CopiedValue::copied_map_t map;
    map.emplace("$", std::make_unique<CopiedValue>(std::move(perms)));

    this->permissions = CopiedValue{};
    this->permissions.type = ValueType::MAP;
    this->permissions.data.emplace<CopiedValue::copied_map_t>(std::move(map));
}

ACLUser::ACLUser() : is_admin(false), permissions() {}


ACL::ACL() {
    this->users["admin"] = ACLUser("rootpwd", true);
}

constexpr bool ACLNode::has_perms(const unsigned char perms) const {
    return ((perms & this->permissions) == perms);
}

bool ACLUser::has_permission(std::vector<std::string> key, unsigned char permissions) {
    this->permissions.print();
    auto *cur_node = &this->permissions;
    std::cout << std::endl;
    int i = 0;
    for (auto &x : key) {
        cur_node->print();
        std::cout << " -> i: " << i << std::endl;
        auto &cur_map = std::get<CopiedValue::copied_map_t>(cur_node->data);
        if (!cur_map.contains("$")) return false;
        unsigned char cur_perm = std::get<int>(cur_map["$"]->data);
        if ((cur_perm | permissions) == cur_perm) {
            // permission found
            return true;
        }
        if (!cur_map.contains(x)) return false; // cant go any deeper
        cur_node = &(*cur_map[x]);
    }
    return false;
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

