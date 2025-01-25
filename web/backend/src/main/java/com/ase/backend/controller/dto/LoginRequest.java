/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.backend.controller.dto;

public class LoginRequest {
    private final String host;
    private final int port;
    private final int version;
    private final byte flags;
    private final String username;
    private final String password;

    /**
     * Constructs a new empty LoginRequest.
     */
    public LoginRequest() {
        this.host = null;
        this.port = 0;
        this.version = 0;
        this.flags = 0;
        this.username = null;
        this.password = null;
    }

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


    /**
     * Gets the hostname or IP address of the in-memory DB.
     *
     * @return The hostname or IP address of the in-memory DB.
     */
    public String getHost() { return host; }

    /**
     * Gets the port for the in-memory DB.
     *
     * @return The port for the in-memory DB.
     */
    public int getPort() { return port; }

    /**
     * Gets the version of the in-memory DB.
     *
     * @return The version of the in-memory DB.
     */
    public int getVersion() { return version; }

    /**
     * Gets the flags for the in-memory DB.
     *
     * @return The flags for the in-memory DB.
     */
    public byte getFlags() { return flags; }

    /**
     * Gets the username for login.
     *
     * @return The username for login.
     */
    public String getUsername() { return username; }

    /**
     * Gets the password for login.
     *
     * @return The password for login.
     */
    public String getPassword() { return password; }
}