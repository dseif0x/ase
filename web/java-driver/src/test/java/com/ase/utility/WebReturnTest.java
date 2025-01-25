/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.utility;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Test suite for the {@link WebReturn} class, validating constructors,
 * static factory methods, and utility functions.
 * <p>
 * Uses JUnit 5 for assertions and test validations.
 * </p>
 */
class WebReturnTest {

    /**
     * Tests the default constructor of {@link WebReturn}.
     * Ensures default values for fields are correctly initialized.
     */
    @Test
    void testDefaultConstructor() {
        WebReturn<Object> webReturn = new WebReturn<>();

        assertFalse(webReturn.isOk(), "Default 'ok' should be false");
        assertNull(webReturn.getMessage(), "Default 'message' should be null");
        assertNull(webReturn.getData(), "Default 'data' should be null");
    }

    /**
     * Tests the parameterized constructor of {@link WebReturn}.
     * Validates field assignments based on inputs.
     */
    @Test
    void testParameterizedConstructor() {
        String message = "Operation completed";
        Integer data = 42;

        WebReturn<Integer> webReturn = new WebReturn<>(true, message, data);

        assertTrue(webReturn.isOk(), "'ok' should be true");
        assertEquals(message, webReturn.getMessage(), "Message should match");
        assertEquals(data, webReturn.getData(), "Data should match");
    }

    /**
     * Tests the {@link WebReturn#success()} static method without data.
     */
    @Test
    void testSuccessStaticMethodNoData() {
        WebReturn<Object> webReturn = WebReturn.success();

        assertTrue(webReturn.isOk(), "'ok' should be true");
        assertNull(webReturn.getMessage(), "Message should be null");
        assertNull(webReturn.getData(), "Data should be null");
    }

    /**
     * Tests the {@link WebReturn#success(Object)} static method with data.
     */
    @Test
    void testSuccessStaticMethodWithData() {
        String data = "Success data";

        WebReturn<String> webReturn = WebReturn.success(data);

        assertTrue(webReturn.isOk(), "'ok' should be true");
        assertNull(webReturn.getMessage(), "Message should be null");
        assertEquals(data, webReturn.getData(), "Data should match");
    }

    /**
     * Tests the {@link WebReturn#success(String, Object)} static method
     * with data and a message.
     */
    @Test
    void testSuccessStaticMethodWithDataAndMessage() {
        String data = "Success data";
        String message = "Success message";

        WebReturn<String> webReturn = WebReturn.success(message, data);

        assertTrue(webReturn.isOk(), "'ok' should be true");
        assertEquals(data, webReturn.getData(), "Data should match");
        assertEquals(message, webReturn.getMessage(), "Message should match");
    }

    /**
     * Tests the {@link WebReturn#error(String)} static method with an error message.
     */
    @Test
    void testErrorStaticMethodWithMessage() {
        String errorMessage = "An error occurred";

        WebReturn<Object> webReturn = WebReturn.error(errorMessage);

        assertFalse(webReturn.isOk(), "'ok' should be false");
        assertEquals(errorMessage, webReturn.getMessage(), "Error message should match");
        assertNull(webReturn.getData(), "Data should be null");
    }

    /**
     * Tests the {@link WebReturn#error(String, Object)} static method
     * with an error message and associated data.
     */
    @Test
    void testErrorStaticMethodWithMessageAndData() {
        String errorMessage = "Invalid input";
        String data = "Error data";

        WebReturn<String> webReturn = WebReturn.error(errorMessage, data);

        assertFalse(webReturn.isOk(), "'ok' should be false");
        assertEquals(errorMessage, webReturn.getMessage(), "Error message should match");
        assertEquals(data, webReturn.getData(), "Data should match");
    }

    /**
     * Tests the {@link WebReturn#toString()} method to ensure
     * it returns the expected string representation.
     */
    @Test
    void testToStringMethod() {
        String message = "Test message";
        int data = 100;

        WebReturn<Integer> webReturn = new WebReturn<>(true, message, data);

        String expectedString = "WebReturn{ok=true, message='Test message', data=100}";
        assertEquals(expectedString, webReturn.toString(), "toString output should match");
    }
}
