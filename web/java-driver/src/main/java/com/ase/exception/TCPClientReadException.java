package com.ase.exception;

public class TCPClientReadException extends TCPClientException {
    public TCPClientReadException(String message) {
        super("Read failed. " + message);
    }

    public TCPClientReadException(String message, Throwable cause) {
        super("Read failed. " + message, cause);
    }
}