package com.ase.utility;

import com.ase.exception.TCPClientException;
import com.ase.exception.TCPClientWriteException;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

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
    public void writeBoolean(boolean value) throws TCPClientException {
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
    public void writeInt(int value) throws TCPClientException {
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
    public void writeDouble(double value) throws TCPClientException {
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
