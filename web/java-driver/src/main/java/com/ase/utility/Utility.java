package com.ase.utility;


import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public class Utility {
    public static final byte NAK = 0x00;
    public static final byte AK = (byte) 0xFF;

    public static byte[] createClientInfoPacket(int version, byte flags) {
        byte[] ret = new byte[3];

        ret[0] = (byte)(version & 0xFF);
        ret[1] = (byte)((version >> 8) & 0xFF);
        ret[2] = flags;

        return ret;
    }

    public static byte[] createUserInfoPacket(String username, String password) {
        try (ByteArrayOutputStream outputStream = new ByteArrayOutputStream()) {
            byte[] usernameLength = createTCPEUIByteArray(username.length());
            byte[] passwordLength = createTCPEUIByteArray(password.length());

            outputStream.write(usernameLength);
            outputStream.write(username.getBytes(StandardCharsets.UTF_8));
            outputStream.write(passwordLength);
            outputStream.write(password.getBytes(StandardCharsets.UTF_8));

            return outputStream.toByteArray();
        } catch (IOException e) {
            throw new RuntimeException("Error creating user info packet", e);
        }
    }

    public static byte[] createTCPEUIByteArray(long value) {
        if (value < 0) {
            throw new IllegalArgumentException("Cannot encode negative values");
        }

        int bytesNeeded = 0;
        long tempValue = value;
        while (tempValue > 0) {
            tempValue >>= 7;
            bytesNeeded++;
        }

        if (bytesNeeded == 0) {
            bytesNeeded = 1;
        }

        if (bytesNeeded > 8) {
            throw new IllegalArgumentException("Value too large to encode: " + value);
        }

        byte[] result = new byte[bytesNeeded];

        tempValue = value;
        for (int i = bytesNeeded - 1; i >= 0; i--) {
            result[i] = (byte)(tempValue & 0xFF);
            tempValue >>= 8;
        }

        if (bytesNeeded == 1 && value <= 0x7F) {
            return result;
        }


        byte header = (byte)(((1 << (bytesNeeded - 1)) - 1) << (8 - (bytesNeeded - 1)));
        header |= result[0];

        byte[] withHeader = new byte[bytesNeeded];
        withHeader[0] = header;
        System.arraycopy(result, 1, withHeader, 1, bytesNeeded - 1);

        return withHeader;
    }


}

