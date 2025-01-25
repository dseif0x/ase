/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.utility.tcp;
/**
 * Enumeration representing simple data types.
 * Extend this enumeration based on your protocol's supported types.
 */
public enum SimpleType {
    UNKNOWN(0x00),
    BOOLEAN(0x01),
    INT(0x02),
    STRING(0x03),
    DOUBLE(0x04),
    SEMAPHORE(0x05),
    REFERENCE(0x06);

    /**
     * The type ID.
     */
    private final int typeId;

    SimpleType(int typeId) {
        this.typeId = typeId;
    }

    /**
     * Retrieves the type ID.
     *
     * @return The type ID.
     */
    public int getTypeId() {
        return typeId;
    }

    /**
     * Retrieves the SimpleType based on the provided type ID.
     *
     * @param typeId The type ID to lookup.
     * @return The corresponding SimpleType.
     * @throws IllegalArgumentException if the type ID is unknown.
     */
    public static SimpleType fromTypeId(int typeId) {
        for (SimpleType type : values()) {
            if (type.typeId == typeId) {
                return type;
            }
        }
        throw new IllegalArgumentException("Unknown SimpleType ID: " + typeId);
    }
}