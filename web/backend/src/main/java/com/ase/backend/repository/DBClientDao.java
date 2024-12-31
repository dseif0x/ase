package com.ase.backend.repository;

import com.ase.utility.WebReturn;
import com.ase.utility.tcp.TcpGeneral;

/**
 * DAO interface defining operations to interact with the in-memory database via the DatabaseClient.
 */
public interface DBClientDao {

    /**
     * Retrieves the value associated with the given key.
     *
     * @param key The unique identifier (key) for which to retrieve the value.
     * @return A {@link WebReturn} wrapping the {@link TcpGeneral} data if successful,
     *         or an error description if retrieval fails or is stopped.
     */
    WebReturn<TcpGeneral> getByKey(String key);

    /**
     * Sets the provided value for the given key.
     *
     * @param key   The unique identifier (key) to be set or updated.
     * @param value The {@link TcpGeneral} value which is to be set or updated.
     * @return A {@link WebReturn} containing an integer success code or error details.
     */
    WebReturn<Integer> setByKey(String key, TcpGeneral value, boolean force);

    /**
     * Sends a stop command to the in-memory database, which interrupts ongoing operations.
     *
     * @return A {@link WebReturn} indicating success or an error message if the stop failed.
     */
    WebReturn<Integer> stop();
}
