/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <db.h>

int main() {
    DB db;

    CopiedValue stopval{};
    stopval.type = ValueType::BOOLEAN;
    stopval.data = false;
    db.set("producer_stop", std::move(stopval), true);

    while (true) {
        auto time = std::chrono::system_clock::now().time_since_epoch();
        auto producer_time = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();

        CopiedValue newval{};
        newval.type = ValueType::DOUBLE;
        newval.data = static_cast<double>(producer_time);
        db.set("producer_time", std::move(newval), true);

        auto shallstop = db.get("producer_stop");
        if (shallstop.has_value() && shallstop->type == ValueType::BOOLEAN && get<bool>(shallstop->data)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Stopped producer" << std::endl;
    return 0;
}