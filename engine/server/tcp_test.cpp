/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <thread>

#include "tcp2_server.h"
#include "acl.h"


int main() {
    {
        DB db;
        db.destroy();
    }
    DB db{};

    {
        CopiedValue val{};
        val.type = ValueType::INT;
        val.data = 15;
        db.set(".top.second.sub.leaf", val);
    }
    {
        auto temp = db.get(".top.second.sub.leaf");
        if (temp) {
            std::cout << "value:";
            temp->print();
            std::cout << std::endl;
        } else {
            std::cout << "value not found" << std::endl;
        }
    }

    { // Integer
        CopiedValue newval{};
        newval.type = ValueType::INT;
        newval.data.emplace<int>(42);
        db.set("test1", newval);
        newval.data.emplace<int>(43);
        db.set("test2", newval);
    }

    { // String
        CopiedValue newval2{};
        newval2.type = ValueType::STRING;
        newval2.data.emplace<std::string>("This are news from ORF.");
        db.set("at.orf.news", newval2);
        newval2.data.emplace<std::string>("This is tvtheke.");
        db.set("at.orf.tv", newval2);
        newval2.data.emplace<std::string>("This is sport.");
        db.set("at.orf.sport", newval2);
    }

    std::cout << "after db" << std::endl;

    ACL acl{};

    if (acl.load_from_disk("test.bin")) {
        std::cout << "acl loaded from disk" << std::endl;
    } else {
        std::cout << "could not load from disk" << std::endl;
    }



    TCP2Server server{acl, db, 42069, 10};

    server.start();

    std::string temp;
    std::cin >> temp;

    server.stop();
    acl.save_to_disk("test.bin");

    return 0;
}