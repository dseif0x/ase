package com.ase.utility;

import org.junit.jupiter.api.Test;

import java.nio.charset.StandardCharsets;

import static org.junit.jupiter.api.Assertions.*;

class UtilityTest {

    @Test
    void testCreateClientInfoPacket() {
        int version = 0x1234;
        byte flags = 0x5A;

        byte[] expected = new byte[]{
            (byte) ((version >> 8) & 0xFF),
            (byte) (version & 0xFF),
            flags
        };

        byte[] actual = Utility.createClientInfoPacket(version, flags);

        assertArrayEquals(expected, actual, "Client info packet does not match expected output.");
    }

    @Test
    void testEncodeDecodeTCPEUIByteArray() {
        // Known encoded values
        byte[] encodedZero = new byte[]{0x00};
        long decodedZero = Utility.decodeTCPEUIByteArray(encodedZero);
        assertEquals(0L, decodedZero, "Decoded zero does not match expected.");

        byte[] encoded127 = new byte[]{0x7F};
        long decoded127 = Utility.decodeTCPEUIByteArray(encoded127);
        assertEquals(127L, decoded127, "Decoded 127 does not match expected.");

        // Encoding and decoding a value
        long originalValue = 987654321L;
        byte[] encoded = Utility.createTCPEUIByteArray(originalValue);
        long decoded = Utility.decodeTCPEUIByteArray(encoded);
        assertEquals(originalValue, decoded, "Decoded value does not match original.");

        // Maximum value
        long valueMax = (1L << 63) - 1;
        byte[] encodedMax = Utility.createTCPEUIByteArray(valueMax);
        long decodedMax = Utility.decodeTCPEUIByteArray(encodedMax);
        assertEquals(valueMax, decodedMax, "Decoded maximum value does not match original.");

        long negativeValue = -1;
        assertThrows(IllegalArgumentException.class, () -> Utility.createTCPEUIByteArray(negativeValue), "Expected exception for negative value.");

        long zeroValue = 0;
        byte[] encodedZeroValue = Utility.createTCPEUIByteArray(zeroValue);
        long decodedZeroValue = Utility.decodeTCPEUIByteArray(encodedZeroValue);
        assertEquals(zeroValue, decodedZeroValue, "Decoded zero value does not match original.");
    }

    @Test
    void testCreateUserInfoPacketLongStrings() {
        String username = "user";
        String password = "pass";

        byte[] usernameBytes = username.getBytes(StandardCharsets.ISO_8859_1);
        byte[] passwordBytes = password.getBytes(StandardCharsets.ISO_8859_1);

        byte[] usernameLength = Utility.createTCPEUIByteArray(usernameBytes.length);
        byte[] passwordLength = Utility.createTCPEUIByteArray(passwordBytes.length);

        byte[] expected = new byte[usernameLength.length + usernameBytes.length + passwordLength.length + passwordBytes.length];
        int pos = 0;
        System.arraycopy(usernameLength, 0, expected, pos, usernameLength.length);
        pos += usernameLength.length;
        System.arraycopy(usernameBytes, 0, expected, pos, usernameBytes.length);
        pos += usernameBytes.length;
        System.arraycopy(passwordLength, 0, expected, pos, passwordLength.length);
        pos += passwordLength.length;
        System.arraycopy(passwordBytes, 0, expected, pos, passwordBytes.length);

        assertDoesNotThrow(() -> {
            byte[] actual = Utility.createUserInfoPacket(username, password);
            assertArrayEquals(expected, actual, "User info packet with does not match expected output.");
        }, "Creating user info packet with short strings should not throw an exception.");
    }

    @Test
    void testGetMinValue() {
        // 1-byte integer
        int targetSize1 = 1;
        long expectedMin1 = -(1L << (8 * targetSize1 - 1));
        long actualMin1 = Utility.getMinValue(targetSize1);
        assertEquals(expectedMin1, actualMin1, "Minimum value for 1-byte integer does not match.");

        // 2-byte integer
        int targetSize2 = 2;
        long expectedMin2 = -(1L << (8 * targetSize2 - 1));
        long actualMin2 = Utility.getMinValue(targetSize2);
        assertEquals(expectedMin2, actualMin2, "Minimum value for 2-byte integer does not match.");

        // 8-byte integer
        int targetSize8 = 8;
        long expectedMin8 = Long.MIN_VALUE;
        long actualMin8 = Utility.getMinValue(targetSize8);
        assertEquals(expectedMin8, actualMin8, "Minimum value for 8-byte integer does not match.");
    }

    @Test
    void testGetMaxValue() {
        // 1-byte integer
        int targetSize1 = 1;
        long expectedMax1 = (1L << (8 * targetSize1 - 1)) - 1;
        long actualMax1 = Utility.getMaxValue(targetSize1);
        assertEquals(expectedMax1, actualMax1, "Maximum value for 1-byte integer does not match.");

        // 2-byte integer
        int targetSize2 = 2;
        long expectedMax2 = (1L << (8 * targetSize2 - 1)) - 1;
        long actualMax2 = Utility.getMaxValue(targetSize2);
        assertEquals(expectedMax2, actualMax2, "Maximum value for 2-byte integer does not match.");

        // 8-byte integer
        int targetSize8 = 8;
        long expectedMax8 = Long.MAX_VALUE;
        long actualMax8 = Utility.getMaxValue(targetSize8);
        assertEquals(expectedMax8, actualMax8, "Maximum value for 8-byte integer does not match.");
    }}