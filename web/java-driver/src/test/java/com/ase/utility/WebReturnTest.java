package com.ase.utility;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class WebReturnTest {

    @Test
    void testDefaultConstructor() {
        WebReturn<Object> webReturn = new WebReturn<>();

        assertFalse(webReturn.isOk(), "Default 'ok' should be false");
        assertNull(webReturn.getMessage(), "Default 'message' should be null");
        assertNull(webReturn.getData(), "Default 'data' should be null");
    }

    @Test
    void testParameterizedConstructor() {
        String message = "Operation completed";
        Integer data = 42;

        WebReturn<Integer> webReturn = new WebReturn<>(true, message, data);

        assertTrue(webReturn.isOk(), "'ok' should be true");
        assertEquals(message, webReturn.getMessage(), "Message should match");
        assertEquals(data, webReturn.getData(), "Data should match");
    }

    @Test
    void testSuccessStaticMethodNoData() {
        WebReturn<Object> webReturn = WebReturn.success();

        assertTrue(webReturn.isOk(), "'ok' should be true");
        assertNull(webReturn.getMessage(), "Message should be null");
        assertNull(webReturn.getData(), "Data should be null");
    }

    @Test
    void testSuccessStaticMethodWithData() {
        String data = "Success data";

        WebReturn<String> webReturn = WebReturn.success(data);

        assertTrue(webReturn.isOk(), "'ok' should be true");
        assertNull(webReturn.getMessage(), "Message should be null");
        assertEquals(data, webReturn.getData(), "Data should match");
    }

    @Test
    void testSuccessStaticMethodWithDataAndMessage() {
        String data = "Success data";
        String message = "Success message";

        WebReturn<String> webReturn = WebReturn.success(message, data);

        assertTrue(webReturn.isOk(), "'ok' should be true");
        assertEquals(data, webReturn.getData(), "Data should match");
        assertEquals(message, webReturn.getMessage(), "Message should match");
    }

    @Test
    void testErrorStaticMethodWithMessage() {
        String errorMessage = "An error occurred";

        WebReturn<Object> webReturn = WebReturn.error(errorMessage);

        assertFalse(webReturn.isOk(), "'ok' should be false");
        assertEquals(errorMessage, webReturn.getMessage(), "Error message should match");
        assertNull(webReturn.getData(), "Data should be null");
    }

    @Test
    void testErrorStaticMethodWithMessageAndData() {
        String errorMessage = "Invalid input";
        String data = "Error data";

        WebReturn<String> webReturn = WebReturn.error(errorMessage, data);

        assertFalse(webReturn.isOk(), "'ok' should be false");
        assertEquals(errorMessage, webReturn.getMessage(), "Error message should match");
        assertEquals(data, webReturn.getData(), "Data should match");
    }

    @Test
    void testToStringMethod() {
        String message = "Test message";
        int data = 100;

        WebReturn<Integer> webReturn = new WebReturn<>(true, message, data);

        String expectedString = "WebReturn{ok=true, message='Test message', data=100}";
        assertEquals(expectedString, webReturn.toString(), "toString output should match");
    }
}
