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
    public static final byte AK = (byte) 0xFF;

    /**
     * Creates a client information packet with version and flags.
     *
     * @param version The version number to be encoded in the packet.
     * @param flags   Flags associated with the client information.
     * @return A byte array representing the client information packet.
     */
    public static byte[] createClientInfoPacket(int version, byte flags) {
        byte[] ret = new byte[3];

        ret[0] = (byte)(version & 0xFF);
        ret[1] = (byte)((version >> 8) & 0xFF);
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
            outputStream.write(username.getBytes(StandardCharsets.UTF_8));
            outputStream.write(passwordLength);
            outputStream.write(password.getBytes(StandardCharsets.UTF_8));

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
}