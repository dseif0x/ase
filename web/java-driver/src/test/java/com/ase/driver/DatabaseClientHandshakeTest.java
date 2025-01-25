/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.driver;

import com.ase.exception.HandshakeException;
import com.ase.exception.ServerErrorException;
import com.ase.exception.TCPClientException;
import com.ase.utility.*;
import com.ase.utility.tcp.TcpErrorCode;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.mockito.MockedStatic;
import org.mockito.Mockito;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;

/**
 * Test suite for {@link DatabaseClient} connect and login operations.
 * <p>
 * Uses JUnit 5 and mock sockets to test various server responses.
 * </p>
 */
class DatabaseClientHandshakeTest {

    /**
     * Mocked socket for testing.
     */
    private Socket mockSocket;
    /**
     * Output stream for writing to the socket.
     */
    private ByteArrayOutputStream byteOut;
    /**
     * Database client instance for testing.
     */
    private DatabaseClient databaseClient;
    /**
     * Mocked Server information for testing.
     */
    private static ServerInformation serverInfo;

    /**
     * Initializes server information with default sizes for testing.
     */
    @BeforeAll
    static void setUp() {
        serverInfo = new ServerInformation();
        serverInfo.setBooleanSize(1);
        serverInfo.setIntSize(4);
        serverInfo.setCharSize(2);
        serverInfo.setDoubleSize(8);
        serverInfo.setSemaphoreSize(4);
        serverInfo.setStringSize(255);
        serverInfo.setArraySize(256);
    }

    /**
     * Configures a mocked server response for testing.
     *
     * @param responseBytes The byte array simulating the server's response.
     * @throws IOException If an error occurs during setup.
     */
    private void setServerResponse(byte[] responseBytes) throws IOException {
        mockSocket = mock(Socket.class);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(responseBytes);

        DataInputStream dataInputStream = new DataInputStream(byteIn);
        DynamicDataReader clientDataReader = new DynamicDataReader(serverInfo, dataInputStream);

        byteOut = new ByteArrayOutputStream();
        DataOutputStream dataOutputStream = new DataOutputStream(byteOut);
        DynamicDataWriter clientDataWriter = new DynamicDataWriter(serverInfo, byteOut);

        when(mockSocket.getOutputStream()).thenReturn(dataOutputStream);
        when(mockSocket.getInputStream()).thenReturn(dataInputStream);

        databaseClient = new DatabaseClient(mockSocket);

        databaseClient.setDataWriter(clientDataWriter);
        databaseClient.setDataReader(clientDataReader);
    }

    /**
     * Configures a mocked server error response for testing.
     *
     * @param errorCode The error code to simulate.
     * @throws Exception If an error occurs during setup.
     */
    private void setServerErrorResponse(int errorCode) throws Exception {
        mockSocket = mock(Socket.class);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(new byte[]{
            Utility.NAK,
            (byte) ((errorCode >> 8) & 0xFF),
            (byte) (errorCode & 0xFF)
        });

        DataInputStream dataInputStream = new DataInputStream(byteIn);
        DynamicDataReader clientDataReader = new DynamicDataReader(serverInfo, dataInputStream);

        byteOut = new ByteArrayOutputStream();
        DataOutputStream dataOutputStream = new DataOutputStream(byteOut);
        DynamicDataWriter clientDataWriter = new DynamicDataWriter(serverInfo, byteOut);

        when(mockSocket.getOutputStream()).thenReturn(dataOutputStream);
        when(mockSocket.getInputStream()).thenReturn(dataInputStream);

        databaseClient = new DatabaseClient(mockSocket);

        databaseClient.setDataWriter(clientDataWriter);
        databaseClient.setDataReader(clientDataReader);
    }

