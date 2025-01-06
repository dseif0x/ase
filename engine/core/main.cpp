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

#include "shmem.h"
#include "stopwatch.h"

struct value {
    int a;
    int b;
    uint8_t test[256];
};

void run(int count) {
    stopwatch s;
    shmem test;
    std::cout << "Attached" << std::endl;
    for (int i : std::views::iota(0, count)) {
        auto ptr = test.construct<value>();
        test.destroy(ptr);
    }
    std::cout << "Finished with dealloc test..." << std::endl;

    for (int i : std::views::iota(0, count)) {
        auto ptr = test.construct<value>();
        ptr.atomic_access(test, [i](value* ptr) {
            ptr->a = i;
            ptr->b = i * 2;
        });

        //std::cout << ptr->a << " " << ptr->b << std::endl;
    }

    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(s.peek()).count() << "ms" << std::endl;
}

int main() {
    {
        shmem test;
        test.destroy();
    }
    std::thread t1(run, 1024 * 32);
    std::thread t2(run, 1024 * 32);

    t1.join();
    t2.join();

    return 0;
}
