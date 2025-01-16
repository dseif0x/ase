/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <optional>
#include "db.h"

template <typename T>
ValueType get_type();

template <>
ValueType get_type<bool>() {
    return ValueType::BOOLEAN;
}

template <>
ValueType get_type<int>() {
    return ValueType::INT;
}

template <>
ValueType get_type<std::string>() {
    return ValueType::STRING;
}

template <>
ValueType get_type<double>() {
    return ValueType::DOUBLE;
}

template <>
ValueType get_type<CopiedValue::copied_array_t>() {
    return ValueType::ARRAY;
}

template <>
ValueType get_type<CopiedValue::copied_pair_t>() {
    return ValueType::PAIR;
}

template <>
ValueType get_type<CopiedValue::copied_map_t>() {
    return ValueType::MAP;
}

template <typename T>
std::optional<T> get(DB& db, const char* key) {
    std::optional<CopiedValue> val = db.get(key);
    if (!val.has_value()) return std::nullopt;
    auto type = get_type<T>();
    if (val->type == type) {
        return {std::move(get<T>(val->data))};
    } else {
        // Found but with wrong type
        return std::nullopt;
    }
}

template <typename T>
std::optional<T> follow(DB& db, const char* key, uint16_t max_chains) {
    std::optional<CopiedValue> val = db.follow(key, max_chains);
    if (!val.has_value()) return std::nullopt;
    auto type = get_type<T>();
    if (val->type == type) {
        return {std::move(get<T>(val->data))};
    } else {
        // Found but with wrong type
        return std::nullopt;
    }
}