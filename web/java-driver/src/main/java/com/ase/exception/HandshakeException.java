package com.ase.exception;

public class HandshakeException extends TCPClientException {
    public HandshakeException(String message) {
        super("Handshake failed. " + message);
    }
}