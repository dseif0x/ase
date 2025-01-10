/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "decoder.h"

#include "acl.h"

#include <iostream>
#include <tcp2_utils.h>
#include <vector>

ACLUser::ACLUser(const std::string& password, bool isAdmin)
    : pwd(password), admin(isAdmin), user_lock(std::make_unique<std::mutex>()) {
    CopiedValue perms;
    perms.type = ValueType::INT;
    perms.data = isAdmin ? tcp2_utils::permissions::all : tcp2_utils::permissions::none;
    CopiedValue::copied_map_t map;
    map.emplace("$", std::make_unique<CopiedValue>(std::move(perms)));

    this->permissions = CopiedValue{};
    this->permissions.type = ValueType::MAP;
    this->permissions.data.emplace<CopiedValue::copied_map_t>(std::move(map));
}


ASE::ACL::ACL() {
    auto temp = ACLUser("rootpwd", true);
    this->users.emplace("admin", std::move(temp));
}

bool ASE::ACL::load_from_disk(std::string path) {
    // format: [ ["username", "pwd", bool <is_admin>, {scope}], ...]
    try {
        FileDecoder decoder{path};
        decoder.read_data();
        auto data = decoder.to_general();
        data->print();
        std::cout << std::endl;
        if (!data) return false;
        if (data->type != ValueType::ARRAY) return false;
        auto &elems = std::get<CopiedValue::copied_array_t>(data->data);
        for (auto &elem : elems) {
            if (elem.type != ValueType::ARRAY) return false;
            auto &ud = std::get<CopiedValue::copied_array_t>(elem.data);
            if (ud.size() != 4) return false;
            if (ud[0].type != ValueType::STRING || ud[1].type != ValueType::STRING || ud[2].type != ValueType::BOOLEAN
                || ud[3].type != ValueType::MAP) return false;
            auto u = ACLUser(std::get<std::string>(ud[1].data), std::get<bool>(ud[2].data));
            u.set_permissions(ud[3]);
            this->users.emplace(std::get<std::string>(ud[0].data), std::move(u));
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool ASE::ACL::save_to_disk(std::string path) {
    try {
        FileEncoder encoder{path};
        CopiedValue val{};
        val.type = ValueType::ARRAY;
        auto &vec = val.data.emplace<CopiedValue::copied_array_t>();
        for (auto &user : this->users) {
            CopiedValue cur{};
            cur.type = ValueType::ARRAY;
            auto &cur_vec = cur.data.emplace<CopiedValue::copied_array_t>();
            CopiedValue username{};
            username.type = ValueType::STRING;
            username.data = user.first;
            cur_vec.emplace_back(std::move(username));
            user.second.serialize_user(cur_vec);
            vec.emplace_back(std::move(cur));
        }
        std::cout << "saving following data:";
        val.print();
        std::cout << std::endl;
        encoder.from_general(val);
        std::cout << "encode worked" << std::endl;
        encoder.write_data();
        return true;
    } catch (...) {
        return false;
    }
}

bool ACLUser::has_permission(std::vector<std::string> key, unsigned char permissions) {
    std::unique_lock<std::mutex> lock(*this->user_lock);
    auto *cur_node = &this->permissions;
    for (int i = 0; i < key.size()+1; i++) {
        auto &cur_map = std::get<CopiedValue::copied_map_t>(cur_node->data);
        if (!cur_map.contains("$")) return false;
        unsigned char cur_perm = std::get<int>(cur_map["$"]->data);
        permissions -= permissions & cur_perm;
        if (permissions == 0) {
            // permission now filled
            return true;
        }
        if (i >= key.size() || !cur_map.contains(key[i])) return false; // cant go any deeper
        cur_node = &(*cur_map[key[i]]);
    }
    return false;
}

// if new permissions are added above existing ones, the old ones are dropped, even if the new permissions are lower.
//  this allows the deletion of permissions.
void ACLUser::update_key_permission(std::vector<std::string> key, unsigned char permissions) {
    std::unique_lock<std::mutex> lock(*this->user_lock);
#ifdef DEBUG
    std::cout << "permissions before:";
    this->permissions.print();
    std::cout << std::endl;
#endif
    auto *cur_node = &this->permissions;
    int i = 0;
    for (auto &x : key) {
        auto &cur_map = std::get<CopiedValue::copied_map_t>(cur_node->data);
        if (!cur_map.contains(x)) {
            CopiedValue perms;
            perms.type = ValueType::INT;
            perms.data = tcp2_utils::permissions::none;
            CopiedValue::copied_map_t map;
            map.emplace("$", std::make_unique<CopiedValue>(std::move(perms)));

            CopiedValue temp{};
            temp.type = ValueType::MAP;
            temp.data.emplace<CopiedValue::copied_map_t>(std::move(map));

            cur_map.emplace(x, std::make_unique<CopiedValue>(std::move(temp)));
        }
        cur_node = &(*cur_map[x]);
    }
    // remove all keys, and only add the $ key with the correct permissions
    CopiedValue perms;
    perms.type = ValueType::INT;
    perms.data = permissions;
    CopiedValue::copied_map_t map;
    map.emplace("$", std::make_unique<CopiedValue>(std::move(perms)));
    cur_node->data.emplace<CopiedValue::copied_map_t>(std::move(map));
#ifdef DEBUG
    std::cout << "permissions after:";
    this->permissions.print();
    std::cout << std::endl;
#endif
}

bool ACLUser::check_pwd(const std::string &pwd) {
    std::unique_lock<std::mutex> lock(*this->user_lock);
    return this->pwd == pwd;
}

bool ACLUser::is_admin() {
    std::unique_lock<std::mutex> lock(*this->user_lock);
    return this->admin;
}

void ACLUser::set_permissions(CopiedValue &value) {
    std::unique_lock<std::mutex> lock(*this->user_lock);
    this->permissions = std::move(value);
}


void ACLUser::serialize_user(std::vector<CopiedValue> &value) {
    std::unique_lock<std::mutex> lock(*this->user_lock);
    CopiedValue pwd{};
    pwd.type = ValueType::STRING;
    pwd.data = this->pwd;
    value.emplace_back(std::move(pwd));

    CopiedValue is_admin{};
    is_admin.type = ValueType::BOOLEAN;
    is_admin.data = this->admin;
    value.emplace_back(std::move(is_admin));

    value.emplace_back(std::move(this->permissions));
}

CopiedValue ACLUser::get_user_permissions_pair() {
    std::unique_lock<std::mutex> lock(*this->user_lock);
    auto adm = std::make_unique<CopiedValue>();
    adm->type = ValueType::BOOLEAN;
    adm->data.emplace<bool>(this->admin);
    auto perms = std::make_unique<CopiedValue>(CopiedValue::make_deep_copy(this->permissions));

    CopiedValue res{};
    res.type = ValueType::PAIR;
    auto &pair = res.data.emplace<CopiedValue::copied_pair_t>();
    pair.first = std::move(adm);
    pair.second = std::move(perms);

    return res;
}


std::string ACLUser::print_user_data() {
    std::unique_lock<std::mutex> lock(*this->user_lock);
    return this->pwd + (this->admin ? " [admin]" : "");
}

int ASE::ACL::try_login(const std::string &user, const std::string &pwd) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    if (!users.contains(user)) return 1;
    if (open_connections.contains(user)) return 2;
    if (!users[user].check_pwd(pwd)) return 3;

    // login successful; add user to open connection
    this->open_connections.insert(user);
    return 0;
}

ACLUser* ASE::ACL::get_user(const std::string &user) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    if (!open_connections.contains(user)) return nullptr;
    return &users[user];
}

void ASE::ACL::disconnect(const std::string &user) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    if (open_connections.contains(user)) open_connections.erase(user);
}

