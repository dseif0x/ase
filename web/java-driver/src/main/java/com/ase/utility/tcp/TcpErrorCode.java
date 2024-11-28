package com.ase.utility.tcp;

/**
 * Represents error codes for TCP communication.
 * It is used before the connection handshake and server information is established.
 */
public enum TcpErrorCode {
    /**
     * Error indicating an invalid client version.
     */
    INVALID_VERSION(0xFF01, "Invalid client version", "Client needs to update their version"),

    /**
     * Specific permission denial error related to connection.
     */
    PERMISSION_DENIED_CONNECTION(0xFFF0, "Permission Denied (Connection)", "Connection was opened in a way that does not allow the used operation"),

    /**
     * Generic unknown error code.
     */
    UNKNOWN_ERROR(0xFFFF, "General Protocol Error", "An unexpected error occurred");

    /** The numeric error code. */
    private final int code;

    /** The name of the error. */
    private final String name;

    /** A detailed description of the error. */
    private final String description;

    /**
     * Constructs a TcpErrorCode with a specific code, name, and description.
     *
     * @param code        The numeric error code.
     * @param name        A short name describing the error.
     * @param description A detailed explanation of the error.
     */
    TcpErrorCode(int code, String name, String description) {
        this.code = code;
        this.name = name;
        this.description = description;
    }

    /**
     * Converts a numeric error code to its corresponding TcpErrorCode enum value.
     *
     * @param code The numeric error code to convert.
     * @return The matching TcpErrorCode, or UNKNOWN_ERROR if no match is found.
     */
    public static TcpErrorCode fromCode(int code) {
        for (TcpErrorCode errorCode : values()) {
            if (errorCode.code == code) {
                return errorCode;
            }
        }
        return TcpErrorCode.UNKNOWN_ERROR;
    }

    /**
     * Retrieves the name of the error.
     *
     * @return The name of the error.
     */
    public String getName() {
        return name;
    }

    /**
     * Provides a formatted string representation of the error.
     *
     * @return A string containing the error code, name, and description.
     */
    @Override
    public String toString() {
        return String.format("Error %04X: %s - %s", code, name, description);
    }
}