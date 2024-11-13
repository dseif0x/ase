/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include <acl.h>


constexpr int BUFFER_SIZE = 8;


int start_tcp_server(int port, ACL &acl);

#endif //TCP_SERVER_H
