package com.ase.backend.repository;

import com.ase.utility.WebReturn;
import com.ase.utility.tcp.TcpGeneral;

import java.util.concurrent.Future;

/**
 * DAO interface defining operations to interact with the in-memory database via the DatabaseClient.
 */
public interface DBClientDao {

    /**
     * Retrieves a list of first level sub keys in a map.
     *
     * @param key The unique identifier (key) for which to retrieve the value.
     * @return A {@link WebReturn} wrapping the {@link TcpGeneral} data if successful,
     * or an error description if retrieval fails or is stopped.
     */
    WebReturn<TcpGeneral> listByKey(String key);

    /**
     * Retrieves the value associated with the given key.
     *
     * @param key The unique identifier (key) for which to retrieve the value.
     * @return A {@link WebReturn} wrapping the {@link TcpGeneral} data if successful,
     * or an error description if retrieval fails or is stopped.
     */
    Future<WebReturn<TcpGeneral>> getByKey(String key);

    /**
     * Sets the provided value for the given key.
     *
     * @param key   The unique identifier (key) to be set or updated.
     * @param value The {@link TcpGeneral} value which is to be set or updated.
     * @return A {@link WebReturn} containing an integer success code or error details.
     */
    Future<WebReturn<Integer>> setByKey(String key, TcpGeneral value, boolean force);

    /**
     * Sends a stop command to the in-memory database, which interrupts ongoing operations.
     *
     * @return A {@link WebReturn} indicating success or an error message if the stop failed.
     */
    WebReturn<Integer> stop();

    /**
     * Deletes a key from the in-memory database.
     *
     * @param key the key to delete
     * @return a {@link WebReturn} indicating success or containing an error message
     */
    WebReturn<Integer> deleteKey(String key);

    /**
     * Pings the server to verify communication and synchronization.
     *
     * @param packageToSend an unsigned integer data payload
     * @return a {@link WebReturn} indicating success or containing an error message
     */
    WebReturn<Integer> ping(long packageToSend);

    /**
     * Updates the scope permissions of a user.
     *
     * @param username The username of the user whose scope permissions are to be updated.
     * @param key      The scope key to be added, edited, or deleted.
     * @param options  The permission options for the scope:
     */
    WebReturn<Integer> updateScope(String username, String key, byte options);

    /**
     * Creates a new user with the given username and password.
     *
     * @param username The username of the new user.
     * @param password The password of the new user.
     * @param options  The options for the new user.
     * @return a {@link WebReturn} indicating success or containing an error message
     */
    WebReturn<Integer> createUser(String username, String password, byte options);
}
