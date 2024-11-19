package com.ase.exception;

public class TCPClientException extends Exception {
    public TCPClientException(String message) {
        super(message);
    }

    public TCPClientException(String message, Throwable cause) {
        super(message, cause);
    }
}