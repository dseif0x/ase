package com.ase.driver;

import com.ase.exception.HandshakeException;
import com.ase.exception.ServerErrorException;
import com.ase.exception.TCPClientException;
import com.ase.utility.ServerInformation;
import com.ase.utility.Utility;
import com.ase.utility.tcp.TcpErrorCode;
import org.junit.jupiter.api.BeforeEach;
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

class DatabaseClientHandshakeTest {

    private Socket mockSocket;
    private ByteArrayInputStream byteIn;
    private ByteArrayOutputStream byteOut;
    private DataInputStream dataInputStream;
    private DataOutputStream dataOutputStream;

    private DatabaseClient databaseClient;

    private void setServerResponse(byte[] responseBytes) throws IOException {
        ByteArrayInputStream byteIn = new ByteArrayInputStream(responseBytes);
        DataInputStream dataInputStream = new DataInputStream(byteIn);
        mockSocket = mock(Socket.class);

        byteOut = new ByteArrayOutputStream();
        dataOutputStream = new DataOutputStream(byteOut);

        when(mockSocket.getOutputStream()).thenReturn(dataOutputStream);
        when(mockSocket.getInputStream()).thenReturn(dataInputStream);

        databaseClient = new DatabaseClient(mockSocket);
    }

    private void setServerErrorResponse(int errorCode) throws IOException {
        ByteArrayInputStream byteIn = new ByteArrayInputStream(new byte[]{
            Utility.NAK,
            (byte)((errorCode >> 8) & 0xFF),
            (byte)(errorCode & 0xFF)
        });

        DataInputStream dataInputStream = new DataInputStream(byteIn);
        mockSocket = mock(Socket.class);

        byteOut = new ByteArrayOutputStream();
        dataOutputStream = new DataOutputStream(byteOut);

        when(mockSocket.getOutputStream()).thenReturn(dataOutputStream);
        when(mockSocket.getInputStream()).thenReturn(dataInputStream);

        databaseClient = new DatabaseClient(mockSocket);
    }

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

    @Test
    void testGeneralProtocolError() throws IOException {
        int errorCode = TcpErrorCode.UNKNOWN_ERROR.getCode();
        setServerErrorResponse(errorCode);

        int version = 1;
        byte flags = 0x01;

        ServerErrorException exception = assertThrows(ServerErrorException.class, () -> {
            databaseClient.connect(version, flags);
        });

        assertEquals(errorCode, exception.getErrorCode());
    }

    @Test
    void testPermissionDeniedError() throws IOException {
        int errorCode = TcpErrorCode.PERMISSION_DENIED_CONNECTION.getCode();
        setServerErrorResponse(errorCode);

        int version = 1;
        byte flags = 0x01;

        ServerErrorException exception = assertThrows(ServerErrorException.class, () -> {
            databaseClient.connect(version, flags);
        });

        assertEquals(errorCode, exception.getErrorCode());
    }

    @Test
    void testInvalidVersionError() throws IOException {
        int errorCode = TcpErrorCode.INVALID_VERSION.getCode();
        setServerErrorResponse(errorCode);

        int version = 1;
        byte flags = 0x01;

        ServerErrorException exception = assertThrows(ServerErrorException.class, () -> {
            databaseClient.connect(version, flags);
        });

        assertEquals(errorCode, exception.getErrorCode());
    }

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

}
