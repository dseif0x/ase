/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <iostream>
#include <ranges>

#include "shmem.h"
#include "stopwatch.h"

struct value {
    int a;
    int b;
};

int main() {
    {
        shmem test;
        test.destroy();
    }
    stopwatch s;
    shmem test;
    std::vector<offset_ptr<value>> values;
    for (int i : std::views::iota(0, 1024 * 1024)) {
        auto ptr = test.construct<value>();
        ptr.atomic_write(test, [i](value* ptr) {
            ptr->a = i;
            ptr->b = i * 2;
        });

        //std::cout << ptr->a << " " << ptr->b << std::endl;
        values.push_back(ptr);
    }

    for (auto& ptr : values) {
        ptr.atomic_read(test, [](const value* ptr) {
            //std::cout << ptr->a << " " << ptr->b << std::endl;
        });
    }
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(s.peek()) << std::endl;

    test.destroy();
    return 0;
}
