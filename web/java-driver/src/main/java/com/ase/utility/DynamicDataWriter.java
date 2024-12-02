package com.ase.utility;

import com.ase.exception.TCPClientException;
import com.ase.exception.TCPClientReadException;
import com.ase.exception.TCPClientWriteException;
import com.ase.utility.tcp.SimpleType;
import com.ase.utility.tcp.TcpGeneral;
import com.ase.utility.tcp.TcpGeneralType;
import com.ase.utility.tcp.TypeFlag;
import org.apache.commons.lang3.tuple.Pair;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

/**
 * Dynamic reading of various data types from a DataInputStream based on server-provided size information.
 */
public class DynamicDataWriter {
    /** Output stream to write data to the server. */
    private final DataOutputStream dataOutputStream;

    /**
     * Server information containing size limits for data types.
     */
    private final ServerInformation serverInfo;

    /**
     * Constructs a DynamicDataWriter with the specified data output stream.
     *
     * @param outputStream The output stream to write to.
     */
    public DynamicDataWriter(ServerInformation serverInformation, OutputStream outputStream) {
        this.serverInfo = serverInformation;
        this.dataOutputStream = new DataOutputStream(outputStream);
    }

    /**
     * Writes a string to the output stream using the TCP_EUI encoding scheme.
     *
     * @param string The string to write.
     * @throws TCPClientWriteException If an I/O error occurs or the string length exceeds the server limit.
     */
    public void writeString(String string) throws TCPClientWriteException {
        try {
            byte[] bytes = string.getBytes(StandardCharsets.UTF_8);

            if (bytes.length > serverInfo.getStringSize()) {
                throw new TCPClientWriteException("String length exceeds server limit.");
            }

            byte[] tcpEUI = Utility.createTCPEUIByteArray(string.length());

            dataOutputStream.write(tcpEUI);
            dataOutputStream.write(bytes);

        } catch (IOException e) {
            throw new TCPClientWriteException("I/O error occurred when writing string.", e);
        }
    }

    /**
     * Writes a boolean value based on the server-provided size.
     *
     * @param value the boolean value to write to the stream
     * @throws TCPClientWriteException if writing the boolean fails
     */
    private void writeBoolean(boolean value) throws TCPClientException {
        try {
            byte[] bytes = new byte[serverInfo.getBooleanSize()];
            bytes[0] = (byte) (value ? 1 : 0);
            dataOutputStream.write(bytes);
        } catch (IOException e) {
            throw new TCPClientWriteException("I/O error occurred when writing boolean.", e);
        }
    }

    /**
     * Writes an integer value based on the server-provided size.
     *
     * @param value the integer value to write to the stream
     * @throws TCPClientWriteException if writing the integer fails, or value is out of range
     */
    private void writeInt(int value) throws TCPClientException {
        try {
            int targetSize = serverInfo.getIntSize();
            final int maxSize = Integer.BYTES;

            long minValue = Utility.getMinValue(targetSize);
            long maxValue = Utility.getMaxValue(targetSize);

            if (value < minValue || value > maxValue) {
                throw new TCPClientWriteException("Integer value " + value + " out of range for " + targetSize + " bytes. Allowed range: " + minValue + " to " + maxValue + ".");
            }

            // Convert the integer to a 4-byte array in big-endian order
            byte[] fullBytes = ByteBuffer.allocate(maxSize)
                .order(ByteOrder.BIG_ENDIAN)
                .putInt(value)
                .array();

            byte[] bytesToWrite;

            if (targetSize < maxSize) {
                bytesToWrite = Arrays.copyOfRange(fullBytes, maxSize - targetSize, maxSize);
            } else {
                bytesToWrite = fullBytes;
            }

            // Write the bytes to the output stream
            dataOutputStream.write(bytesToWrite);
        } catch (Exception e) {
            throw new TCPClientWriteException("Failed to write int.", e);
        }
    }

