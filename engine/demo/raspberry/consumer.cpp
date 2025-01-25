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
    db.set("consumer.stop", std::move(stopval), true);

    CopiedValue pauseval{};
    pauseval.type = ValueType::BOOLEAN;
    pauseval.data = false;
    db.set("consumer.pause", std::move(pauseval), true);

    while (true) {
        auto shallpause = db.get("consumer.pause");
        if (shallpause.has_value() && shallpause->type == ValueType::BOOLEAN && get<bool>(shallpause->data)) {
            std::cout << "Paused consumer" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        auto prodtime = db.get("producer.time");
        if (prodtime.has_value() && prodtime->type == ValueType::DOUBLE) {
            std::cout << "Producer time: " << static_cast<long long>(get<double>(prodtime->data)) << std::endl;
        }

        auto shallstop = db.get("consumer.stop");
        if (shallstop.has_value() && shallstop->type == ValueType::BOOLEAN && get<bool>(shallstop->data)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Stopped consumer" << std::endl;
    return 0;
}