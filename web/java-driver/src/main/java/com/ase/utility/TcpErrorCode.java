package com.ase.utility;

public enum TcpErrorCode {
    // Connection and Handshake Errors
    INVALID_VERSION(0x8000, "Invalid client version", "Client needs to update their version"),
    USER_NOT_FOUND(0x8001, "User not found", "No user exists with the provided username"),
    WRONG_PASSWORD(0x8002, "Wrong password", "The provided password is invalid"),

    // Permission and Access Errors
    PERMISSION_DENIED(0xFF00, "Permission denied", "User lacks required permissions"),

    // Key-related Errors
    INVALID_KEY(0x0100, "Invalid key", "The provided key is not valid"),
    KEY_NOT_FOUND(0x0101, "Key not found", "The specified key does not exist"),
    KEY_ALREADY_EXISTS(0x0200, "Key already exists", "Cannot overwrite existing key without forced set"),

    // User Management Errors
    USERNAME_ALREADY_EXISTS(0x8000, "Username already exists", "Cannot create a user with an existing username");

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
        return null;
    }

    public int getCode() {
        return code;
    }

    public String getName() {
        return name;
    }

    public String getDescription() {
        return description;
    }

    @Override
    public String toString() {
        return String.format("Error %04X: %s - %s", code, name, description);
    }
}