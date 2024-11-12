//
// Created by Tobias Panek on 12.11.24.
//


#include "acl.h"

#include <vector>

ACLUser::ACLUser(const std::string& password, bool isAdmin)
    : is_admin(isAdmin) {}


ACL::ACL() {
    //this->users.insert({"admin", ACLUser("rootpwd", true)});
}

bool ACL::has_permission(const std::string &user, std::vector<std::string> key, unsigned char permissions) {
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
    if (!users.contains(user)) return false;
    return (users[user].pwd == pwd);
}