    /**
     * Writes a double value based on the server-provided size.
     *
     * @param value the double value to write to the stream
     * @throws TCPClientWriteException if writing the double fails or if the value exceeds the server's byte size
     */
    private void writeDouble(double value) throws TCPClientException {
        try {
            int targetSize = serverInfo.getDoubleSize();
            final int maxSize = Double.BYTES;


            byte[] bytesToWrite;

            if (targetSize == maxSize) {
                bytesToWrite = doubleToBytes(value);
            } else {
                if (value < Float.MIN_VALUE || value > Float.MAX_VALUE) {
                    throw new TCPClientWriteException("Double value " + value + " out of range for 4 bytes.");
                }
                float floatValue = (float) value;
                bytesToWrite = floatToBytes(floatValue);
            }

            dataOutputStream.write(bytesToWrite);
            dataOutputStream.flush();
        } catch (IOException e) {
            throw new TCPClientWriteException("Failed to write double.", e);
        }
    }

    /**
     * Writes a 4-byte unsigned integer to the output stream in big-endian order.
     *
     * @param value The unsigned integer value to write. Should be between 0 and 0xFFFFFFFF (4294967295).
     * @throws IOException If an I/O error occurs.
     * @throws IllegalArgumentException If the value is out of the unsigned int range.
     */
    public void writeUnsignedInt(long value) throws IOException {
        if (value < 0 || value > 0xFFFFFFFFL) {
            throw new IllegalArgumentException("Value out of range for unsigned int: " + value);
        }

        ByteBuffer buffer = ByteBuffer.allocate(4);
        buffer.order(ByteOrder.BIG_ENDIAN);
        buffer.putInt((int) value); // Only the lower 32 bits are used
        dataOutputStream.write(buffer.array());
    }

    /**
     * Serializes and wirte a Key-Value map of type {@link TcpGeneral} to the output stream.
     *
     * @throws TCPClientReadException if an I/O error occurs or the Key-Value Map length is invalid.
     * @throws IllegalArgumentException if the provided {@link TcpGeneral} is not of type {@link TcpGeneralType#KEY_MAP}.
     */
    private void writeKeyMap(TcpGeneral value) throws TCPClientException {
        if (value.getType() != TcpGeneralType.KEY_MAP) {
            throw new IllegalArgumentException("Expected KEY_MAP TcpGeneral.");
        }

        try {
            HashMap<String, TcpGeneral> keyMap = value.getKeyMap();
            int len = keyMap.size();
            if (len == 0) {
                throw new TCPClientWriteException("KeyMap length invalid.");
            }

            byte[] lengthBytes = Utility.createTCPEUIByteArray(len);
            dataOutputStream.write(lengthBytes);

            for (String key : keyMap.keySet()) {
                writeString(key);
                writeGeneral(keyMap.get(key));
            }
        } catch (IOException e) {
            throw new TCPClientWriteException("Failed to write key map.", e);
        }
    }

    /**
     * Serializes and writes a pair of {@link TcpGeneral} objects to the output stream.
     *
     * @param value The {@link TcpGeneral} object representing the pair to serialize.
     * @throws TCPClientException If an I/O error occurs or the pair is invalid.
     * @throws IllegalArgumentException If the provided {@link TcpGeneral} is not of type {@link TcpGeneralType#PAIR}.
     */
    private void writePair(TcpGeneral value) throws TCPClientException {
        if (value.getType() != TcpGeneralType.PAIR) {
            throw new IllegalArgumentException("Expected PAIR TcpGeneral.");
        }

        try {
            Pair<TcpGeneral, TcpGeneral> pair = value.getPair();
            writeGeneral(pair.getLeft());
            writeGeneral(pair.getRight());
        } catch (TCPClientException e) {
            throw new TCPClientWriteException("Failed to write pair.", e);
        }
    }

    /**
     * Serializes and writes an array of {@link TcpGeneral} objects to the output stream.
     *
     * @param value The {@link TcpGeneral} object representing the array to serialize.
     * @throws TCPClientException If an I/O error occurs or the array length is invalid.
     * @throws IllegalArgumentException If the provided {@link TcpGeneral} is not of type {@link com.ase.utility.tcp.TcpGeneralType#ARRAY}.
     */
    private void writeArray(TcpGeneral value) throws TCPClientException {
        if (value.getType() != TcpGeneralType.ARRAY) {
            throw new IllegalArgumentException("Expected ARRAY TcpGeneral.");
        }

        try {
            List<TcpGeneral> array = value.getArray();
            int len = array.size();
            if (len == 0 || len > serverInfo.getArraySize()) {
                throw new TCPClientWriteException("Array length invalid.");
            }

            byte[] lengthBytes = Utility.createTCPEUIByteArray(len);
            dataOutputStream.write(lengthBytes);


            if (value.isComplex()) {
                for (TcpGeneral element : array) {
                    writeGeneral(element);
                }
            } else {
                SimpleType elementType = value.getElementType();
                for (TcpGeneral element : array) {
                    writeSimpleType(elementType, element.getValue());
                }
            }
        } catch (IOException e) {
            throw new TCPClientWriteException("Failed to write array.", e);
        }
    }

