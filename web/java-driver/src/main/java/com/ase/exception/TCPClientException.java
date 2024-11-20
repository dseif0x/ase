package com.ase.exception;

/**
 * A base exception class for TCP client-related errors.
 */
public class TCPClientException extends Exception {
    /**
     * Constructs a new TCPClientException with a specific error message.
     *
     * @param message A descriptive message.
     */
    public TCPClientException(String message) {
        super(message);
    }

    /**
     * Constructs a new TCPClientException with a specific error message and cause.
     *
     * @param message A descriptive message.
     * @param cause   The original throwable that caused this exception to be raised.
     */
    public TCPClientException(String message, Throwable cause) {
        super(message, cause);
    }
}