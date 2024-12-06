package com.ase.driver;

import com.ase.utility.*;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.io.*;
import java.net.Socket;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;


public class DatabaseClientCommandTest {

    private static ServerInformation serverInfo;
    private DatabaseClient databaseClient;

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

        databaseClient = new DatabaseClient(mockSocket);

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
        dataOutputStream.writeChar(0xFFFF);
        serverDataWriter.writeString("General Protocol Error.");

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.ping(packageToSend);

        assertFalse(result.isOk(), "Expected ping to fail after NAK response.");
        assertNotNull(result.getMessage(), "Expected an error message due to server error.");
        assertTrue(result.getMessage().contains("General Protocol Error."),
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
        serverDataWriter.writeString("General Protocol Error."); // Will not be read

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.ping(packageToSend);

        assertFalse(result.isOk(), "Expected ping to fail after NAK response.");
        assertNotNull(result.getMessage(), "Expected an error message due to server error.");
        assertTrue(result.getMessage().contains("Failed to read string."),
            "Expected readString to fail.");
        assertNull(result.getData(), "Expected no data on failure.");
    }

    @Test
    void testStopSuccess() throws Exception {
        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(0x03);
        serverResponseOut.write(0x01);
        serverResponseOut.write(0x01);
        serverResponseOut.write(Utility.ACK);

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.stop();

        assertTrue(result.isOk(), "Expected stop to succeed with ACK.");
        assertNull(result.getMessage(), "Expected no message on success.");
        assertNull(result.getData(), "Expected no data on success.");
    }

    @Test
    void testStopUnknownCode() throws Exception {
        byte[] unknownBytes = new byte[1024];
        for (int i = 0; i < 1024; i++) {
            unknownBytes[i] = 0x12;
        }

        setServerResponse(unknownBytes);

        WebReturn<Integer> result = databaseClient.stop();

        assertFalse(result.isOk(), "Expected stop to fail due to no ACK/NAK received.");
        assertNotNull(result.getMessage(), "Expected an error message.");
        assertTrue(result.getMessage().contains("ACK/NAK not received"),
            "Expected error message about not receiving ACK/NAK.");
        assertNull(result.getData(), "Expected no data on failure.");
    }

    @Test
    void testDisconnect() throws Exception {
        Socket mockSocket = mock(Socket.class);
        ByteArrayOutputStream clientByteOut = new ByteArrayOutputStream();
        DataOutputStream dataOutputStream = new DataOutputStream(clientByteOut);

        when(mockSocket.getOutputStream()).thenReturn(dataOutputStream);

        DatabaseClient databaseClient = new DatabaseClient(mockSocket);

        databaseClient.disconnect();

        verify(mockSocket, times(1)).close();
    }

    @Test
    void testCreateUserSuccess() throws Exception {
        String username = "testuser";
        String password = "securepassword";
        byte options = 0x01;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(Utility.ACK);

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.createUser(username, password, options);

        assertTrue(result.isOk(), "Expected createUser to succeed with ACK.");
        assertNull(result.getMessage(), "Expected no message on success.");
        assertNull(result.getData(), "Expected no data on success.");
    }

    @Test
    void testCreateUserServerError() throws Exception {
        String username = "testuser";
        String password = "securepassword";
        byte options = 0x00; // Regular user flag

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DataOutputStream dataOut = new DataOutputStream(serverResponseOut);

        serverResponseOut.write(Utility.NAK);
        dataOut.writeChar(0xFF00);
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);
        serverDataWriter.writeString("Permission denied.");

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.createUser(username, password, options);

