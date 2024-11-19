package com.ase.utility;

public class ServerInformation {
    private final int booleanSize;
    private final int intSize;
    private final int charSize;
    private final int doubleSize;
    private final int semaphoreSize;
    private final int stringSize;
    private final int arraySize;

    public ServerInformation(int booleanSize, int intSize, int charSize, int doubleSize, int semaphoreSize, int stringSize, int arraySize) {
        this.booleanSize = booleanSize;
        this.intSize = intSize;
        this.charSize = charSize;
        this.doubleSize = doubleSize;
        this.semaphoreSize = semaphoreSize;
        this.stringSize = stringSize;
        this.arraySize = arraySize;
    }

    public boolean checkBooleanSize(int size) {
        return this.booleanSize >= size;
    }

    public boolean checkIntSize(int size) {
        return this.intSize >= size;
    }

    public boolean checkCharSize(int size) {
        return this.charSize >= size;
    }

    public boolean checkDoubleSize(int size) {
        return this.doubleSize >= size;
    }

    public boolean checkSemaphoreSize(int size) {
        return this.semaphoreSize >= size;
    }

    public boolean checkStringSize(int size) {
        return this.stringSize >= size;
    }

    public boolean checkArraySize(int size) {
        return this.arraySize >= size;
    }
}
