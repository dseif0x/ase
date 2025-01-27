/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
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