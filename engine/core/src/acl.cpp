//
// Created by Tobias Panek on 12.11.24.
//


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

bool ACL::has_permission(const std::string &user, std::vector<std::string> key, unsigned char permissions) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    if (!users.contains(user)) return false;
    ACLNode *curNode = &users[user].scope;
    for (auto &x : key) {
        if ((permissions & curNode->permissions) == permissions) return true; // Node containing necessary perms found
        if (!curNode->subs.contains(x)) return false; // No more specific permissions exist
        curNode = &curNode->subs[x];
    }
    if ((permissions & curNode->permissions) == permissions) return true;
    return false;
}

bool ACL::successful_login(const std::string &user, const std::string &pwd) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    if (!users.contains(user)) return false;
    return (users[user].pwd == pwd);
}

void ACL::print_full_info() {
    std::lock_guard<std::mutex> lock(this->general_lock);
    for (auto &x : this->users) {
        std::cout << x.first << ":" << x.second.pwd << (x.second.is_admin ? " [admin]" : "") << std::endl;
    }
}

