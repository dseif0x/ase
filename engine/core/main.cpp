/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <ranges>
#include <mutex>
#include <thread>
#include <unistd.h>

#include "shmem.h"
#include "stopwatch.h"

struct value {
    int a;
    int b;
    uint8_t test[256];
};

int main() {
    {
        shmem test;
        test.destroy();
    }
    pid_t pid = fork();
    stopwatch s;
    if (pid == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    shmem test;
    std::cout << "Attached" << std::endl;
    std::vector<offset_ptr<value>> values;
    for (int i : std::views::iota(0, 1024 * 1024)) {
        auto ptr = test.construct<value>();
        ptr.atomic_access(test, [i](value* ptr) {
            ptr->a = i;
            ptr->b = i * 2;
        });

        //std::cout << ptr->a << " " << ptr->b << std::endl;
        values.push_back(ptr);
    }

    for (auto& ptr : values) {
        ptr.atomic_access(test, [](const value* ptr) {
            //std::cout << ptr->a << " " << ptr->b << std::endl;
        });
    }
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(s.peek()) << std::endl;

    if (pid == 0) {
    } else {
        waitpid(pid, nullptr, 0);
        test.destroy();
    }
    return 0;
}
