package com.ase.utility;
import com.ase.exception.ServerErrorException;
import com.ase.exception.TCPClientException;
import com.ase.exception.TCPClientReadException;

import java.io.DataInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.*;

/**
 * Facilitates dynamic reading of various data types from a DataInputStream based on server-provided size information.
 */
public class DynamicDataReader {

    /**
     * Server information containing size limits for data types.
     */
    private final ServerInformation serverInfo;

    /**
     * Input stream to read data from the server.
     */
    private final DataInputStream dataInputStream;

    /**
     * Constructs a DynamicDataReader with the specified server information and data input stream.
     *
     * @param serverInfo      the server information containing size limits
     * @param dataInputStream the data input stream to read from
     */
    public DynamicDataReader(ServerInformation serverInfo, DataInputStream dataInputStream) {
        this.serverInfo = serverInfo;
        this.dataInputStream = dataInputStream;
    }

    /**
     * Reads a string from the input stream using the TCP_EUI encoding scheme.
     *
     * @return The decoded String.
     * @throws TCPClientReadException if an I/O error occurs or the encoding is invalid.
     */
    public String readString() throws TCPClientException {
        try {
            long length = readTCPEUI();

            if (length < 0 || length > this.serverInfo.getStringSize()) {
                throw new TCPClientReadException("Invalid string length: " + length);
            }

            byte[] bytes = new byte[(int) length];
            dataInputStream.readFully(bytes);

            return new String(bytes, StandardCharsets.UTF_8);
        } catch (IOException e) {
            throw new TCPClientReadException("Failed to read string.", e);
        }
    }

    /**
     * Decodes a TCP_EUI-encoded unsigned integer from the input stream.
     *
     * @return The decoded long value.
     * @throws TCPClientException if an I/O error occurs or the encoding is invalid.
     */
    public long readTCPEUI() throws TCPClientException {
        try {
            int firstByte = dataInputStream.readUnsignedByte();
            int leadingOnes = 0;

            while (leadingOnes < 8 && ((firstByte & (0x80 >> leadingOnes)) != 0)) {
                leadingOnes++;
            }

            if (leadingOnes == 0) {
                return firstByte & 0x7F;
            }

            if (leadingOnes > 8) {
                throw new IllegalArgumentException("Invalid TCP_EUI encoding: too many leading ones.");
            }

            long value = 0;
            value |= (long)(firstByte & (0xFF >> (leadingOnes + 1))) << (8 * leadingOnes);

            // Read continuation bytes and assemble the value
            for (int i = 0; i < leadingOnes; i++) {
                int nextByte = dataInputStream.readUnsignedByte();
                value |= ((long) nextByte) << (8 * (leadingOnes - 1 - i));
            }

            return value;
        } catch (IOException e) {
            throw new TCPClientReadException("Failed to read TCP_EUI encoded integer.", e);
        }
    }

    /**
     * Reads a boolean value based on the server-provided size.
     *
     * @return the boolean value read from the stream
     * @throws TCPClientReadException if reading the boolean fails
     */
    public boolean readBoolean() throws TCPClientException {
        try {
            byte[] bytes = new byte[serverInfo.getBooleanSize()];
            dataInputStream.readFully(bytes);
            return bytes[0] != 0;
        } catch (Exception e) {
            throw new TCPClientReadException("Failed to read boolean.", e);
        }
    }

    /**
     * Reads an integer value based on the server-provided size.
     *
     * @return the integer value read from the stream
     * @throws TCPClientReadException if reading the integer fails
     */
    public int readInt() throws TCPClientException {
        try {
            byte[] bytes = new byte[serverInfo.getIntSize()];
            dataInputStream.readFully(bytes);
            return bytesToInt(bytes);
        } catch (Exception e) {
            throw new TCPClientReadException("Failed to read int.", e);
        }
    }

    /**
     * Reads a character value based on the server-provided size.
     *
     * @return the character value read from the stream
     * @throws TCPClientReadException if reading the character fails
     */
    public char readChar() throws TCPClientException {
        try {
            byte[] bytes = new byte[serverInfo.getCharSize()];
            dataInputStream.readFully(bytes);
            return bytesToChar(bytes);
        } catch (Exception e) {
            throw new TCPClientReadException("Failed to read char.", e);
        }
    }

