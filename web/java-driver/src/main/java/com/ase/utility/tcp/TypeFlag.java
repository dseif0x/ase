package com.ase.utility.tcp;

public enum TypeFlag {
    ARRAY(0x80),
    PAIR(0x40),
    COMPLEX(0x20),
    KEY(0x10);

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