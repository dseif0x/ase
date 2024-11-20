package com.ase.utility;

public enum TcpErrorCode {
    // Connection and Handshake Errors
    INVALID_VERSION(0xFF01, "Invalid client version", "Client needs to update their version"),
    USER_NOT_FOUND(0xFF02, "User not found", "No user exists with the provided username"),
    WRONG_PASSWORD(0xFF03, "Wrong password", "The provided password is invalid"),
    ALREADY_CONNECTED(0xFF04, "Already connected", "The provided user is already connected to the database"),
    USERNAME_ALREADY_EXISTS(0xFF05, "Username already exists", "Cannot create a user with an existing username"),

    // Permission and Access Errors
    PERMISSION_DENIED(0xFF00, "Permission denied", "User lacks required permissions"),
    PERMISSION_DENIED_CONNECTION(0xFFF0, "Permission Denied (Connection)", "Connection was opened in a way that does not allow the used operation"),

    // Key-related Errors
    INVALID_KEY(0x0100, "Invalid key", "The provided key is not valid"),
    KEY_NOT_FOUND(0x0101, "Key not found", "The specified key does not exist"),
    KEY_ALREADY_EXISTS(0x0200, "Key already exists", "Cannot overwrite existing key without forced set"),

    // Generic Error
    UNKNOWN_ERROR(0xFFFF, "General Protocol Error", "An unexpected error occurred");

    private final int code;
    private final String name;
    private final String description;

    TcpErrorCode(int code, String name, String description) {
        this.code = code;
        this.name = name;
        this.description = description;
    }

    public static TcpErrorCode fromCode(int code) {
        for (TcpErrorCode errorCode : values()) {
            if (errorCode.code == code) {
                return errorCode;
            }
        }
        return TcpErrorCode.UNKNOWN_ERROR;
    }

    public String getName() {
        return name;
    }

    @Override
    public String toString() {
        return String.format("Error %04X: %s - %s", code, name, description);
    }
}