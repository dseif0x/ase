package com.ase.backend.service;

import com.ase.driver.DatabaseClient;
import com.ase.exception.TCPClientException;
import org.springframework.stereotype.Service;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Manages the lifecycle of the DatabaseClient,
 * creating it on user login and storing it for subsequent use.
 */
@Service
public class ConnectionService {

    private final AtomicReference<DatabaseClient> currentClient = new AtomicReference<>();

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
    public void createClient(String host, int port, int version, byte flags, String username, String password) throws IOException, TCPClientException {
        DatabaseClient newClient = new DatabaseClient(host, port);
        newClient.connect(version, flags);
        newClient.login(username, password);
        this.currentClient.set(newClient);
    }

    /**
     * Retrieves the active DatabaseClient instance.
     *
     * @return The active DatabaseClient instance, or null if not yet created/logged in.
     */
    public DatabaseClient getClient() {
        return currentClient.get();
    }
}
