/*
 * Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <tuple>
#include <mutex>
#include <vector>
#include <thread>
#include <algorithm>
#include <shared_mutex>

struct shared_lock_type {
    static constexpr bool unique = false;
};

struct unique_lock_type {
    static constexpr bool unique = true;
};

// Helper function to lock with the appropriate mode
template <typename Mutex, typename LockType>
bool try_lock(Mutex& mtx, LockType) {
    if constexpr (LockType::unique) {
        return mtx.try_lock();
    } else {
        return mtx.try_lock_shared();
    }
}

// Helper function to unlock with the appropriate mode
template <typename Mutex, typename LockType>
void unlock(Mutex& mtx, LockType) {
    if constexpr (LockType::unique) {
        mtx.unlock();
    } else {
        mtx.unlock_shared();
    }
}

// This wil unlock all mutexes at indices inside locked_indices
template <std::size_t Index = 0, typename... mutexes, typename... lock_modes>
void unlock_index(std::vector<std::size_t>& locked_indices, std::tuple<mutexes, lock_modes>... mutex_modes) {
    if (std::any_of(locked_indices.begin(), locked_indices.end(), [](std::size_t i) { return i == Index; })) {
        auto &[mtx, mode] = std::get<Index>(std::forward_as_tuple(mutex_modes...));
        unlock(mtx, mode);
    }
    if constexpr (Index + 1 < sizeof...(mutex_modes)) {
        unlock_index<Index + 1>(locked_indices, mutex_modes...);
    }
}

template <std::size_t Index = 0, typename... mutexes, typename... lock_modes>
bool try_lock_all(std::vector<std::size_t>& locked_indices, std::tuple<mutexes, lock_modes>... mutex_modes) {
    if constexpr (Index < sizeof...(mutex_modes)) {
        auto &[mtx, mode] = std::get<Index>(std::forward_as_tuple(mutex_modes...));
        if (try_lock(mtx, mode)) {
            locked_indices.push_back(Index);
            return try_lock_all<Index + 1>(locked_indices, mutex_modes...);
        } else {
            return false;
        }
    }
    return true;
}

template <typename... mutexes, typename... lock_modes>
void custom_lock(std::tuple<mutexes, lock_modes>... mutex_modes) {
    while (true) {
        std::vector<std::size_t> locked_indices; // Track indices of successfully locked mutexes
        bool all_locked = try_lock_all(locked_indices, mutex_modes...);

        if (all_locked) {
            break;  // Success: all mutexes are locked
        }

        // If locking failed, unlock all acquired locks
        unlock_index(locked_indices, mutex_modes...);

        // Optional: small delay to avoid busy waiting
        std::this_thread::yield();
    }
}