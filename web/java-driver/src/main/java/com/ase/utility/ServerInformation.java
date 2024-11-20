package com.ase.utility;

/**
 * Represents server-side information about size constraints for different data types.
 */
public class ServerInformation {
    /** Size limit for boolean type. */
    private final int booleanSize;

    /** Size limit for integer type. */
    private final int intSize;

    /** Size limit for character type. */
    private final int charSize;

    /** Size limit for double type. */
    private final int doubleSize;

    /** Size limit for semaphore type. */
    private final int semaphoreSize;

    /** Size limit for string type. */
    private final int stringSize;

    /** Size limit for array type. */
    private final int arraySize;

    /**
     * Constructs a new ServerInformation instance with specified size limits for various data types.
     *
     * @param booleanSize   Maximum size limit for boolean type.
     * @param intSize       Maximum size limit for integer type.
     * @param charSize      Maximum size limit for character type.
     * @param doubleSize    Maximum size limit for double type.
     * @param semaphoreSize Maximum size limit for semaphore type.
     * @param stringSize    Maximum size limit for string type.
     * @param arraySize     Maximum size limit for array type.
     */
    public ServerInformation(int booleanSize, int intSize, int charSize, int doubleSize, int semaphoreSize, int stringSize, int arraySize) {
        this.booleanSize = booleanSize;
        this.intSize = intSize;
        this.charSize = charSize;
        this.doubleSize = doubleSize;
        this.semaphoreSize = semaphoreSize;
        this.stringSize = stringSize;
        this.arraySize = arraySize;
    }

    /**
     * Checks if the given size is within the boolean type size limit.
     *
     * @param size The size to check against the boolean size limit.
     * @return true if the given size is less than or equal to the boolean size limit, false otherwise.
     */
    public boolean checkBooleanSize(int size) {
        return this.booleanSize >= size;
    }

    /**
     * Checks if the given size is within the integer type size limit.
     *
     * @param size The size to check against the integer size limit.
     * @return true if the given size is less than or equal to the integer size limit, false otherwise.
     */
    public boolean checkIntSize(int size) {
        return this.intSize >= size;
    }

    /**
     * Checks if the given size is within the character type size limit.
     *
     * @param size The size to check against the character size limit.
     * @return true if the given size is less than or equal to the character size limit, false otherwise.
     */
    public boolean checkCharSize(int size) {
        return this.charSize >= size;
    }

    /**
     * Checks if the given size is within the double type size limit.
     *
     * @param size The size to check against the double size limit.
     * @return true if the given size is less than or equal to the double size limit, false otherwise.
     */
    public boolean checkDoubleSize(int size) {
        return this.doubleSize >= size;
    }

    /**
     * Checks if the given size is within the semaphore type size limit.
     *
     * @param size The size to check against the semaphore size limit.
     * @return true if the given size is less than or equal to the semaphore size limit, false otherwise.
     */
    public boolean checkSemaphoreSize(int size) {
        return this.semaphoreSize >= size;
    }

    /**
     * Checks if the given size is within the string type size limit.
     *
     * @param size The size to check against the string size limit.
     * @return true if the given size is less than or equal to the string size limit, false otherwise.
     */
    public boolean checkStringSize(int size) {
        return this.stringSize >= size;
    }

    /**
     * Checks if the given size is within the array type size limit.
     *
     * @param size The size to check against the array size limit.
     * @return true if the given size is less than or equal to the array size limit, false otherwise.
     */
    public boolean checkArraySize(int size) {
        return this.arraySize >= size;
    }
}