    /**
     * Reads a double value based on the server-provided size.
     *
     * @return the double value read from the stream
     * @throws TCPClientReadException if reading the double fails
     */
    public double readDouble() throws TCPClientException {
        try {
            byte[] bytes = new byte[serverInfo.getDoubleSize()];
            dataInputStream.readFully(bytes);
            return bytesToDouble(bytes);
        } catch (Exception e) {
            throw new TCPClientReadException("Failed to read double.", e);
        }
    }

    /**
     * Reads a semaphore value based on the server-provided size.
     * Assumes that the semaphore is represented as an integer identifier.
     *
     * @return the semaphore identifier read from the stream
     * @throws TCPClientReadException if reading the semaphore fails
     */
    public int readSemaphore() throws TCPClientException {
        try {
            byte[] bytes = new byte[serverInfo.getSemaphoreSize()];
            dataInputStream.readFully(bytes);
            return bytesToInt(bytes);
        } catch (Exception e) {
            throw new TCPClientReadException("Failed to read semaphore.", e);
        }
    }

    /**
     * Reads a Generic Server Error Package from the input stream.
     *
     * @throws ServerErrorException   when an error package is successfully read from the server.
     * @throws TCPClientException     if an error occurs while reading the server error string.
     * @throws IOException            if an I/O error occurs while reading the server error.
     */
    public void readServerError() throws ServerErrorException, TCPClientException, IOException {
        long errorCodeLong = dataInputStream.readUnsignedShort();

        int errorCode = (int) errorCodeLong;

        String errorMessage = readString();
        throw new ServerErrorException(errorCode, errorMessage);
    }

    /**
     * Converts a byte array to an integer value.
     * Assumes big-endian byte order.
     *
     * @param bytes the byte array to convert
     * @return the integer value represented by the byte array
     */
    private int bytesToInt(byte[] bytes) throws TCPClientException {
        final int TARGET_SIZE = Integer.BYTES;
        if (bytes.length > TARGET_SIZE) {
            throw new TCPClientReadException("Byte array too large to convert to int: " + bytes.length + " bytes");
        }

        ByteBuffer buffer = ByteBuffer.allocate(TARGET_SIZE);
        buffer.order(ByteOrder.BIG_ENDIAN);

        final byte padByte = (bytes.length > 0 && (bytes[0] & 0x80) != 0) ? (byte) 0xFF : 0x00;
        for (int i = 0; i < TARGET_SIZE - bytes.length; i++) {
            buffer.put(padByte);
        }

        buffer.put(bytes);
        buffer.flip();

        return buffer.getInt();
    }

    /**
     * Converts a byte array to a character value.
     * Assumes UTF-8 encoding.
     *
     * @param bytes the byte array to convert
     * @return the character represented by the byte array
     * @throws IllegalArgumentException if the byte array is null, empty, or does not represent a valid character
     */
    private char bytesToChar(byte[] bytes) throws TCPClientException {
        if (bytes == null || bytes.length == 0) {
            throw new TCPClientReadException("Byte array is null or empty.");
        }

        Charset charset = StandardCharsets.UTF_8;
        CharsetDecoder decoder = charset.newDecoder();

        decoder.onMalformedInput(CodingErrorAction.REPORT);
        decoder.onUnmappableCharacter(CodingErrorAction.REPORT);

        try {
            ByteBuffer byteBuffer = ByteBuffer.wrap(bytes);
            String decoded = decoder.decode(byteBuffer).toString();

            if (decoded.length() == 1) {
                return decoded.charAt(0);
            } else if (decoded.length() > 1) {
                throw new TCPClientReadException("Byte array represents multiple characters.");
            } else {
                throw new TCPClientReadException("Byte array does not represent a valid character.");
            }
        } catch (CharacterCodingException e) {
            throw new TCPClientReadException("Failed to decode byte array into a character.", e);
        }
    }

    /**
     * Converts a byte array to a double value.
     * Assumes big-endian byte order.
     *
     * @param bytes the byte array to convert
     * @return the double value represented by the byte array
     * @throws IllegalArgumentException if the byte array length is not 8
     */
    private double bytesToDouble(byte[] bytes) {
        if (bytes.length != 8 && bytes.length != 4)  {
            throw new IllegalArgumentException("Double must be 8 bytes.");
        }

        long bits = 0;

        for (byte b : bytes) {
            bits = (bits << 8) | (b & 0xFF);
        }

        if (bytes.length == 4) return Float.intBitsToFloat((int) bits);
        else return Double.longBitsToDouble(bits);
    }
}
