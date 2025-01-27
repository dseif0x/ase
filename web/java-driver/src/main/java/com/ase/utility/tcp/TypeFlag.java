/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.utility.tcp;

public enum TypeFlag {
    ARRAY(0x80),
    PAIR(0x40),
    COMPLEX(0x20),
    KEY(0x10),
    ONLY_TYPE(0x08);

    /**
     * The bitmask for the type flag.
     */
    private final int bitmask;

    TypeFlag(int bitmask) {
        this.bitmask = bitmask;
    }

    /**
     * Retrieves the bitmask.
     *
     * @return The bitmask.
     */
    public int getBitmask() {
        return bitmask;
    }
}