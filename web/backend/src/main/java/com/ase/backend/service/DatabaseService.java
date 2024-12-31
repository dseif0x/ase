package com.ase.backend.service;

import com.ase.backend.repository.DBClientDao;
import com.ase.utility.WebReturn;
import com.ase.utility.tcp.TcpGeneral;
import org.springframework.stereotype.Service;

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
    public WebReturn<TcpGeneral> getValue(String key) {
        return dao.getByKey(key);
    }

    /**
     * Sets a value in the in-memory database.
     *
     * @param key   The unique key to set or update.
     * @param value A {@link TcpGeneral} value which is to be set or updated.
     * @return A {@link WebReturn} with success or error details.
     */
    public WebReturn<Integer> setValue(String key, TcpGeneral value, boolean force) {
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
}