        assertFalse(result.isOk(), "Expected createUser to fail with NAK.");
        assertNotNull(result.getMessage(), "Expected an error message.");
        assertTrue(result.getMessage().contains("Permission denied."),
            "Expected the error message to match the server response.");
        assertNotNull(result.getData(), "Expected no data on failure.");
    }

    @Test
    void testCreateUserUnexpectedResponse() throws Exception {
        String username = "testuser";
        String password = "securepassword";
        byte options = 0x00; // Regular user flag

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(0x12);

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.createUser(username, password, options);

        assertFalse(result.isOk(), "Expected createUser to fail due to unknown response.");
        assertNotNull(result.getMessage(), "Expected an error message.");
        assertTrue(result.getMessage().contains("unknown response"),
            "Expected the error message to indicate an unknown response.");
        assertNotNull(result.getData(), "Expected data to contain the response byte.");
        assertEquals(0x12, result.getData(), "Expected the response byte to match.");
    }

    @Test
    void testCreateUserTcpClientException() throws Exception {
        String username = "testuser";
        String password = "securepassword";
        byte options = 0x00;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DataOutputStream dataOutputStream = new DataOutputStream(serverResponseOut);
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);

        serverResponseOut.write(Utility.NAK);
        dataOutputStream.write(0xFF00); // Will not write as unsigned short so readString will fail
        serverDataWriter.writeString("Permission denied"); // Will not be read

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.createUser(username, password, options);

        assertFalse(result.isOk(), "Expected ping to fail after NAK response.");
        assertNotNull(result.getMessage(), "Expected an error message due to server error.");
        assertTrue(result.getMessage().contains("Failed to read string."),
            "Expected readString to fail.");
        assertNull(result.getData(), "Expected no data on failure.");
    }

    @Test
    void testUpdateScopeSuccess() throws Exception {
        String username = "testuser";
        String key = "scopeKey";
        byte options = 0x03; // Read and write permissions

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(Utility.ACK);

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.updateScope(username, key, options);

        assertTrue(result.isOk(), "Expected updateScope to succeed with ACK.");
        assertNull(result.getMessage(), "Expected no message on success.");
        assertNull(result.getData(), "Expected no data on success.");
    }

    @Test
    void testUpdateScopeServerError() throws Exception {
        String username = "testuser";
        String key = "scopeKey";
        byte options = 0x01;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DataOutputStream dataOut = new DataOutputStream(serverResponseOut);

        serverResponseOut.write(Utility.NAK);
        dataOut.writeChar(0xFF00);
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);
        serverDataWriter.writeString("Permission denied.");

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.updateScope(username, key, options);

        assertFalse(result.isOk(), "Expected updateScope to fail with NAK.");
        assertNotNull(result.getMessage(), "Expected an error message.");
        assertTrue(result.getMessage().contains("Permission denied."),
            "Expected the error message to match the server response.");
        assertNotNull(result.getData(), "Expected no data on failure.");
    }

    @Test
    void testUpdateScopeUnexpectedResponse() throws Exception {
        String username = "testuser";
        String key = "scopeKey";
        byte options = 0x02; // Write permission

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(0x12); // Unexpected response

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.updateScope(username, key, options);

        assertFalse(result.isOk(), "Expected updateScope to fail due to unknown response.");
        assertNotNull(result.getMessage(), "Expected an error message.");
        assertTrue(result.getMessage().contains("unknown response"),
            "Expected the error message to indicate an unknown response.");
        assertNotNull(result.getData(), "Expected data to contain the response byte.");
        assertEquals(0x12, result.getData(), "Expected the response byte to match.");
    }

    @Test
    void testUpdateScopeTcpClientException() throws Exception {
        String username = "testuser";
        String password = "securepassword";
        byte options = 0x00;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DataOutputStream dataOutputStream = new DataOutputStream(serverResponseOut);
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);

        serverResponseOut.write(Utility.NAK);
        dataOutputStream.write(0xFF00); // Will not write as unsigned short so readString will fail
        serverDataWriter.writeString("Permission denied"); // Will not be read

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.updateScope(username, password, options);

        assertFalse(result.isOk(), "Expected ping to fail after NAK response.");
        assertNotNull(result.getMessage(), "Expected an error message due to server error.");
        assertTrue(result.getMessage().contains("Failed to read string."),
            "Expected readString to fail.");
        assertNull(result.getData(), "Expected no data on failure.");
    }
}
