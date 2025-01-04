package com.ase.backend.service;

import com.ase.backend.repository.DBClientDao;
import com.ase.utility.WebReturn;
import com.ase.utility.tcp.TcpGeneral;
import org.springframework.stereotype.Service;

import java.util.concurrent.Future;

/**
 * Service layer class that contains business logic for the in-memory database operations,
 * delegating to the {@link DBClientDao} for actual data access.
 */
@Service
public class DatabaseService {

    private final DBClientDao dao;

    /**
     * Constructs a {@code DatabaseService} with the given data access object.
     *
     * @param dao The {@link DBClientDao} implementation that will handle operations.
     */
    public DatabaseService(DBClientDao dao) {
        this.dao = dao;
    }

    /**
     * Retrieves a value from the in-memory database by key.
     *
     * @param key The unique key to look up.
     * @return A {@link WebReturn} wrapping the {@link TcpGeneral} data if successful,
     *         or an error description on failure.
     */
    public Future<WebReturn<TcpGeneral>> getValue(String key) {
        return dao.getByKey(key);
    }

    /**
     * Sets a value in the in-memory database.
     *
     * @param key   The unique key to set or update.
     * @param value A {@link TcpGeneral} value which is to be set or updated.
     * @return A {@link WebReturn} with success or error details.
     */
    public Future<WebReturn<Integer>> setValue(String key, TcpGeneral value, boolean force) {
        return dao.setByKey(key, value, force);
    }

    /**
     * Sends a command to stop all get or set operations in the in-memory database.
     *
     * @return A {@link WebReturn} indicating success or error status.
     */
    public WebReturn<Integer> stop() {
        return dao.stop();
    }

    /**
     * Deletes a key from the in-memory database.
     *
     * @param key The key to delete.
     * @return A {@link WebReturn} indicating success or containing an error message.
     */
    public WebReturn<Integer> deleteKey(String key) {
        return dao.deleteKey(key);
    }

    /**
     * Pings the server to verify communication and synchronization.
     *
     * @param packageToSend An unsigned integer data payload.
     * @return A {@link WebReturn} indicating success or containing an error message.
     */
    public WebReturn<Integer> ping(long packageToSend) {
        return dao.ping(packageToSend);
    }

    /**
     * Updates the scope permissions of a user.
     *
     * @param username The username of the user whose scope permissions are to be updated.
     * @param key The scope key to be added, edited, or deleted.
     * @param options The permission options for the scope.
     * @return A {@link WebReturn} indicating success or containing an error message.
     */
    public WebReturn<Integer> updateScope(String username, String key, byte options) {
        return dao.updateScope(username, key, options);
    }

    /**
     * Creates a new user with the given username and password.
     *
     * @param username The username of the new user.
     * @param password The password of the new user.
     * @param options The options for the new user.
     * @return A {@link WebReturn} indicating success or containing an error message.
     */
    public WebReturn<Integer> createUser(String username, String password, byte options) {
        return dao.createUser(username, password, options);
    }
}