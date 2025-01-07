package com.ase.utility;

import com.ase.exception.HandshakeException;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/**
 * Utility class providing methods for creating various byte arrays for communication.
 */
public class Utility {
    /** Negative Acknowledgement sent by server (NAK). */
    public static final byte NAK = 0x00;

    /** Acknowledgement sent by server (AK). */
    public static final byte ACK = (byte) 0xFF;

    // --- START DATA COMMANDS ---
    /** Command to get a key. */
    public static final byte GET_COMMAND = (byte) 0x01;

    /** Command to set a key. */
    public static final byte SET_COMMAND = (byte) 0x02;

    /** Command to force set a key. */
    public static final byte FORCED_SET_COMMAND = (byte) 0x03;

    /** Command to delete a key. */
    public static final byte DELETE_COMMAND = (byte) 0x04;

    /** Command to list a key. */
    public static final byte LIST_COMMAND = (byte) 0x05;

    /*+ Command to follow a reference. */
    public static final byte FOLLOW_COMMAND = (byte) 0x06;

    /** Command to force follow a reference. */
    public static final byte FORCE_FOLLOW_COMMAND = (byte) 0x07;
    // --- END DATA COMMANDS ---


    // --- START USER  COMMANDS ---
    /** Command to create a user. */
    public static final byte CREATE_USER_COMMAND = (byte) 0x80;

    /** Command to update scope. */
    public static final byte UPDATE_SCOPE_COMMAND = (byte) 0x81;

    /** Command to delete a user. */
    public static final byte DELETE_USER_COMMAND = (byte) 0x82;

    /** Command to change a user's password. */
    public static final byte CHANGE_PASSWORD_COMMAND = (byte) 0x83;
    // --- END USER COMMANDS ---


    // --- START SERVER COMMANDS ---
    /** Command to stop operation. */
    public static final byte STOP_COMMAND = (byte) 0xFE;

    /** Command to ping the server. */
    public static final byte PING_COMMAND = (byte) 0xFD;

    /** Command to disconnect from the server. */
    public static final byte DISCONNECT_COMMAND = (byte) 0xFC;
    // --- END SERVER COMMANDS ---

    // --- START CONSTANTS ---
    public static final int MAX_BYTES_TO_READ = 1024;
    /**
     * Creates a client information packet with version and flags.
     *
     * @param version The version number to be encoded in the packet.
     * @param flags   Flags associated with the client information.
     * @return A byte array representing the client information packet.
     */
    public static byte[] createClientInfoPacket(int version, byte flags) {
        byte[] ret = new byte[3];

        ret[0] = (byte)((version >> 8) & 0xFF);
        ret[1] = (byte)(version & 0xFF);
        ret[2] = flags;

        return ret;
    }

    /**
     * Creates a user information packet containing encoded username and password.
     *
     * @param username The username to be encoded.
     * @param password The password to be encoded.
     * @return A byte array representing the user information packet.
     * @throws HandshakeException if there is an error creating the packet.
     */
    public static byte[] createUserInfoPacket(String username, String password) throws HandshakeException {
        try (ByteArrayOutputStream outputStream = new ByteArrayOutputStream()) {
            byte[] usernameLength = createTCPEUIByteArray(username.length());
            byte[] passwordLength = createTCPEUIByteArray(password.length());

            outputStream.write(usernameLength);
            outputStream.write(username.getBytes(StandardCharsets.ISO_8859_1));
            outputStream.write(passwordLength);
            outputStream.write(password.getBytes(StandardCharsets.ISO_8859_1));

            return outputStream.toByteArray();
        } catch (IOException e) {
            throw new HandshakeException("Error creating user info packet");
        }
    }

    /**
     * Creates a byte array representing a variable-length encoded integer
     * using the TCP_EUI encoding method.
     *
     * @param value The non-negative long value to be encoded.
     * @return A byte array representing the encoded value.
     * @throws IllegalArgumentException if the value is negative or too large to encode.
     */
    public static byte[] createTCPEUIByteArray(long value) {
        if (value < 0) {
            throw new IllegalArgumentException("Cannot encode negative values");
        }

        if (value == 0) {
            return new byte[] {0x00};
        }

        int bytesNeeded = 1;
        long tempValue = value;
        while (tempValue > (1L << (7 * bytesNeeded)) - 1) {
            bytesNeeded++;
            if (bytesNeeded > 9) {
                throw new IllegalArgumentException("Value too large to encode: " + value);
            }
        }

        byte[] result = new byte[bytesNeeded];

        for (int i = bytesNeeded - 1; i >= 0; i--) {
            result[i] = (byte)(tempValue & 0xFF);
            tempValue >>= 8;
        }

        if (bytesNeeded == 1 && value <= 0x7F) {
            return result;
        }

        byte header = (byte)(((1 << (bytesNeeded - 1)) - 1) << (8 - (bytesNeeded - 1)));
        result[0] = (byte) (header | (result[0] & (0xFF >> (bytesNeeded - 1))));

        return result;
    }

    /**
     * Decodes a byte array representing a variable-length encoded positive integer
     * using the TCP_EUI encoding method.
     *
     * @param encodedBytes The byte array to decode.
     * @return The decoded long value.
     * @throws IllegalArgumentException if the byte array is invalid or exceeds 9 bytes.
     */
    public static long decodeTCPEUIByteArray(byte[] encodedBytes) {
        if (encodedBytes == null || encodedBytes.length == 0) {
            throw new IllegalArgumentException("Encoded byte array is null or empty.");
        }

        int leadingOnes = 0;
        byte firstByte = encodedBytes[0];
        while (leadingOnes < 8 && ((firstByte & (0x80 >> leadingOnes)) != 0)) {
            leadingOnes++;
        }

        if (leadingOnes == 0) {
            return firstByte & 0x7F;
        }

        if (leadingOnes > 8 || encodedBytes.length != leadingOnes + 1) {
            throw new IllegalArgumentException("Invalid encoding.");
        }

        long value = 0;
        value |= (long)(firstByte & (0xFF >> (leadingOnes + 1))) << (8 * leadingOnes);

        for (int i = 1; i < encodedBytes.length; i++) {
            value |= (long)(encodedBytes[i] & 0xFF) << (8 * (encodedBytes.length - i -1));
        }

        return value;
    }

    /**
     * Retrieves the minimum value that can be represented by a signed integer of the specified size.
     *
     * @param targetSize The size of the integer in bytes.
     * @return The minimum value that can be represented by a signed integer of the specified size.
     */
    public static long getMinValue(int targetSize) {
        return -(1L << (8 * targetSize - 1));
    }

    /**
     * Retrieves the maximum value that can be represented by a signed integer of the specified size.
     *
     * @param targetSize The size of the integer in bytes.
     * @return The maximum value that can be represented by a signed integer of the specified size.
     */
    public static long getMaxValue(int targetSize) {
        return (1L << (8 * targetSize - 1)) - 1;
    }
}