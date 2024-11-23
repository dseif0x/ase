package com.ase.utility;

import com.ase.exception.HandshakeException;
import com.ase.exception.TCPClientException;

import java.io.DataInputStream;

/**
 * Represents server-side information about size constraints for different data types.
 */
public class ServerInformation {
    /** Size limit for boolean type. */
    private int booleanSize;

    /** Size limit for integer type. */
    private int intSize;

    /** Size limit for character type. */
    private int charSize;

    /** Size limit for double type. */
    private int doubleSize;

    /** Size limit for semaphore type. */
    private int semaphoreSize;

    /** Size limit for string type. */
    private int stringSize;

    /** Size limit for array type. */
    private int arraySize;

    /**
     * Reads server information from the given input stream.
     *
     * @param reader the input stream to read from
     * @return the server information read from the stream
     * @throws HandshakeException if there is an error reading the server information
     */
    public static ServerInformation readServerInformation(DataInputStream reader) throws HandshakeException {
        ServerInformation serverInformation = new ServerInformation();

        try {
            serverInformation.setBooleanSize(reader.readUnsignedByte());
            serverInformation.setIntSize(reader.readUnsignedByte());
            serverInformation.setCharSize(reader.readUnsignedByte());
            serverInformation.setDoubleSize(reader.readUnsignedByte());
            serverInformation.setSemaphoreSize(reader.readUnsignedByte());
            serverInformation.setStringSize(reader.readUnsignedShort());
            serverInformation.setArraySize(reader.readUnsignedShort());
        } catch (Exception e) {
            throw new HandshakeException("Error reading server information");
        }

        return serverInformation;
    }

    /**
     * Retrieves the size limit for the boolean type.
     *
     * @return the size of boolean in bytes
     */
    public int getBooleanSize() {
        return booleanSize;
    }

    /**
     * Sets the size limit for the boolean type.
     *
     * @param booleanSize the size of boolean in bytes, must be positive
     */
    public void setBooleanSize(int booleanSize) {
        this.booleanSize = booleanSize;
    }

    /**
     * Retrieves the size limit for the integer type.
     *
     * @return the size of integer in bytes
     */
    public int getIntSize() {
        return intSize;
    }

    /**
     * Sets the size limit for the integer type.
     *
     * @param intSize the size of integer in bytes, must be positive
     */
    public void setIntSize(int intSize) {
        this.intSize = intSize;
    }

    /**
     * Retrieves the size limit for the character type.
     *
     * @return the size of character in bytes
     */
    public int getCharSize() {
        return charSize;
    }

    /**
     * Sets the size limit for the character type.
     *
     * @param charSize the size of character in bytes, must be positive
     */
    public void setCharSize(int charSize) {
        this.charSize = charSize;
    }

    /**
     * Retrieves the size limit for the double type.
     *
     * @return the size of double in bytes
     */
    public int getDoubleSize() {
        return doubleSize;
    }

    /**
     * Sets the size limit for the double type.
     *
     * @param doubleSize the size of double in bytes, must be positive
     */
    public void setDoubleSize(int doubleSize) {
        this.doubleSize = doubleSize;
    }

    /**
     * Retrieves the size limit for the semaphore type.
     *
     * @return the size of semaphore in bytes
     */
    public int getSemaphoreSize() {
        return semaphoreSize;
    }

    /**
     * Sets the size limit for the semaphore type.
     *
     * @param semaphoreSize the size of semaphore in bytes, must be positive
     */
    public void setSemaphoreSize(int semaphoreSize) {
        this.semaphoreSize = semaphoreSize;
    }

    /**
     * Retrieves the size limit for the string type.
     *
     * @return the size of string in bytes
     */
    public int getStringSize() {
        return stringSize;
    }

    /**
     * Sets the size limit for the string type.
     *
     * @param stringSize the size of string in bytes, must be positive
     */
    public void setStringSize(int stringSize) {
        this.stringSize = stringSize;
    }

    /**
     * Retrieves the size limit for the array type.
     *
     * @return the size of array in bytes
     */
    public int getArraySize() {
        return arraySize;
    }

    /**
     * Sets the size limit for the array type.
     *
     * @param arraySize the size of array in bytes, must be positive
     */
    public void setArraySize(int arraySize) {
        this.arraySize = arraySize;
    }
}