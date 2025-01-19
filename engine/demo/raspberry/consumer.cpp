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
    db.set("consumer_stop", std::move(stopval), true);

    while (true) {
        auto prodtime = db.get("producer_time");
        if (prodtime.has_value() && prodtime->type == ValueType::DOUBLE) {
            std::cout << "Producer time: " << static_cast<long long>(get<double>(prodtime->data)) << std::endl;
        }

        auto shallstop = db.get("consumer_stop");
        if (shallstop.has_value() && shallstop->type == ValueType::BOOLEAN && get<bool>(shallstop->data)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Stopped consumer" << std::endl;
    return 0;
}