    /**
     * Serializes and writes a {@link TcpGeneral} object to the output stream, including its Type Definition Block.
     *
     * @param value The {@link TcpGeneral} object to serialize and write.
     * @throws TCPClientException if an I/O error occurs or the encoding is invalid.
     */
    public void writeGeneral(TcpGeneral value) throws TCPClientException {
        writeTypeDefinition(value);

        switch (value.getType()) {
            case SIMPLE:
                writeSimpleType(value.getSimpleType(), value.getValue());
                break;
            case ARRAY:
                writeArray(value);
                break;
            case PAIR:
                writePair(value);
                break;
            case KEY_MAP:
                writeKeyMap(value);
                break;
        }
    }

    /**
     * Writes the Type Definition Block (TDB) byte to the output stream based on the TcpGeneral's type.
     *
     * @param value The TcpGeneral object whose type definition is to be written.
     * @throws TCPClientException If an I/O error occurs or getting the SimpelType failed.
     */
    public void writeTypeDefinition(TcpGeneral value) throws TCPClientException {
        try {
            SimpleType type = SimpleType.UNKNOWN;
            int flags = 0;

            switch (value.getType()) {
                case SIMPLE:
                    type = value.getSimpleType();
                    break;
                case ARRAY:
                    if (value.isComplex()) {
                        flags |= TypeFlag.COMPLEX.getBitmask();
                        flags |= TypeFlag.ARRAY.getBitmask();
                    } else {
                        flags |= TypeFlag.ARRAY.getBitmask();
                        type = value.getElementType();
                    }
                case PAIR:
                    if (value.isComplex()) {
                        flags |= TypeFlag.COMPLEX.getBitmask();
                        flags |= TypeFlag.PAIR.getBitmask();
                    } else {
                        flags |= TypeFlag.PAIR.getBitmask();
                        type = value.getElementType();
                    }
                case KEY_MAP:
                    flags |= TypeFlag.KEY.getBitmask();
                    break;
            }

            byte tdb = (byte) (type.getTypeId() | flags);
            dataOutputStream.writeByte(tdb);
        } catch (IOException e) {
            throw new TCPClientWriteException("Failed to write Type-Definition-Block.", e);
        } catch (IllegalStateException e) {
            throw new TCPClientWriteException("Failed to get SimpleType for: \n " + value + "\nError:" + e.getMessage(), e);
        }
    }

    /**
     * Write a simple type value to the output stream.
     *
     * @param simpleType The SimpleType to write.
     * @throws TCPClientWriteException if an I/O error occurs or the SimpleType is invalid.
     */
    private void writeSimpleType(SimpleType simpleType, Object value) throws TCPClientException {
        switch (simpleType) {
            case BOOLEAN:
                writeBoolean((boolean) value);
            case INT:
                writeInt((int) value);
            case STRING:
                writeString((String) value);
            case DOUBLE:
                writeDouble((double) value);
            default:
                throw new TCPClientWriteException("Unsupported SimpleType: " + simpleType);
        }
    }

    /**
     * Converts a float value to a 4-byte array.
     * Uses big-endian byte order.
     *
     * @param value the float value to convert
     * @return a 4-byte array representing the float
     */
    private byte[] floatToBytes(float value) {
        ByteBuffer buffer = ByteBuffer.allocate(Float.BYTES);
        buffer.order(ByteOrder.BIG_ENDIAN);
        buffer.putFloat(value);
        return buffer.array();
    }

    /**
     * Converts a double value to an 8-byte array.
     * Uses big-endian byte order.
     *
     * @param value the double value to convert
     * @return an 8-byte array representing the double
     */
    private byte[] doubleToBytes(double value) {
        ByteBuffer buffer = ByteBuffer.allocate(Double.BYTES);
        buffer.order(ByteOrder.BIG_ENDIAN);
        buffer.putDouble(value);
        return buffer.array();
    }
}
