/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <chrono>

typedef std::chrono::high_resolution_clock::time_point time_point;
typedef std::chrono::high_resolution_clock::duration duration;

/**
 * @brief A simple stopwatch utility for measuring elapsed time.
 *
 * This class provides functionality to measure the time between a start and stop event.
 * It allows starting, stopping, resetting, and peeking the current elapsed time.
 */

class stopwatch {
private:
    time_point startPoint;

public:
    static time_point getCurrentTimePoint();

    stopwatch() : stopwatch{true} {};
    stopwatch(bool beginDirectly);

    void start();
    void reset();
    duration stop();
    duration peek();
};