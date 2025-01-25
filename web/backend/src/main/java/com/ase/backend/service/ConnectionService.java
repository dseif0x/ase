/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.backend.service;

import com.ase.driver.DatabaseClient;
import com.ase.exception.TCPClientException;
import jakarta.annotation.PreDestroy;
import org.springframework.stereotype.Service;

import java.io.IOException;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Manages the lifecycle of the DatabaseClient,
 * creating it on user login and storing it for subsequent use.
 */
@Service
public class ConnectionService {

    private final Map<String, DatabaseClient> clientMap = new ConcurrentHashMap<>();

    /**
     * Creates and initializes a new DatabaseClient using the given connection info.
     * Stores it in an AtomicReference so the rest of the app can retrieve it.
     *
     * @param host The hostname or IP address of the in-memory DB.
     * @param port The port for the in-memory DB.
     * @param version The version of the in-memory DB.
     * @param flags The flags for the in-memory DB.
     * @param username The username for login.
     * @param password The password for login.
     * @throws IOException         If the socket connection fails.
     * @throws TCPClientException  If connect/login fails for server-related reasons.
     */
    public String openConnection(String host, int port, int version, byte flags, String username, String password) throws IOException, TCPClientException {
        DatabaseClient newClient = new DatabaseClient(host, port);
        newClient.connect(version, flags);
        newClient.login(username, password);
        String connectionId = UUID.randomUUID().toString();
        clientMap.put(connectionId, newClient);
        return connectionId;
    }

    /**
     * Retrieves the active DatabaseClient instance.
     *
     * @return The active DatabaseClient instance, or null if not yet created/logged in.
     */
    public DatabaseClient getClient(String connectionId) {
        return clientMap.get(connectionId);
    }

    /**
     * Removes the stored DatabaseClient (if present) and disconnects it.
     */
    public void logout(String connectionId) {
        DatabaseClient client = clientMap.remove(connectionId);
        if (client != null) {
            client.disconnect();
        }
    }

    @PreDestroy
    public void onShutdown() {
        clientMap.values().forEach(DatabaseClient::disconnect);
        clientMap.clear();
    }
}
