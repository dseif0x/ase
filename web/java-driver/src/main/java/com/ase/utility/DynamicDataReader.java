package com.ase.utility;
import com.ase.exception.ServerErrorException;
import com.ase.exception.TCPClientException;
import com.ase.exception.TCPClientReadException;
import com.ase.utility.tcp.SimpleType;
import com.ase.utility.tcp.TypeDefinition;
import com.ase.utility.tcp.TypeFlag;
import org.apache.commons.lang3.tuple.Pair;

import java.io.DataInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Optional;

/**
 * Dynamic reading of various data types from a DataInputStream based on server-provided size information.
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
     * @throws TCPClientReadException if an I/O error occurs or the encoding is invalid.
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
            int size = serverInfo.getDoubleSize();

            byte[] bytes = new byte[size];
            dataInputStream.readFully(bytes);

            if (size == Double.BYTES) {
                return bytesToDouble(bytes);
            } else {
                return bytesToFloat(bytes);
            }
        } catch (IOException e) {
            throw new TCPClientReadException("Failed to read double.", e);
        }
    }

    /**
     * Reads a 4-byte unsigned integer from the input stream in big-endian order.
     *
     * @return The unsigned integer value as a long (0 to 4294967295).
     * @throws IOException If an I/O error occurs or if the end of the stream is reached prematurely.
     */
    public long readUnsignedInt() throws IOException {
        byte[] bytes = new byte[4];
        dataInputStream.readFully(bytes);

        ByteBuffer buffer = ByteBuffer.wrap(bytes);
        buffer.order(ByteOrder.BIG_ENDIAN);
        return buffer.getInt() & 0xFFFFFFFFL;
    }

    /**
     * Reads a key from the input stream based on the Type-Definition-Block.
     *
     * @return The Key-Value HashMap.
     * @throws TCPClientReadException if an I/O error occurs or the reading fails.
     */
    public HashMap<String, Object> readKey() throws TCPClientException {
        long keyLength = readTCPEUI();
        int len = (int) keyLength;

        HashMap<String, Object> key = new HashMap<>(len);

        for (int i = 0; i < len; i++) {
            String keyE = readString();
            Object valueE = readGeneral();
            key.put(keyE, valueE);
        }
        return key;
    }

    /**
     * Reads an array from the input stream based on the Type-Definition-Block.
     *
     * @return A List containing the array elements.
     * @throws TCPClientReadException if an I/O error occurs or the encoding is invalid.
     */
    public List<Object> readArray(boolean isComplex, SimpleType simpleType) throws TCPClientException {
        long arrayLength = readTCPEUI();
        if (arrayLength < 0 || arrayLength > serverInfo.getArraySize()) {
            throw new TCPClientReadException("Invalid array length: " + arrayLength);
        }

        int len = (int) arrayLength;

        List<Object> array = new ArrayList<>(len);

        if (!isComplex) {
            for (int i = 0; i < len; i++) {
                Object element = readSimpleType(simpleType);
                array.add(element);
            }
        } else {
            for (int i = 0; i < len; i++) {
                Object element = readGeneral();
                array.add(element);
            }
        }

        return array;
    }

    /**
     * Reads a value from the input stream based on its Type-Definition-Block.
     *
     * @return The decoded value as an Object.
     * @throws TCPClientReadException if an I/O error occurs or the encoding is invalid.
     */
    public Object readGeneral() throws TCPClientException {
        TypeDefinition typeDef = readTypeDefinition();

        if (typeDef.isKey()) {
            return readKey();
        } else if (typeDef.isArray()) {
            return readArray(typeDef.isComplex(), typeDef.getSimpleType());
        } else if (typeDef.isPair()) {
            return readPair(typeDef.isComplex(), typeDef.getSimpleType());
        } else {
            if (typeDef.getSimpleType() == null) {
                throw new TCPClientReadException("SimpleType is null for a non-complex, non-array, non-pair type.");
            }
            return readSimpleType(typeDef.getSimpleType());
        }
    }

    /**
     * Reads a Type-Definition-Block from the input stream.
     *
     * @return A TypeDefinition object encapsulating the type properties.
     * @throws TCPClientReadException if an I/O error occurs or the encoding is invalid.
     */
    private TypeDefinition readTypeDefinition() throws TCPClientReadException {
        try {
            int typeByte = dataInputStream.readUnsignedByte();

            boolean isArray = (typeByte & TypeFlag.ARRAY.getBitmask()) != 0;
            boolean isPair = (typeByte & TypeFlag.PAIR.getBitmask()) != 0;
            boolean isComplex = (typeByte & TypeFlag.COMPLEX.getBitmask()) != 0 && (isPair || isArray);
            boolean isKey = (typeByte & TypeFlag.KEY.getBitmask()) != 0;

            if (isArray && isPair) {
                throw new TCPClientReadException("Type-Definition-Block cannot have both Array and Pair flags set.");
            }

            SimpleType simpleType = SimpleType.fromTypeId(typeByte & 0x07);
            return new TypeDefinition(isArray, isPair, isComplex, isKey, simpleType);
        } catch (IOException e) {
            throw new TCPClientReadException("Failed to read Type-Definition-Block.", e);
        } catch (IllegalArgumentException e) {
            throw new TCPClientReadException("Unknown SimpleType in Type-Definition-Block.", e);
        }
    }

    /**
     * Reads a pair (key-value pair) from the input stream based on the Type-Definition-Block.
     *
     * @param isComplex Indicates whether the first value is a complex type requiring its own TDB.
     * @return A Pair object containing the key and value.
     * @throws TCPClientReadException if an I/O error occurs or the encoding is invalid.
     */
    private Pair<Object, Object> readPair(boolean isComplex, SimpleType type) throws TCPClientException {
        Object first, second;

        if (isComplex) first = readGeneral();
        else first = readSimpleType(type);

        second = readGeneral();
        return Pair.of(first, second);
    }

    /**
     * Reads a simple type value from the input stream.
     *
     * @param simpleType The SimpleType to read.
     * @return The decoded value as an Object.
     * @throws TCPClientReadException if an I/O error occurs or the encoding is invalid.
     */
    private Object readSimpleType(SimpleType simpleType) throws TCPClientException {
        switch (simpleType) {
            case BOOLEAN:
                return readBoolean();
            case INT:
                return readInt();
            case STRING:
                return readString();
            case DOUBLE:
                return readDouble();
            default:
                throw new TCPClientReadException("Unsupported SimpleType: " + simpleType);
        }
    }

    /**
     * Reads a Generic Server Error Package from the input stream.
     *
     * @param  throwException whether to throw an exception if a server error is read
     * @return an optional pair containing the error message and error code
     *
     * @throws ServerErrorException   when an error package is successfully read from the server.
     * @throws TCPClientException     if an error occurs while reading the server error string.
     * @throws IOException            if an I/O error occurs while reading the server error.
     */
    public Optional<Pair<String, Integer>> readServerError(boolean throwException) throws ServerErrorException, TCPClientException, IOException {
        long errorCodeLong = dataInputStream.readUnsignedShort();

        int errorCode = (int) errorCodeLong;

        String errorMessage = readString();
        if (throwException) throw new ServerErrorException(errorCode, errorMessage);
        return Optional.of(Pair.of(errorMessage, errorCode));
    }

    /**
     * Converts a byte array to an integer value.
     * Assumes big-endian byte order.
     *
     * @param bytes the byte array to convert
     * @return the integer value represented by the byte array
     */
    private int bytesToInt(byte[] bytes) throws TCPClientException {
        final int targetSize = Integer.BYTES;
        if (bytes.length > targetSize) {
            throw new TCPClientReadException("Byte array too large to convert to int: " + bytes.length + " bytes");
        }

        ByteBuffer buffer = ByteBuffer.allocate(targetSize);
        buffer.order(ByteOrder.BIG_ENDIAN);

        final byte padByte = (bytes.length > 0 && (bytes[0] & 0x80) != 0) ? (byte) 0xFF : 0x00;
        for (int i = 0; i < targetSize - bytes.length; i++) {
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
     */
    private char bytesToChar(byte[] bytes) {
        return new String(bytes, StandardCharsets.UTF_8).charAt(0);
    }

    /**
     * Converts an 8-byte array to a double value using ByteBuffer.
     * Assumes big-endian byte order.
     *
     * @param bytes the byte array to convert (must be 8 bytes)
     * @return the double value represented by the byte array
     * @throws IllegalArgumentException if the byte array length is not 8
     */
    private double bytesToDouble(byte[] bytes) {
        if (bytes.length != 8)  {
            throw new IllegalArgumentException("Double must be 8 bytes.");
        }

        ByteBuffer buffer = ByteBuffer.wrap(bytes);
        buffer.order(ByteOrder.BIG_ENDIAN);
        return buffer.getDouble();
    }

    /**
     * Converts a 4-byte array to a float value.
     * Assumes big-endian byte order.
     *
     * @param bytes the byte array to convert (must be 4 bytes)
     * @return the float value represented by the byte array
     * @throws IllegalArgumentException if the byte array length is not 4
     */
    private float bytesToFloat(byte[] bytes) {
        if (bytes.length != 4)  {
            throw new IllegalArgumentException("Float must be 4 bytes.");
        }

        int bits = 0;

        for (byte b : bytes) {
            bits = (bits << 8) | (b & 0xFF);
        }

        return Float.intBitsToFloat(bits);
    }
}
