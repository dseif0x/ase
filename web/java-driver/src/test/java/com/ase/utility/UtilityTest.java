package com.ase.utility;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class UtilityTest {

    @Test
    void getMinValue() {
        assertEquals(-9223372036854775808L, Utility.getMinValue(4096));
    }
}