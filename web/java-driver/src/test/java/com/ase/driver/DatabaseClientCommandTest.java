package com.ase.driver;

import com.ase.utility.*;
import com.ase.utility.tcp.SimpleType;
import com.ase.utility.tcp.TcpGeneral;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.io.*;
import java.net.Socket;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;

/**
 * Test suite for the {@link DatabaseClient}, validating its methods and behavior.
 * <p>
 * Uses JUnit 5 and mocks server responses to simulate various scenarios.
 * </p>
 */
public class DatabaseClientCommandTest {

    /** The mocked server information. */
    private static ServerInformation serverInfo;
    /** The database client to test. */
    private DatabaseClient databaseClient;

    /**
     * Sets up server information with default size values for tests.
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
     * @param responseBytes The server response to simulate.
     * @throws Exception If an error occurs during setup.
     */
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

    /**
     * Test the ping method with a successful response.
     *
     * @throws Exception If an error occurs during the test.
     */
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

    /**
     * Test the ping method with a failed response.
     *
     * @throws Exception If an error occurs during the test.
     */
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

    /**
     * Test the ping method with a server response of NAK.
     *
     * @throws Exception If an error occurs during the test.
     */
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

    /**
     * Test the ping method with an unknown server response code.
     *
     * @throws Exception If an error occurs during the test.
     */
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

    /**
     * Test the ping method with a TCPClientException during reading.
     *
     * @throws Exception If an error occurs during the test.
     */
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

    /**
     * Test the start method with a successful response.
     *
     * @throws Exception If an error occurs during the test.
     */
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

    /**
     * Test the stop method with no response code.
     *
     * @throws Exception If an error occurs during the test.
     */
    @Test
    void testStopNoResponseCode() throws Exception {
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

    /**
     * Test the disconnect method.
     *
     * @throws Exception If an error occurs during the test.
     */
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

    /**
     * Test the createUser method with a successful response.
     *
     * @throws Exception If an error occurs during the test.
     */
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

    /**
     * Test the createUser method with a server response of NAK and server error response.
     *
     * @throws Exception If an error occurs during the test.
     */
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

    /**
     * Test the createUser method with an unexpected server response.
     *
     * @throws Exception If an error occurs during the test.
     */
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

    /**
     * Test the createUser method with a TCPClientException during reading.
     *
     * @throws Exception If an error occurs during the test.
     */
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

        assertFalse(result.isOk(), "Expected createUser to fail after NAK response.");
        assertNotNull(result.getMessage(), "Expected an error message due to server error.");
        assertTrue(result.getMessage().contains("Failed to read string."),
            "Expected readString to fail.");
        assertNull(result.getData(), "Expected no data on failure.");
    }

    /**
     * Tests successful scope update for a user.
     *
     * @throws Exception If an error occurs during testing.
     */
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

    /**
     * Test the updateScope method with a server response of NAK and server error response.
     *
     * @throws Exception If an error occurs during testing.
     */
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

    /**
     * Test the updateScope method with an unexpected server response.
     *
     * @throws Exception If an error occurs during testing.
     */
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

    /**
     * Test the updateScope method with a TCPClientException during reading.
     *
     * @throws Exception If an error occurs during testing.
     */
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

