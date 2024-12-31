package com.ase.backend.repository.impl;

import com.ase.backend.repository.DBClientDao;
import com.ase.backend.service.ConnectionService;
import com.ase.driver.DatabaseClient;
import com.ase.utility.WebReturn;
import com.ase.utility.tcp.SimpleType;
import com.ase.utility.tcp.TcpGeneral;
import org.springframework.stereotype.Repository;

import java.util.Random;

/**
 * Concrete implementation of the {@link DBClientDao} interface that uses
 * the {@link DatabaseClient} to interact with the in-memory database.
 */
@Repository
public class DBClientRepository implements DBClientDao {

    private final ConnectionService connectionService;

    /**
     * Constructs the DBClientRepository with a {@link DatabaseClient}.
     *
     * @param connectionService The service that manages the lifecycle of the DatabaseClient.
     */
    public DBClientRepository(ConnectionService connectionService) {
        this.connectionService = connectionService;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public WebReturn<TcpGeneral> getByKey(String key) {
        DatabaseClient databaseClient = connectionService.getClient();
        return databaseClient.get(key);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public WebReturn<Integer> setByKey(String key, TcpGeneral value, boolean force) {
        DatabaseClient databaseClient = connectionService.getClient();
        return databaseClient.set(key, value, force);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public WebReturn<Integer> stop() {
        DatabaseClient databaseClient = connectionService.getClient();

        Random random = new Random();
        long unsignedInt = Integer.toUnsignedLong(random.nextInt()) & 0xFFFFFFFFL;

        return databaseClient.stop(unsignedInt);
    }
}