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
    public WebReturn<TcpGeneral> listByKey(String key) {
        DatabaseClient databaseClient = connectionService.getClient();
        return databaseClient.list(key);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Future<WebReturn<TcpGeneral>> getByKey(String key){
        DatabaseClient databaseClient = connectionService.getClient();
        return databaseClient.getAsync(key);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Future<WebReturn<Integer>> setByKey(String key, TcpGeneral value, boolean force) {
        DatabaseClient databaseClient = connectionService.getClient();
        return databaseClient.setAsync(key, value, force);
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

    @Override
    public WebReturn<Integer> deleteKey(String key) {
        return connectionService.getClient().delete(key);
    }

    @Override
    public WebReturn<Integer> ping(long packageToSend) {
        return connectionService.getClient().ping(packageToSend);
    }

    @Override
    public WebReturn<Integer> updateScope(String username, String key, byte options) {
        return connectionService.getClient().updateScope(username, key, options);
    }

    @Override
    public WebReturn<Integer> createUser(String username, String password, byte options) {
        return connectionService.getClient().createUser(username, password, options);
    }
}