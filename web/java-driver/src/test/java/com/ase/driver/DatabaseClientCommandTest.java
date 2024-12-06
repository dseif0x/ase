package com.ase.driver;

import com.ase.exception.TCPClientException;
import com.ase.utility.*;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;

import java.io.*;
import java.net.Socket;
import java.lang.reflect.Field;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;


public class DatabaseClientCommandTest {

    private static ServerInformation serverInfo;
    private DatabaseClient databaseClient;

    @BeforeAll
   static void setUp() throws Exception {
        serverInfo = new ServerInformation();
        serverInfo.setBooleanSize(1);
        serverInfo.setIntSize(4);
        serverInfo.setCharSize(2);
        serverInfo.setDoubleSize(8);
        serverInfo.setSemaphoreSize(4);
        serverInfo.setStringSize(255);
        serverInfo.setArraySize(256);
    }

    private void setServerResponse(byte[] responseBytes) throws Exception {
        Socket mockSocket = mock(Socket.class);

        ByteArrayInputStream clientByteIn = new ByteArrayInputStream(responseBytes);
        DataInputStream dataInputStream = new DataInputStream(clientByteIn);
        DynamicDataReader clientDataReader = new DynamicDataReader(serverInfo, dataInputStream);

        ByteArrayOutputStream clientByteOut = new ByteArrayOutputStream();
        DataOutputStream dataOutputStream = new DataOutputStream(clientByteOut);
        DynamicDataWriter clientDataWriter = new DynamicDataWriter(serverInfo, clientByteOut);

        when(mockSocket.getOutputStream()).thenReturn(dataOutputStream);
        when(mockSocket.getInputStream()).thenReturn(dataInputStream);

        // Initialize the DatabaseClient
        databaseClient = new DatabaseClient(mockSocket);

        // Instead of reflection, use setters now
        databaseClient.setDataWriter(clientDataWriter);
        databaseClient.setDataReader(clientDataReader);
    }

    @Test
    void testPingSuccess() throws Exception {
        long packageToSend = 123456789L;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);

        serverResponseOut.write(Utility.ACK);
        serverDataWriter.writeUnsignedInt(packageToSend);

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.ping(packageToSend);

        assertTrue(result.isOk());
        assertNull(result.getMessage());
        assertNull(result.getData());
    }

    @Test
    void testPingFailure() throws Exception {
        long packageToSend = 123456789L;
        long mismatchPackage = 987654321L;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);

        serverResponseOut.write(Utility.ACK);
        serverDataWriter.writeUnsignedInt(mismatchPackage);

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.ping(packageToSend);

        assertFalse(result.isOk(), "Expected ping to fail because the package doesn't match.");
        assertNotNull(result.getMessage(), "Expected an error message to be returned.");
        assertTrue(result.getMessage().contains("Server responded with " + mismatchPackage),
            "Expected the error message to mention the mismatching server response.");
        assertNull(result.getData(), "Expected no data on failure.");
    }

    @Test
    void testPingServerRespondsNAK() throws Exception {
        long packageToSend = 123456789L;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DataOutputStream dataOutputStream = new DataOutputStream(serverResponseOut);
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);

        serverResponseOut.write(Utility.NAK);
        dataOutputStream.writeChar(0xFF01);
        serverDataWriter.writeString("Invalid client version.");

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.ping(packageToSend);

        assertFalse(result.isOk(), "Expected ping to fail after NAK response.");
        assertNotNull(result.getMessage(), "Expected an error message due to server error.");
        assertTrue(result.getMessage().contains("Invalid client version."),
            "Expected the error message read from the server.");
        assertNotNull(result.getData(), "Expected no data on failure.");
    }

    @Test
    void testPingServerRespondsUnknownCode() throws Exception {
        long packageToSend = 123456789L;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(0x12);
        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.ping(packageToSend);

        assertFalse(result.isOk(), "Expected ping to fail due to unknown response code.");
        assertNotNull(result.getMessage(), "Expected an error message.");
        assertTrue(result.getMessage().contains("unknown response"),
            "Expected the error message to indicate unknown response.");
    }

    @Test
    void testPingTcpClientException() throws Exception {
        long packageToSend = 123456789L;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DataOutputStream dataOutputStream = new DataOutputStream(serverResponseOut);
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);

        serverResponseOut.write(Utility.NAK);
        dataOutputStream.write(0xFF01); // Will not write as unsigned short so readString will fail
        serverDataWriter.writeString("Invalid client version."); // Will not be read

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.ping(packageToSend);

        assertFalse(result.isOk(), "Expected ping to fail after NAK response.");
        assertNotNull(result.getMessage(), "Expected an error message due to server error.");
        assertTrue(result.getMessage().contains("Failed to read string."),
            "Expected readString to fail.");
        assertNull(result.getData(), "Expected no data on failure.");
    }
}
