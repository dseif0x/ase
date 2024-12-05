package com.ase.exception;

/**
 * Custom exception to represent errors communicated from the server.
 */
public class ServerErrorException extends TCPClientException {

    private int errorCode;
    /**
     * Constructs a new ServerErrorException with the specified error code and message.
     *
     * @param errorCode    The error code sent by the server.
     * @param errorMessage The error message sent by the server.
     */
    public ServerErrorException(int errorCode, String errorMessage) {
        super("Server Error " + errorCode + ": " + errorMessage);
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
}