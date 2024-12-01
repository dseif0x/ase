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

/**
 * @brief Represents a node in the permission hierarchy.
 */
struct ACLNode {
    // permissions:
    // 0x4: delete
    // 0x2: write
    // 0x1: read
    unsigned char permissions;
    std::unordered_map<std::string, ACLNode> subs;

    /**
     * @brief Checks if a node has the required permissions.
     *
     * @param perms The permissions to check.
     * @return True if the node has all the required permissions, otherwise false.
     */
    constexpr bool has_perms(unsigned char perms) const;
};

/**
 * @brief Represents a user in the ACL system with specific permissions and attributes.
 */
class ACLUser {
    std::string pwd;
    bool is_admin;

    ACLNode scope;

public:
    ACLUser(const std::string& password, bool isAdmin);
    ACLUser();

    /**
     * @brief Checks if a user has the required permissions for a given key.
     *
     * @param key A vector of strings representing the key in the permission hierarchy.
     * @param permissions The required permissions to check.
     * @return True if the user has the required permissions, otherwise false.
     */
    bool has_permission(std::vector<std::string> key, unsigned char permissions);

    /**
     * @brief Verifies the user's password.
     *
     * @param pwd The password to verify.
     * @return True if the password matches the stored password, otherwise false.
     */
    bool check_pwd(const std::string& pwd);

    /**
     * @brief Generates a string representation of the user's data.
     *
     * @return A string containing the password and admin status of the user.
     */
    std::string print_user_data();
};


/**
 * @brief Manages users and their access permissions in the ACL system.
 */
class ACL {
    std::unordered_map<std::string, ACLUser> users;
    std::set<std::string> open_connections;

    std::mutex general_lock; // todo think about locking individual users
    // todo or transfer the ability to search for permissions into the ACLUser and just transfer the struct back if changes were made to the scope

public:
    ACL(); // create default with one admin
    explicit ACL(std::string path); // load acl from disk

    /**
     * @brief Attempts to log in a user.
     *
     * @param user The username.
     * @param pwd The user's password.
     * @return An integer status code:
     *         - 0: Login successful.
     *         - 1: User does not exist.
     *         - 2: User is already logged in.
     *         - 3: Incorrect password.
     */
    int try_login(const std::string &user, const std::string &pwd);
    /**
     * @brief Retrieves a pointer to a user's data.
     *
     * @param user The username.
     * @return A pointer to the ACLUser object if the user is logged in, otherwise nullptr.
     */
    ACLUser *get_user(const std::string &user);
    /**
     * @brief Disconnects a logged-in user.
     *
     * @param user The username to disconnect.
     */
    void disconnect(const std::string &user);

    /**
     * @brief Prints detailed information about all users.
     */
    void print_full_info();
};


#endif //ACL_H
