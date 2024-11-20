package com.ase.exception;

/**
 * An exception specifically for handling TCP handshake-related errors.
 */
public class HandshakeException extends TCPClientException {
    /**
     * Constructs a new HandshakeException with a specific error message.
     *
     * @param message A detailed description.
     */
    public HandshakeException(String message) {
        super("Handshake failed. " + message);
    }
}