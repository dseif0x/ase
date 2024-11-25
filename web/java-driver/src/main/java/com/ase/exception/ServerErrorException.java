package com.ase.exception;

/**
 * Custom exception to represent errors communicated from the server.
 */
public class ServerErrorException extends TCPClientException {
    private final int errorCode;
    private final String errorMessage;

    /**
     * Constructs a new ServerErrorException with the specified error code and message.
     *
     * @param errorCode    The error code sent by the server.
     * @param errorMessage The error message sent by the server.
     */
    public ServerErrorException(int errorCode, String errorMessage) {
        super("Server Error " + errorCode + ": " + errorMessage);
        this.errorCode = errorCode;
        this.errorMessage = errorMessage;
    }


    /**
     * Constructs a new ServerErrorException with only the message.
     *
     * @param errorMessage The error message sent by the server.
     */
    public ServerErrorException(String errorMessage) {
        super(errorMessage);
        this.errorMessage = errorMessage;
        this.errorCode = 0;
    }
}