        assertFalse(result.isOk(), "Expected updateScope to fail after NAK response.");
        assertNotNull(result.getMessage(), "Expected an error message due to server error.");
        assertTrue(result.getMessage().contains("Failed to read string."),
            "Expected readString to fail.");
        assertNull(result.getData(), "Expected no data on failure.");
    }

    /**
     * Tests set failure due to null key.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testSetNullKey() throws Exception {
        TcpGeneral value = mock(TcpGeneral.class);

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(Utility.ACK);
        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.set(null, value, false);

        // Verify error
        assertFalse(result.isOk(), "Expected set to fail due to null key.");
        assertEquals("Key must not be null or empty.", result.getMessage(),
            "Expected error message for null key.");
        assertNull(result.getData(), "Expected no data on failure.");
    }

    /**
     * Tests set success with a simple value.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testSetSuccess() throws Exception {
        String key = "testKey";
        TcpGeneral value = TcpGeneral.ofSimple(SimpleType.STRING, "testValue");
        boolean force = false;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(Utility.ACK);
        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.set(key, value, force);

        assertTrue(result.isOk(), "Expected set to succeed with ACK.");
        assertNull(result.getMessage(), "Expected no message on success.");
        assertNull(result.getData(), "Expected no data on success.");
    }


    /**
     * Tests set success with a simple value and force flag.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testSetForceSuccess() throws Exception {
        String key = "testKey";
        TcpGeneral value = TcpGeneral.ofSimple(SimpleType.STRING, "testValue");
        boolean force = true;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(Utility.ACK);
        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.set(key, value, force);

        assertTrue(result.isOk(), "Expected set to succeed with ACK.");
        assertNull(result.getMessage(), "Expected no message on success.");
        assertNull(result.getData(), "Expected no data on success.");
    }

    /**
     * Tests set failure due to null value.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testSetNullValue() throws Exception {
        String key = "testKey";
        boolean force = false;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(Utility.ACK);
        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.set(key, null, force);

        // Verify error
        assertFalse(result.isOk(), "Expected set to fail due to null TcpGeneral value.");
        assertEquals("Value must not be null.", result.getMessage(),
            "Expected error message for null TcpGeneral value.");
        assertNull(result.getData(), "Expected no data on failure.");
    }

    /**
     * Tests set failure due to null value type.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testSetServerError() throws Exception {
        String key = "testKey";
        TcpGeneral value = TcpGeneral.ofSimple(SimpleType.STRING, "testValue");
        boolean force = false;

        // Simulate server responding with NAK and an error message
        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DataOutputStream dataOut = new DataOutputStream(serverResponseOut);
        DynamicDataWriter clientDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);

        serverResponseOut.write(Utility.NAK); // NAK response
        dataOut.writeShort(0x0100); // Example error code
        clientDataWriter.writeString("Invalid key.");

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.set(key, value, force);

        // Verify error
        assertFalse(result.isOk(), "Expected set to fail with NAK.");
        assertNotNull(result.getMessage(), "Expected an error message.");
        assertTrue(result.getMessage().contains("Invalid key."),
            "Expected the error message to match the server response.");
        assertNotNull(result.getData(), "Expected no data on failure.");
    }

    /**
     * Tests set failure due to unknown response.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testSetUnexpectedResponse() throws Exception {
        String key = "testKey";
        TcpGeneral value = TcpGeneral.ofSimple(SimpleType.INT, 100);
        boolean force = false;

        // Simulate server responding with an unknown byte
        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(0x12); // Unknown response

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.set(key, value, force);

        // Verify error
        assertFalse(result.isOk(), "Expected set to fail due to unknown response.");
        assertNotNull(result.getMessage(), "Expected an error message.");
        assertTrue(result.getMessage().contains("unknown response"),
            "Expected the error message to indicate unknown response.");
        assertNotNull(result.getData(), "Expected data to contain the response byte.");
        assertEquals(0x12, result.getData(), "Expected the response byte to match.");
    }

    /**
     * Tests set failure due to TCPClientException during reading.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testSetTcpClientException() throws Exception {
        String key = "testKey";
        TcpGeneral value = TcpGeneral.ofSimple(SimpleType.INT, 100);
        boolean force = false;

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DataOutputStream dataOutputStream = new DataOutputStream(serverResponseOut);
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);

        serverResponseOut.write(Utility.NAK);
        dataOutputStream.write(0xFF00); // Will not write as unsigned short so readString will fail
        serverDataWriter.writeString("Permission denied"); // Will not be read

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<Integer> result = databaseClient.set(key, value, force);

        assertFalse(result.isOk(), "Expected set to fail after NAK response.");
        assertNotNull(result.getMessage(), "Expected an error message due to server error.");
        assertTrue(result.getMessage().contains("Failed to read string."),
            "Expected readString to fail.");
        assertNull(result.getData(), "Expected no data on failure.");
    }

    /**
     * Tests successful get for a key.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testGetSuccess() throws Exception {
        String key = "testKey";
        TcpGeneral expectedValue = TcpGeneral.ofSimple(SimpleType.STRING, "testValue");

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(Utility.ACK);
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);
        serverDataWriter.writeGeneral(expectedValue);

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<TcpGeneral> result = databaseClient.get(key);

        assertTrue(result.isOk(), "Expected get to succeed with ACK.");
        assertNull(result.getMessage(), "Expected no message on success.");
        assertNotNull(result.getData(), "Expected data on success.");
        assertEquals(expectedValue, result.getData(), "Expected the TcpGeneral value to match.");
    }

    /**
     * Tests get failure due to null key.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testGetNullKey() throws Exception {
        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(Utility.ACK);
        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<TcpGeneral> result = databaseClient.get(null);

        // Verify error
        assertFalse(result.isOk(), "Expected get to fail due to null key.");
        assertEquals("Key must not be null or empty.", result.getMessage(),
            "Expected error message for null key.");
        assertNull(result.getData(), "Expected no data on failure.");
    }

    /**
     * Tests get failure due to server error.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testGetServerError() throws Exception {
        String key = "testKey";

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DataOutputStream dataOut = new DataOutputStream(serverResponseOut);
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);

        serverResponseOut.write(Utility.NAK);
        dataOut.writeShort(0xFF00);
        serverDataWriter.writeString("Permission denied.");

        setServerResponse(serverResponseOut.toByteArray());

        WebReturn<TcpGeneral> result = databaseClient.get(key);

        assertFalse(result.isOk(), "Expected get to fail with NAK.");
        assertNotNull(result.getMessage(), "Expected an error message.");
        assertTrue(result.getMessage().contains("Permission denied."),
            "Expected the error message to indicate NAK response.");
        assertNotNull(result.getData(), "Expected data to contain the error code.");
        assertEquals(0xFF00, result.getData().getValue(), "Expected the error code to match.");
    }

    /**
     * Tests get failure due to unknown response.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testGetUnexpectedResponse() throws Exception {
        String key = "testKey";

        // Simulate server responding with an unknown byte
        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        serverResponseOut.write(0x12);

        setServerResponse(serverResponseOut.toByteArray());

        // Call the get method
        WebReturn<TcpGeneral> result = databaseClient.get(key);

        // Verify error
        assertFalse(result.isOk(), "Expected get to fail due to unknown response.");
        assertNotNull(result.getMessage(), "Expected an error message.");
        assertTrue(result.getMessage().contains("unknown response"),
            "Expected the error message to indicate unknown response.");
        assertNull(result.getData(), "Expected no data on failure.");
    }

    /**
     * Tests get failure due to TCPClientException during reading.
     *
     * @throws Exception If an error occurs during testing.
     */
    @Test
    void testGetTCPClientException() throws Exception {
        String key = "testKey";

        ByteArrayOutputStream serverResponseOut = new ByteArrayOutputStream();
        DataOutputStream serverDataOut = new DataOutputStream(serverResponseOut);
        DynamicDataWriter serverDataWriter = new DynamicDataWriter(serverInfo, serverResponseOut);

        serverResponseOut.write(Utility.NAK);
        serverDataOut.write(0xFF00);
        serverDataWriter.writeString("Permission denied");

        setServerResponse(serverResponseOut.toByteArray());

        // Call the get method
        WebReturn<TcpGeneral> result = databaseClient.get(key);

        // Verify error
        assertFalse(result.isOk(), "Expected get to fail due to TCPClientException.");
        assertEquals("Read failed. Failed to read string.", result.getMessage(),
            "Expected the message from the thrown TCPClientException.");
        assertNull(result.getData(), "Expected no data on failure.");
    }
}
