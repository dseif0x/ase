package com.ase.backend.controller;

import com.ase.backend.controller.dto.LoginRequest;
import com.ase.backend.service.ConnectionService;
import com.ase.exception.TCPClientException;
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

    public AuthController(ConnectionService connectionService) {
        this.connectionService = connectionService;
    }

    /**
     * Endpoint for logging in (i.e., creating and connecting the DatabaseClient).
     */
    @PostMapping("/login")
    public ResponseEntity<String> login(@RequestBody LoginRequest request) {
        try {
            connectionService.createClient(
                request.getHost(),
                request.getPort(),
                request.getVersion(),
                request.getFlags(),
                request.getUsername(),
                request.getPassword()
            );
            return ResponseEntity.ok("Login successful. DB client created.");
        } catch (IOException | TCPClientException e) {
            return ResponseEntity.status(500).body("Login failed: " + e.getMessage());
        }
    }
}
