package com.ase.utility;

/**
 * A generic class representing the response of a web request.
 *
 * @param <T> the type of the data returned
 */
public class WebReturn<T> {

    /**
     * Indicates whether the request was successful.
     */
    private boolean ok;

    /**
     * Contains an error or informational message.
     */
    private String message;

    /**
     * Holds any type of returned data.
     */
    private T data;

    /**
     * Default constructor.
     * Initializes with ok=false, no message, and no data.
     */
    public WebReturn() {
        this.ok = false;
        this.message = null;
        this.data = null;
    }

    /**
     * Parameterized constructor.
     *
     * @param ok      indicates if the request was successful
     * @param message an error or informational message
     * @param data    the data returned from the request
     */
    public WebReturn(boolean ok, String message, T data) {
        this.ok = ok;
        this.message = message;
        this.data = data;
    }

    /**
     * Static factory method for successful responses.
     *
     * @return a WebReturn instance representing success
     */
    public static <T> WebReturn<T> success() {
        return new WebReturn<>(true, null, null);
    }

    /**
     * Static factory method for successful responses.
     *
     * @param data the data to return
     * @param <T>  the type of the data
     * @return a WebReturn instance representing success
     */
    public static <T> WebReturn<T> success(T data) {
        return new WebReturn<>(true, null, data);
    }

    /**
     * Static factory method for successful responses with a message.
     *
     * @param data    the data to return
     * @param message an informational message
     * @param <T>     the type of the data
     * @return a WebReturn instance representing success with a message
     */
    public static <T> WebReturn<T> success(String message, T data) {
        return new WebReturn<>(true, message, data);
    }

    /**
     * Static factory method for error responses.
     *
     * @param message the error message
     * @param <T>     the type of the data (typically null in error cases)
     * @return a WebReturn instance representing an error
     */
    public static <T> WebReturn<T> error(String message) {
        return new WebReturn<>(false, message, null);
    }

    /**
     * Static factory method for error responses with data.
     *
     * @param message the error message
     * @param data    additional data related to the error
     * @param <T>     the type of the data
     * @return a WebReturn instance representing an error with data
     */
    public static <T> WebReturn<T> error(String message, T data) {
        return new WebReturn<>(false, message, data);
    }

    // Getters and Setters

    /**
     * @return the ok status of the response
     */
    public boolean isOk() {
        return ok;
    }

    /**
     * @return the message of the response
     */
    public String getMessage() {
        return message;
    }

    /**
     * @return the data of the response
     */
    public T getData() {
        return data;
    }

    /**
     * Returns a string representation of the WebReturn object.
     *
     * @return a string containing the ok status, message, and data
     */
    @Override
    public String toString() {
        return "WebReturn{" +
            "ok=" + ok +
            ", message='" + message + '\'' +
            ", data=" + data +
            '}';
    }
}