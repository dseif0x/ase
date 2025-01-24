package com.ase.backend.controller;

import com.ase.backend.controller.dto.LoginRequest;
import com.ase.backend.service.ConnectionService;
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
    public ResponseEntity<Boolean> isLoggedIn(@RequestHeader(value="X-Connection-Id") String connectionId) {
        return ResponseEntity.ok(connectionService.getClient(connectionId) != null);
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
