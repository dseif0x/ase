package com.ase.backend.controller;

import com.ase.backend.controller.dto.LoginRequest;
import com.ase.backend.controller.dto.LoginResponse;
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
@CrossOrigin(origins = "http://localhost:3000")
public class AuthController {

    private final ConnectionService connectionService;

    /**
     * Constructs a new AuthController.
     *
     * @param connectionService The service for managing the DB connection.
     */
    public AuthController(ConnectionService connectionService) {
        this.connectionService = connectionService;
    }

    /**
     * Endpoint for logging in.
     *
     * @param request The login request containing connection info.
     * @return A response indicating success or failure.
     */
    @PostMapping("/login")
    public ResponseEntity<LoginResponse> login(@RequestBody LoginRequest request) {
        try {
            String id = connectionService.openConnection(
                request.getHost(),
                request.getPort(),
                request.getVersion(),
                request.getFlags(),
                request.getUsername(),
                request.getPassword()
            );

            LoginResponse response = new LoginResponse(id, null);
            return ResponseEntity.ok(response);
        } catch (IOException | TCPClientException e) {
            return ResponseEntity.status(500).body(new LoginResponse(null, "Login failed: " + e.getMessage()));
        }
    }

    /**
     * Logs out by removing and disconnecting the DatabaseClient.
     */
    @PostMapping("/logout")
    public ResponseEntity<String> logout(@RequestHeader("X-Connection-Id") String connectionId) {
        connectionService.logout(connectionId);
        return ResponseEntity.ok("Logged out successfully");
    }
}
