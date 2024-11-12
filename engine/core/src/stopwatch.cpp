/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include "stopwatch.h"
stopwatch::stopwatch(bool beginDirectly) {
    if(beginDirectly){
        startPoint = getCurrentTimePoint();
    }
}

time_point stopwatch::getCurrentTimePoint() {
    return std::chrono::high_resolution_clock::now();
}

duration stopwatch::peek() {
    return getCurrentTimePoint() - startPoint;
}

void stopwatch::reset() {
    startPoint = getCurrentTimePoint();
}

void stopwatch::start() {
    reset();
}

duration stopwatch::stop() {
    return peek();
}