package com.ase.backend.controller.dto;

public class LoginResponse {
    private String id;
    private String errorMessage;

    /**
     * Constructs a new LoginResponse.
     */
    public LoginResponse(String id, String errorMessage) {
        this.id = id;
        this.errorMessage = errorMessage;
    }

    /**
     * Gets the id.
     *
     * @return The id.
     */
    public String getId() { return id; }

    /**
     * Gets the error message.
     *
     * @return The error message.
     */
    public String getErrorMessage() { return errorMessage; }

    /**
     * Sets the id.
     *
     * @param id The id.
     */
    public void setId(String id) { this.id = id; }

    /**
     * Sets the error message.
     *
     * @param errorMessage The error message.
     */
    public void setErrorMessage(String errorMessage) { this.errorMessage = errorMessage; }
}
