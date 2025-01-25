/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.exception;

public class TCPClientReadException extends TCPClientException {
    public TCPClientReadException(String message) {
        super("Read failed. " + message);
    }

    public TCPClientReadException(String message, Throwable cause) {
        super("Read failed. " + message, cause);
    }
}