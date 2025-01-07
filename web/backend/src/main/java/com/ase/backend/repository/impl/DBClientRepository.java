package com.ase.backend.repository.impl;

import com.ase.backend.repository.DBClientDao;
import com.ase.backend.service.ConnectionService;
import com.ase.driver.DatabaseClient;
import com.ase.utility.WebReturn;
import com.ase.utility.tcp.TcpGeneral;
import org.springframework.stereotype.Repository;

import java.util.Random;
import java.util.concurrent.Future;

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
    public Future<WebReturn<TcpGeneral>> listByKey(String key, String connectionId) {
        DatabaseClient databaseClient = connectionService.getClient(connectionId);
        return databaseClient.listAsync(key);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Future<WebReturn<TcpGeneral>> getByKey(String key, String connectionId){
        DatabaseClient databaseClient = connectionService.getClient(connectionId);
        return databaseClient.getAsync(key);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Future<WebReturn<Integer>> setByKey(String key, TcpGeneral value, boolean force, String connectionId) {
        DatabaseClient databaseClient = connectionService.getClient(connectionId);
        return databaseClient.setAsync(key, value, force);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public WebReturn<Integer> stop(String connectionId) {
        DatabaseClient databaseClient = connectionService.getClient(connectionId);

        Random random = new Random();
        long unsignedInt = Integer.toUnsignedLong(random.nextInt()) & 0xFFFFFFFFL;

        return databaseClient.stop(unsignedInt);
    }

    @Override
    public WebReturn<Integer> deleteKey(String key, String connectionId) {
        return connectionService.getClient(connectionId).delete(key);
    }

    @Override
    public WebReturn<Integer> ping(long packageToSend, String connectionId) {
        return connectionService.getClient(connectionId).ping(packageToSend);
    }

    @Override
    public WebReturn<Integer> updateScope(String username, String key, byte options, String connectionId) {
        return connectionService.getClient(connectionId).updateScope(username, key, options);
    }

    @Override
    public WebReturn<Integer> createUser(String username, String password, byte options, String connectionId) {
        return connectionService.getClient(connectionId).createUser(username, password, options);
    }
}