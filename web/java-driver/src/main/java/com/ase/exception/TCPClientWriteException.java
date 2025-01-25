/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.exception;

public class TCPClientWriteException extends TCPClientException {
    public TCPClientWriteException(String message) {
        super("Write failed. " + message);
    }

    public TCPClientWriteException(String message, Throwable cause) {
        super("Write failed. " + message, cause);
    }
}
