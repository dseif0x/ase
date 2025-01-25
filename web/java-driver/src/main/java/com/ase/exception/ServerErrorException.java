/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.exception;

/**
 * Custom exception to represent errors communicated from the server.
 */
public class ServerErrorException extends TCPClientException {

    /** The error code sent by the server. */
    private int errorCode;

    /** The error message sent by the server. */
    private String errorMessage;

    /**
     * Constructs a new ServerErrorException with the specified error code and message.
     *
     * @param errorCode    The error code sent by the server.
     * @param errorMessage The error message sent by the server.
     */
    public ServerErrorException(int errorCode, String errorMessage) {
        super("Server Error " + errorCode + ": " + errorMessage);

        this.errorMessage = errorMessage;
        this.errorCode = errorCode;
    }

    /**
     * Retrieves the error code sent by the server.
     *
     * @return The error code sent by the server.
     */
    public int getErrorCode() {
        return errorCode;
    }

    /**
     * Retrieves the error message sent by the server.
     *
     * @return The error message sent by the server.
     */
    public String getErrorMessage() {
        return errorMessage;
    }
}