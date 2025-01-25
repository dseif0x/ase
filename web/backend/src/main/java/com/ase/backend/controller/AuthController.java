/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.backend.controller;

import com.ase.backend.controller.dto.LoginRequest;
import com.ase.backend.service.ConnectionService;
import com.ase.backend.service.DatabaseService;
import com.ase.exception.TCPClientException;
import com.ase.utility.WebReturn;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.io.IOException;

/**
 * REST Controller for user login that triggers the in-memory DB connection.
 */
@RestController
@RequestMapping("/api/auth")
public class AuthController {

    private final ConnectionService connectionService;
    private final DatabaseService databaseService;
    /**
     * Constructs a new AuthController.
     *
     * @param connectionService The service for managing the DB connection.
     */
    public AuthController(ConnectionService connectionService, DatabaseService databaseService) {
        this.connectionService = connectionService;
        this.databaseService = databaseService;
    }

    /**
     * Endpoint for logging in.
     *
     * @param request The login request containing connection info.
     * @return A response indicating success or failure.
     */
    @PostMapping("/login")
    public ResponseEntity<WebReturn<String>> login(@RequestBody LoginRequest request) {
        try {
            String id = connectionService.openConnection(
                request.getHost(),
                request.getPort(),
                request.getVersion(),
                request.getFlags(),
                request.getUsername(),
                request.getPassword()
            );

            return ResponseEntity.ok(WebReturn.success(id));
        } catch (Exception e) {
            return ResponseEntity.status(500).body(WebReturn.error(e.getMessage()));
        }
    }

    /**
     * Endpoint for login check.
     *
     * @return A response indicating if logged in or not.
     */
    @PostMapping("/isLoggedIn")
    public ResponseEntity<WebReturn<Boolean>> isLoggedIn(@RequestHeader(value="X-Connection-Id") String connectionId) {
        boolean loggedIn = connectionService.getClient(connectionId) != null;
        if (loggedIn) {
            boolean connectionClosed = databaseService.connectionClosed(connectionId);
            if (connectionClosed) {
                return ResponseEntity.ok(WebReturn.error("Connection to Server is closed. Please start it again."));
            }
            return ResponseEntity.ok(WebReturn.success(true));
        } else {
            return ResponseEntity.ok(WebReturn.error("Please login first"));
        }

    }

    /**
     * Logs out by removing and disconnecting the DatabaseClient.
     */
    @PostMapping("/logout")
    public ResponseEntity<WebReturn<String>> logout(@RequestHeader("X-Connection-Id") String connectionId) {
        try {
            connectionService.logout(connectionId);
            return ResponseEntity.ok(WebReturn.success("Logged out successfully"));
        } catch (Exception e) {
            return ResponseEntity.status(500).body(WebReturn.error(e.getMessage()));
        }
    }
}
