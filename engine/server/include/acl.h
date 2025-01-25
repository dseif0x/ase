/*
 * Copyright 2025 Project Contributors
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
#include <optional>

class ACLUser;


namespace ASE {
    /**
         * @brief Manages users and their access permissions in the ACL system.
         */
    class ACL {
        std::unordered_map<std::string, ACLUser> users;
        std::set<std::string> open_connections;

        std::mutex general_lock; // todo think about locking individual users

    public:
        ACL(); //!< create default with one admin

        bool load_from_disk(std::string path); //!< load acl from disk
        bool save_to_disk(std::string path); //!< save acl to disk

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
        * Checks if a user exists
        *
        * @param username the username to check for
        * @return whether a user with the given name exists or not
        */
        bool has_user(std::string &username);

        /**
        * Checks if a user is connected
        *
        * @param username the username to check for
        * @return whether the user is connected or not
        */
        bool user_is_connected(std::string &username);

        /**
        * Updates the permissions of a given user
        *
        * @param user the username to check for
        * @param key the split key
        * @param permissions the new permission value
        * @return whether the operation was successful
        */
        bool update_key_permission(std::string user, std::vector<std::string> key, unsigned char permissions);


        /**
        * @brief Creates a user with the provided parameters and returns true on success.
        */
        bool create_user(std::string &username, std::string &pwd, bool is_admin);

        /**
        * @brief Deletes a user from the acl.
        */
        bool delete_user(std::string &username);

        /**
         * @brief Prints detailed information about all users.
         */
        void print_full_info();

        /**
        * @brief returns all usernames
        */
        std::vector<std::string> get_all_usernames();

        /**
        * gets the permission and an indication if the user is admin
        * @param name the username
        * @return If value where found, a @ref CopiedValue of type PAIR is returned where the first value
        * indicates that the user is admin and the second contains the ACL tree
        */
        std::optional<CopiedValue> get_user_permissions_pair(std::string &name);
    };
}

/**
 * @brief Represents a user in the ACL system with specific permissions and attributes.
 */
class ACLUser {
    std::string pwd;
    bool admin;
    std::unique_ptr<std::mutex> user_lock;

    // Tree representation of permissions
    // All elements except for the leafs are of type MAP
    // each map contains one key "$" which points to a CopiedValue of type int storing the actual permission
    CopiedValue permissions;

public:
    /**
    * constructs a new user
    * @param password the password of the user
    * @param isAdmin indicates if the user should be an admin
    */
    ACLUser(const std::string& password, bool isAdmin);
    ACLUser() = default; //!< the default constructor

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
    * @brief Update the password of the user.
    */
    void update_pwd(const std::string& pwd);
    bool is_admin(); //!< checks if the user is admin

    void set_permissions(CopiedValue &value); //!< override ALL permissions of the user
    void serialize_user(std::vector<CopiedValue> &value); //!< serializes the data of the user
    CopiedValue get_user_permissions_pair(); //!< gets the user permissions as well as an indicator if the user is admin. see @ref ASE::ACL::get_user_permissions_pair

    /**
     * @brief Generates a string representation of the user's data.
     *
     * @return A string containing the password and admin status of the user.
     */
    std::string print_user_data();

    void update_key_permission(std::vector<std::string> key, unsigned char permissions); //!< set the permission of the user. see @ref ASE::ACL::update_key_permission
};


#endif //ACL_H