bool ASE::ACL::has_user(std::string &username) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    return this->users.contains(username);
}

bool ASE::ACL::user_is_connected(std::string &username) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    return this->open_connections.contains(username);
}

bool ASE::ACL::update_key_permission(std::string user, std::vector<std::string> key, unsigned char permissions) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    if (!this->users.contains(user)) return false;
    this->users[user].update_key_permission(key, permissions);
    return true;
}

bool ASE::ACL::create_user(std::string &username, std::string &pwd, bool is_admin) {
    std::lock_guard<std::mutex> lock(this->general_lock);
    if (this->users.contains(username)) return false;
    this->users[username] = ACLUser(pwd, is_admin);
    return true;
}

bool ASE::ACL::delete_user(std::string &username) {
    if (!this->has_user(username) || this->user_is_connected(username)) return false;
    std::lock_guard<std::mutex> lock(this->general_lock);
    if (this->users.erase(username) != 1) return false;
    return true;
}

void ASE::ACL::print_full_info() {
    std::lock_guard<std::mutex> lock(this->general_lock);
    for (auto &x : this->users) {
        std::cout << x.first << ":" << x.second.print_user_data() << std::endl;
    }
}

std::vector<std::string> ASE::ACL::get_all_usernames() {
    std::vector<std::string> res;
    for (auto &x : this->users) res.push_back(x.first);
    return res;
}

void ACLUser::update_pwd(const std::string& pwd) {
    std::unique_lock<std::mutex> lock(*this->user_lock);
    this->pwd = pwd;
}