    /**
     * Tests successful server connection with valid handshake response.
     *
     * @throws IOException        If an I/O error occurs.
     * @throws TCPClientException If the client encounters an error.
     */
    @Test
    void testSuccessfulConnect() throws IOException, TCPClientException {
        setServerResponse(new byte[]{
            Utility.ACK,
        });

        int version = 1;
        byte flags = 0x01;

        try (MockedStatic<ServerInformation> mockedServerInfo = Mockito.mockStatic(ServerInformation.class)) {
            ServerInformation mockInfo = mock(ServerInformation.class);
            when(mockInfo.validate()).thenReturn(true);
            mockedServerInfo.when(() -> ServerInformation.readServerInformation(any())).thenReturn(mockInfo);

            databaseClient.connect(version, flags);

            byte[] sentPacket = byteOut.toByteArray();
            byte[] expectedPacket = new byte[]{
                0,
                1,
                flags,
                Utility.ACK
            };
            assertArrayEquals(expectedPacket, sentPacket);
        }
    }

    /**
     * Tests handshake failure due to a general protocol error.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testGeneralProtocolError() throws Exception {
        int errorCode = TcpErrorCode.UNKNOWN_ERROR.getCode();
        setServerErrorResponse(errorCode);

        int version = 1;
        byte flags = 0x01;

        ServerErrorException exception = assertThrows(ServerErrorException.class, () -> {
            databaseClient.connect(version, flags);
        });

        assertEquals(errorCode, exception.getErrorCode());
    }

    /**
     * Tests handshake failure due to permission denied by the server.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testPermissionDeniedError() throws Exception {
        int errorCode = TcpErrorCode.PERMISSION_DENIED_CONNECTION.getCode();
        setServerErrorResponse(errorCode);

        int version = 1;
        byte flags = 0x01;

        ServerErrorException exception = assertThrows(ServerErrorException.class, () -> {
            databaseClient.connect(version, flags);
        });

        assertEquals(errorCode, exception.getErrorCode());
    }

    /**
     * Tests handshake failure due to an invalid version error.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testInvalidVersionError() throws Exception {
        int errorCode = TcpErrorCode.INVALID_VERSION.getCode();
        setServerErrorResponse(errorCode);

        int version = 1;
        byte flags = 0x01;

        ServerErrorException exception = assertThrows(ServerErrorException.class, () -> {
            databaseClient.connect(version, flags);
        });

        assertEquals(errorCode, exception.getErrorCode());
    }

    /**
     * Tests handshake failure when server information validation fails.
     *
     * @throws IOException If an error occurs during testing.
     */
    @Test
    void testValidateServerInfoFailed() throws IOException {
        setServerResponse(new byte[]{
            Utility.ACK,
        });

        int version = 1;
        byte flags = 0x01;

        try (MockedStatic<ServerInformation> mockedServerInfo = Mockito.mockStatic(ServerInformation.class)) {
            ServerInformation mockInfo = mock(ServerInformation.class);
            when(mockInfo.validate()).thenReturn(false);
            mockedServerInfo.when(() -> ServerInformation.readServerInformation(any())).thenReturn(mockInfo);

            HandshakeException exception = assertThrows(HandshakeException.class, () -> {
                databaseClient.connect(version, flags);
            });
            assertEquals("Handshake failed. Validation failed for server information. Sizes of server are too large.", exception.getMessage());

            byte[] sentPacket = byteOut.toByteArray();
            assertEquals(Utility.NAK, sentPacket[sentPacket.length - 1]);
        }
    }

    /**
     * Tests successful user login.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testSuccessLogin() throws Exception {
        setServerResponse(new byte[]{
            Utility.ACK,
        });

        databaseClient.login("username", "password");
    }

    /**
     * Tests login failure due to an invalid username.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testFailedLogin() throws Exception {
        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DataOutputStream dataOut = new DataOutputStream(serverResponseOut);

        serverResponseOut.write(Utility.NAK);
        dataOut.writeChar(0xFF02);

        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);

        serverDataWriter.writeString("There is no user with that username.");
        setServerResponse(serverResponseOut.toByteArray());

        ServerErrorException exception = assertThrows(ServerErrorException.class, () -> {
            databaseClient.login("username", "password");
        });

        assertEquals(0xFF02, exception.getErrorCode());
        assertEquals("Server Error 65282: There is no user with that username.", exception.getMessage());
    }

    /**
     * Tests login failure due to an unknown server response.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testLoginUnknownResponse() throws Exception {

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(0x12);

        setServerResponse(serverResponseOut.toByteArray());

        assertThrows(TCPClientException.class, () -> {
            databaseClient.login("username", "password");
        });
    }

}
