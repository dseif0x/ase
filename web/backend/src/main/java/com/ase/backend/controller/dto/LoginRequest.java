package com.ase.backend.controller.dto;

public class LoginRequest {
    private String host;
    private int port;
    private int version;
    private byte flags;
    private String username;

    /**
     * Constructs a new LoginRequest.
     */
    public LoginRequest(String host, int port, int version, byte flags, String username, String password) {
        this.host = host;
        this.port = port;
        this.version = version;
        this.flags = flags;
        this.username = username;
        this.password = password;
    }

    private String password;

    // getters/setters
    public String getHost() { return host; }
    public int getPort() { return port; }
    public int getVersion() { return version; }
    public byte getFlags() { return flags; }
    public String getUsername() { return username; }
    public String getPassword() { return password; }
}