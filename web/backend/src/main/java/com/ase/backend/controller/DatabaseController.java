/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.backend.controller;

import com.ase.backend.controller.dto.SetValueRequest;
import com.ase.backend.service.ConnectionService;
import com.ase.backend.service.DatabaseService;
import com.ase.utility.WebReturn;
import com.ase.utility.tcp.TcpGeneral;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.Base64;
import java.util.concurrent.Future;

/**
 * REST controller for exposing in-memory database operations over HTTP.
 */
@RestController
@RequestMapping("/api/db")
public class DatabaseController {

    private final DatabaseService service;
    private final ConnectionService connectionService;

    /**
     * Constructs a {@code DatabaseController} with the given service.
     *
     * @param service The {@link DatabaseService} that handles business logic.
     */
    public DatabaseController(DatabaseService service, ConnectionService connectionService) {
        this.service = service;
        this.connectionService = connectionService;
    }

    /**
     * Retrieves a list of first level sub keys in a map.
     *
     * @param key The unique identifier for the value to be fetched.
     * @return A {@link WebReturn} wrapping the {@link TcpGeneral} or an error message.
     */
    @GetMapping("/list/{key}")
    public ResponseEntity<WebReturn<TcpGeneral>> list(@PathVariable String key, @RequestHeader(value="X-Connection-Id") String connectionId) {
        String decodedKey = new String(Base64.getDecoder().decode(key.getBytes()));
        try {
            Future<WebReturn<TcpGeneral>> future = service.list(decodedKey, connectionId);
            return ResponseEntity.ok(future.get());
        } catch (Exception e) {
            connectionService.logout(connectionId);
            return ResponseEntity.status(500).body(WebReturn.error(e.getMessage()));
        }
    }

    /**
     * Retrieves a value from the in-memory database by key.
     *
     * @param key The unique identifier for the value to be fetched.
     * @return A {@link WebReturn} wrapping the {@link TcpGeneral} or an error message.
     */
    @GetMapping("/get/{key}")
    public ResponseEntity<WebReturn<TcpGeneral>> getValue(@PathVariable String key, @RequestHeader(value="X-Connection-Id") String connectionId) {
        String decodedKey = new String(Base64.getDecoder().decode(key.getBytes()));
        try {
            Future<WebReturn<TcpGeneral>> future = service.getValue(decodedKey, connectionId);
            return ResponseEntity.ok(future.get());
        } catch (Exception e) {
            connectionService.logout(connectionId);
            return ResponseEntity.status(500).body(WebReturn.error(e.getMessage()));
        }
    }

    /**
     * Sets a value in the in-memory database.
     *
     *
     * @return A {@link WebReturn} with success code or error details.
     */
    @PostMapping("/set")
    public ResponseEntity<?> setValue(@RequestBody SetValueRequest request, @RequestHeader(value="X-Connection-Id") String connectionId) {
        try {
            Future<WebReturn<Integer>> future = service.setValue(request.getKey(), request.getValue(), request.isForce(), connectionId);
            return ResponseEntity.ok(future.get());
        } catch (Exception e) {
            connectionService.logout(connectionId);
            return ResponseEntity.internalServerError().body(WebReturn.error(e.getMessage()));
        }
    }

    /**
     * Sends a stop command to the in-memory database to halt ongoing operations.
     *
     * @return A {@link WebReturn} indicating success or failure of the stop operation.
     */
    @PostMapping("/stop")
    public ResponseEntity<WebReturn<Integer>> stop(@RequestHeader(value="X-Connection-Id") String connectionId) {
        try {
            return ResponseEntity.ok(service.stop(connectionId));
        } catch (Exception e) {
            connectionService.logout(connectionId);
            return ResponseEntity.status(500).body(WebReturn.error(e.getMessage()));
        }
    }

