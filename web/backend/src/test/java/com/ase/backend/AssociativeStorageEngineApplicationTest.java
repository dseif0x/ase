package com.ase.backend;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class AssociativeStorageEngineApplicationTest {

    @Test
    void testMul() {
        assertEquals(15, AssociativeStorageEngineApplication.testMul(3, 5));
    }
}