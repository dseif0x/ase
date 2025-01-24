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
     * Retrieves a list of first level sub keys in a map.
     *
     * @param key The unique key to look up.
     * @return A {@link WebReturn} wrapping the {@link TcpGeneral} data if successful,
     *         or an error description on failure.
     */
    public Future<WebReturn<TcpGeneral>> list(String key, String connectionId) {
        return dao.listByKey(key, connectionId);
    }

    /**
     * Retrieves a value from the in-memory database by key.
     *
     * @param key The unique key to look up.
     * @return A {@link WebReturn} wrapping the {@link TcpGeneral} data if successful,
     *         or an error description on failure.
     */
    public Future<WebReturn<TcpGeneral>> getValue(String key, String connectionId) {
        return dao.getByKey(key, connectionId);
    }

    /**
     * Sets a value in the in-memory database.
     *
     * @param key   The unique key to set or update.
     * @param value A {@link TcpGeneral} value which is to be set or updated.
     * @return A {@link WebReturn} with success or error details.
     */
    public Future<WebReturn<Integer>> setValue(String key, TcpGeneral value, boolean force, String connectionId) {
        return dao.setByKey(key, value, force, connectionId);
    }

    /**
     * Sends a command to stop all get or set operations in the in-memory database.
     *
     * @return A {@link WebReturn} indicating success or error status.
     */
    public WebReturn<Integer> stop(String connectionId) {
        return dao.stop(connectionId);
    }

    /**
     * Deletes a key from the in-memory database.
     *
     * @param key The key to delete.
     * @return A {@link WebReturn} indicating success or containing an error message.
     */
    public WebReturn<Integer> deleteKey(String key, String connectionId) {
        return dao.deleteKey(key, connectionId);
    }

    /**
     * Pings the server to verify communication and synchronization.
     *
     * @param packageToSend An unsigned integer data payload.
     * @return A {@link WebReturn} indicating success or containing an error message.
     */
    public WebReturn<Integer> ping(long packageToSend, String connectionId) {
        return dao.ping(packageToSend, connectionId);
    }

    /**
     * Updates the scope permissions of a user.
     *
     * @param username The username of the user whose scope permissions are to be updated.
     * @param key The scope key to be added, edited, or deleted.
     * @param options The permission options for the scope.
     * @return A {@link WebReturn} indicating success or containing an error message.
     */
    public WebReturn<Integer> updateScope(String username, String key, byte options, String connectionId) {
        return dao.updateScope(username, key, options, connectionId);
    }

    /**
     * Creates a new user with the given username and password.
     *
     * @param username The username of the new user.
     * @param password The password of the new user.
     * @param options The options for the new user.
     * @return A {@link WebReturn} indicating success or containing an error message.
     */
    public WebReturn<Integer> createUser(String username, String password, byte options, String connectionId) {
        return dao.createUser(username, password, options, connectionId);
    }

    /**
     * Follows a reference in the database.
     *
     * @param key The key to follow.
     * @param maxJump The maximum number of jumps to follow.
     * @param isSafe Whether to follow the reference safely.
     * @return A {@link WebReturn} indicating success or containing an error message.
     */
    public WebReturn<TcpGeneral> follow(String key, long maxJump, boolean isSafe, String connectionId) {
        return dao.follow(key, maxJump, isSafe, connectionId);
    }

    /**
     * Retrieves the permissions of a list of users.
     *
     * @param usernames Usernames to get permissions from.
     * @return A {@link WebReturn} wrapping the {@link TcpGeneral} data if successful,
     * or an error description if retrieval fails or is stopped.
     */
    public WebReturn<TcpGeneral> getUserPermissions(TcpGeneral usernames, String connectionId) {
        return dao.getUserPermissions(usernames, connectionId);
    }

    /** Checks if the connection is closed.
     *  @param connectionId The connection ID to check.
     * @return True if the connection is closed, false otherwise.
     *
     * */
    public boolean connectionClosed(String connectionId) {
        return dao.connectionClosed(connectionId);
    }
}