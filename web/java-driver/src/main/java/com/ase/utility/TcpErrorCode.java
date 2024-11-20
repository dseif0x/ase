package com.ase.utility;

/**
 * Represents error codes for TCP communication.
 * <p>
 * This enum defines a set of error codes used in the TCP communication protocol. Errors are
 * categorized into connection, permission, and key-related errors. Each error code
 * includes a numeric code, a name, and a detailed description.
 */
public enum TcpErrorCode {
    // --- START CONNECTION ERRORS ---
    /**
     * Error indicating an invalid client version.
     */
    INVALID_VERSION(0xFF01, "Invalid client version", "Client needs to update their version"),

    /**
     * Error indicating that the specified user was not found.
     */
    USER_NOT_FOUND(0xFF02, "User not found", "No user exists with the provided username"),

    /**
     * Error indicating an incorrect password was provided.
     */
    WRONG_PASSWORD(0xFF03, "Wrong password", "The provided password is invalid"),

    /**
     * Error indicating the user is already connected to the database.
     */
    ALREADY_CONNECTED(0xFF04, "Already connected", "The provided user is already connected to the database"),

    /**
     * Error indicating an attempt to create a user with an existing username.
     */
    USERNAME_ALREADY_EXISTS(0xFF05, "Username already exists", "Cannot create a user with an existing username"),

    // --- END CONNECTION ERRORS ---

    // --- START PERMISSION AMD ACCESS ERRORS ---

    /**
     * Generic permission denial error.
     */
    PERMISSION_DENIED(0xFF00, "Permission denied", "User lacks required permissions"),

    /**
     * Specific permission denial error related to connection.
     */
    PERMISSION_DENIED_CONNECTION(0xFFF0, "Permission Denied (Connection)", "Connection was opened in a way that does not allow the used operation"),

    // --- END PERMISSION AMD ACCESS ERRORS ---

    // START KEY-RELATED ERRORS ---

    /**
     * Error indicating an invalid key was provided.
     */
    INVALID_KEY(0x0100, "Invalid key", "The provided key is not valid"),

    /**
     * Error indicating the specified key does not exist.
     */
    KEY_NOT_FOUND(0x0101, "Key not found", "The specified key does not exist"),

    /**
     * Error indicating an attempt to overwrite an existing key.
     */
    KEY_ALREADY_EXISTS(0x0200, "Key already exists", "Cannot overwrite existing key without forced set"),

    // END KEY-RELATED ERRORS ---

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