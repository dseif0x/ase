package com.ase.exception;

public class TCPClientWriteException extends TCPClientException {
    public TCPClientWriteException(String message) {
        super("Write failed. " + message);
    }

    public TCPClientWriteException(String message, Throwable cause) {
        super("Write failed. " + message, cause);
    }
}