    /**
     * Deletes a key from the in-memory database.
     *
     * @param key The key to delete.
     * @return A {@link WebReturn} indicating success or containing an error message.
     */
    @DeleteMapping("/delete/{key}")
    public ResponseEntity<WebReturn<Integer>> deleteKey(@PathVariable String key, @RequestHeader(value="X-Connection-Id") String connectionId) {
        try {
            return ResponseEntity.ok(service.deleteKey(key, connectionId));
        } catch (Exception e) {
            connectionService.logout(connectionId);
            return ResponseEntity.status(500).body(WebReturn.error(e.getMessage()));
        }
    }

    /**
     * Pings the server to verify communication and synchronization.
     *
     * @param packageToSend an unsigned integer data payload
     * @return a {@link WebReturn} indicating success or containing an error message
     */
    @GetMapping("/ping/{packageToSend}")
    public ResponseEntity<WebReturn<Integer>> ping(@PathVariable long packageToSend, @RequestHeader(value="X-Connection-Id") String connectionId) {
        try {
            return ResponseEntity.ok(service.ping(packageToSend, connectionId));
        } catch (Exception e) {
            connectionService.logout(connectionId);
            return ResponseEntity.status(500).body(WebReturn.error(e.getMessage()));
        }
    }

    /**
     * Updates the scope permissions of a user.
     *
     * @param username The username of the user whose scope permissions are to be updated.
     * @param key      The scope key to be added, edited, or deleted.
     * @param options  The permission options for the scope:
     */
    @PostMapping("/updateScope")
    public ResponseEntity<WebReturn<Integer>> updateScope(@RequestParam String username, @RequestParam String key, @RequestParam byte options, @RequestHeader(value="X-Connection-Id") String connectionId) {
        try {
            return ResponseEntity.ok(service.updateScope(username, key, options, connectionId));
        } catch (Exception e) {
            connectionService.logout(connectionId);
            return ResponseEntity.status(500).body(WebReturn.error(e.getMessage()));
        }
    }

    /**
     * Creates a new user with the given username and password.
     *
     * @param username The username of the new user.
     * @param password The password of the new user.
     * @param options  The options for the new user.
     * @return a {@link WebReturn} indicating success or containing an error message
     */
    @PostMapping("/createUser")
    public ResponseEntity<WebReturn<Integer>> createUser(@RequestParam String username, @RequestParam String password, @RequestParam byte options, @RequestHeader(value="X-Connection-Id") String connectionId) {
        try {
            return ResponseEntity.ok(service.createUser(username, password, options, connectionId));
        } catch (Exception e) {
            connectionService.logout(connectionId);
            return ResponseEntity.status(500).body(WebReturn.error(e.getMessage()));
        }
    }

    /**
     * Follows a reference in the in-memory database.
     *
     * @param key The key to follow.
     * @param maxJump The maximum number of jumps to follow.
     * @param isSafe Whether to follow safely.
     * @return A {@link WebReturn} indicating success or containing an error message.
     */
    @GetMapping("/follow")
    public ResponseEntity<WebReturn<TcpGeneral>> follow(@RequestParam String key, @RequestParam long maxJump, @RequestParam boolean isSafe, @RequestHeader(value="X-Connection-Id") String connectionId) {
        try {
            return ResponseEntity.ok(service.follow(key, maxJump, isSafe, connectionId));
        } catch (Exception e) {
            connectionService.logout(connectionId);
            return ResponseEntity.status(500).body(WebReturn.error(e.getMessage()));
        }
    }

    /**
     * Retrieves the permissions of a list of users.
     *
     * @param usernames Usernames to get permissions from.
     * @return A {@link WebReturn} wrapping the {@link TcpGeneral} data if successful,
     * or an error description if retrieval fails or is stopped.
     */
    @PostMapping("/getUserPermissions")
    public ResponseEntity<WebReturn<TcpGeneral>> getUserPermissions(@RequestBody TcpGeneral usernames, @RequestHeader(value="X-Connection-Id") String connectionId) {
        WebReturn<TcpGeneral> future = service.getUserPermissions(usernames, connectionId);
        try {
            return ResponseEntity.ok(future);
        } catch (Exception e) {
            connectionService.logout(connectionId);
            return ResponseEntity.status(500).body(WebReturn.error(e.getMessage()));
        }
    }
}