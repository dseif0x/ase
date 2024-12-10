package com.ase.utility;

import com.ase.exception.ServerErrorException;
import com.ase.exception.TCPClientException;
import com.ase.exception.TCPClientReadException;
import com.ase.exception.TCPClientWriteException;
import com.ase.utility.tcp.SimpleType;
import com.ase.utility.tcp.TcpGeneral;
import com.ase.utility.tcp.TcpGeneralType;
import com.ase.utility.tcp.TypeFlag;
import org.apache.commons.lang3.tuple.Pair;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Optional;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Test suite for {@link DynamicDataReader} and {@link DynamicDataWriter}, validating
 * data reading and writing operations based on server configurations.
 * <p>
 * Uses JUnit 5 for assertions and mock data to simulate server interactions.
 * </p>
 */
public class DynamicDataReaderWriterTest {
    /** Mocked Server information for testing. */
    private static ServerInformation serverInfo;
    /** Data reader instance for testing. */
    private DynamicDataReader dataReader;
    /** Data writer instance for testing. */
    private DynamicDataWriter dataWriter;
    /** Output stream for writing data. */
    private ByteArrayOutputStream byteOut;

    /**
     * Sets up server information with default configurations.
     */
    @BeforeAll
    static void setUpServerInformation() {
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
     * Sets up the data writer and output stream for tests.
     */
    @BeforeEach
    void setUp() {
        byteOut = new ByteArrayOutputStream();
        dataWriter = new DynamicDataWriter(serverInfo, byteOut);
    }

    /**
     * Tests reading a single-byte TCP_EUI value successfully.
     */
    @Test
    void testReadTCPEUISingleByte() throws Exception {
        byte[] bytes = Utility.createTCPEUIByteArray(5);
        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        long value = dataReader.readTCPEUI();
        assertEquals(5, value, "Expected TCP_EUI value to be 5.");
    }

    /**
     * Tests reading a multibyte TCP_EUI value successfully.
     */
    @Test
    void testReadTCPEUIMultiByte() throws Exception {
        byte[] bytes = Utility.createTCPEUIByteArray(300);
        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        long value = dataReader.readTCPEUI();
        assertEquals(300, value, "Expected TCP_EUI value to be 300.");
    }

    /**
     * Tests handling of invalid TCP_EUI encoding.
     */
    @Test
    void testReadTCPEUIInvalidEncoding() {
        byte[] bytes = new byte[]{(byte) 0xFF};
        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        Exception exception = assertThrows(TCPClientException.class, () -> dataReader.readTCPEUI());

        assertTrue(exception.getMessage().contains("Read failed. Failed to read TCP_EUI encoded integer."),
            "Expected exception message to mention too many leading ones.");
    }

    /**
     * Tests reading a valid string within size limits.
     */
    @Test
    void testReadStringSuccess() throws Exception {
        String expectedString = "Hello, World!";
        byte[] tcpEUI = Utility.createTCPEUIByteArray(expectedString.length());
        byte[] stringBytes = expectedString.getBytes(StandardCharsets.ISO_8859_1);

        ByteArrayOutputStream byteOut = new ByteArrayOutputStream();
        byteOut.write(tcpEUI);
        byteOut.write(stringBytes);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOut.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        String actualString = dataReader.readString();
        assertEquals(expectedString, actualString, "Expected strings to match.");
    }

    /**
     * Tests handling of a string exceeding size limits.
     */
    @Test
    void testReadStringExceedsLimit() throws Exception {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < serverInfo.getStringSize() + 1; i++) {
            sb.append('a');
        }
        String longString = sb.toString();

        byte[] tcpEUI = Utility.createTCPEUIByteArray(longString.length());
        byte[] stringBytes = longString.getBytes(StandardCharsets.ISO_8859_1);

        ByteArrayOutputStream byteOut = new ByteArrayOutputStream();
        byteOut.write(tcpEUI);
        byteOut.write(stringBytes);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOut.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        Exception exception = assertThrows(TCPClientReadException.class, () -> dataReader.readString());

        assertTrue(exception.getMessage().contains("Invalid string length"),
            "Expected exception message to mention invalid string length.");
    }

    /**
     * Tests reading a boolean value successfully.
     */
    @Test
    void testReadBoolean() throws Exception {
        byte[] bytes = new byte[]{0x01};
        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        boolean value = dataReader.readBoolean();
        assertTrue(value, "Expected boolean value to be true.");
    }

    /**
     * Tests reading a valid integer value.
     */
    @Test
    void testReadIntSuccess() throws Exception {
        int expectedInt = 123456789;
        byte[] bytes = ByteBuffer.allocate(serverInfo.getIntSize())
            .order(ByteOrder.BIG_ENDIAN)
            .putInt(expectedInt)
            .array();

        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        int actualInt = dataReader.readInt();
        assertEquals(expectedInt, actualInt, "Expected integers to match.");
    }

    /**
     * Tests handling of insufficient data for integer reading.
     */
    @Test
    void testReadIntInvalidSize() {
        // Provide fewer bytes than expected
        byte[] bytes = new byte[]{0x00, 0x01}; // Only 2 bytes, but server expects 4
        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        Exception exception = assertThrows(TCPClientReadException.class, () -> dataReader.readInt());

        assertTrue(exception.getMessage().contains("Failed to read int"),
            "Expected exception message to mention failure to read int.");
    }

    /**
     * Tests reading a double value successfully as 8 bytes.
     */
    @Test
    void testReadDoubleSuccess_Double() throws Exception {
        serverInfo.setDoubleSize(8);
        double expectedDouble = 12345.6789;
        byte[] bytes = ByteBuffer.allocate(serverInfo.getDoubleSize())
            .order(ByteOrder.BIG_ENDIAN)
            .putDouble(expectedDouble)
            .array();

        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        double actualDouble = dataReader.readDouble();
        assertEquals(expectedDouble, actualDouble, "Expected doubles to match.");
    }

    /**
     * Tests reading a double value successfully as 4 bytes (float).
     */
    @Test
    void testReadDoubleSuccess_Float() throws Exception {
        // Configure serverInfo to expect 4 bytes for double
        serverInfo.setDoubleSize(4);
        float expectedFloat = 12345.67f;
        byte[] bytes = ByteBuffer.allocate(serverInfo.getDoubleSize())
            .order(ByteOrder.BIG_ENDIAN)
            .putFloat(expectedFloat)
            .array();

        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        double actualValue = dataReader.readDouble();
        assertEquals(expectedFloat, actualValue, "Expected float value to match as double.");
        serverInfo.setDoubleSize(8);
    }

    /**
     * Tests handling of a double value exceeding the float range.
     */
    @Test
    void testReadDoubleExceedsFloatRange() throws Exception {
        serverInfo.setDoubleSize(4);
        double largeDouble = Float.MAX_VALUE * 2;
        byte[] bytes = ByteBuffer.allocate(serverInfo.getDoubleSize())
            .order(ByteOrder.BIG_ENDIAN)
            .putFloat((float) largeDouble)
            .array();

        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        double actualValue = dataReader.readDouble();
        assertEquals((float) largeDouble, actualValue, "Expected float conversion to handle large double.");
    }

    /**
     * Tests reading a valid unsigned integer.
     */
    @Test
    void testReadUnsignedIntSuccess() throws Exception {
        long expectedUnsignedInt = 4000000000L;
        byte[] bytes = ByteBuffer.allocate(4)
            .order(ByteOrder.BIG_ENDIAN)
            .putInt((int) expectedUnsignedInt)
            .array();

        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        long actualUnsignedInt = dataReader.readUnsignedInt();
        assertEquals(expectedUnsignedInt, actualUnsignedInt, "Expected unsigned integers to match.");
    }

    /**
     * Tests boundary values for reading unsigned integers.
     */
    @Test
    void testReadUnsignedIntBoundaryValues() throws Exception {
        // Minimum unsigned int
        byte[] minBytes = new byte[]{0x00, 0x00, 0x00, 0x00};
        ByteArrayInputStream minIn = new ByteArrayInputStream(minBytes);
        DataInputStream dataMin = new DataInputStream(minIn);
        dataReader = new DynamicDataReader(serverInfo, dataMin);

        long minValue = dataReader.readUnsignedInt();
        assertEquals(0L, minValue, "Expected minimum unsigned int to be 0.");

        byte[] maxBytes = new byte[]{(byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF};
        ByteArrayInputStream maxIn = new ByteArrayInputStream(maxBytes);
        DataInputStream dataMax = new DataInputStream(maxIn);
        dataReader = new DynamicDataReader(serverInfo, dataMax);

        long maxValue = dataReader.readUnsignedInt();
        assertEquals(4294967295L, maxValue, "Expected maximum unsigned int to be 4294967295.");
    }

    /**
     * Tests reading a server error and extracting error details.
     */
    @Test
    void testReadServerError() throws Exception {
        int errorCode = 0xFFFF;
        String errorMessage = "General Protocol Error";

        ByteArrayOutputStream byteOut = new ByteArrayOutputStream();
        DataOutputStream dataOut = new DataOutputStream(byteOut);
        dataOut.writeShort(errorCode);

        byte[] errorMessageBytes = errorMessage.getBytes(StandardCharsets.ISO_8859_1);
        byte[] tcpEUI = Utility.createTCPEUIByteArray(errorMessageBytes.length);
        dataOut.write(tcpEUI);
        dataOut.write(errorMessageBytes);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOut.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        Optional<Pair<String, Integer>> error = dataReader.readServerError(false);
        assertTrue(error.isPresent(), "Expected an error to be present.");
        assertEquals(errorMessage, error.get().getLeft(), "Expected error message to match.");
        assertEquals(errorCode, error.get().getRight(), "Expected error code to match.");
    }

    /**
     * Tests reading a server error and throwing an exception.
     */
    @Test
    void testReadServerErrorWithException() throws Exception {
        int errorCode = 0xFFFF;
        String errorMessage = "General Protocol Error";

        ByteArrayOutputStream byteOut = new ByteArrayOutputStream();
        DataOutputStream dataOut = new DataOutputStream(byteOut);
        dataOut.writeShort(errorCode);
        byte[] errorMessageBytes = errorMessage.getBytes(StandardCharsets.ISO_8859_1);
        byte[] tcpEUI = Utility.createTCPEUIByteArray(errorMessageBytes.length);
        dataOut.write(tcpEUI);
        dataOut.write(errorMessageBytes);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOut.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        ServerErrorException exception = assertThrows(ServerErrorException.class, () -> dataReader.readServerError(true));

        assertEquals(errorCode, exception.getErrorCode(), "Expected error code to match.");
        assertEquals("Server Error 65535: " + errorMessage, exception.getMessage(), "Expected error message to match.");
    }

    /**
     * Tests writing a single-byte TCP_EUI value.
     */
    @Test
    void testWriteTCPEUISingleByte() throws Exception {
        String testString = "aaaaaa";

        dataWriter.writeString(testString);
        byte[] expected = Utility.createTCPEUIByteArray(testString.length());

        byte[] actual = byteOut.toByteArray();
        byte[] expectedPacket = new byte[expected.length + testString.length()];
        System.arraycopy(expected, 0, expectedPacket, 0, expected.length);
        System.arraycopy(testString.getBytes(StandardCharsets.ISO_8859_1), 0, expectedPacket, expected.length, testString.length());
        System.arraycopy(testString.getBytes(StandardCharsets.ISO_8859_1), 0, expectedPacket, expected.length, testString.length());

        assertArrayEquals(expectedPacket, actual, "Expected TCP_EUI encoded length followed by string bytes.");
    }

    /**
     * Tests writing a multi-byte TCP_EUI value.
     */
    @Test
    void testWriteTCPEUIMultiByte() throws Exception {
        long value = 255;
        byte[] expected = Utility.createTCPEUIByteArray(value);

        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < value; i++) {
            sb.append('a');
        }
        dataWriter.writeString(sb.toString());

        byte[] actual = byteOut.toByteArray();
        byte[] expectedPacket = new byte[expected.length + sb.length()];
        System.arraycopy(expected, 0, expectedPacket, 0, expected.length);
        System.arraycopy(sb.toString().getBytes(StandardCharsets.ISO_8859_1), 0, expectedPacket, expected.length, sb.length());

        assertArrayEquals(expectedPacket, actual, "Expected multi-byte TCP_EUI encoded length followed by string bytes.");
    }

    /**
     * Test writing a string that exceeds the server's size limit.
     */
    @Test
    void testWriteStringExceedsLimit() {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < serverInfo.getStringSize() + 1; i++) {
            sb.append('a');
        }
        String longString = sb.toString();

        TCPClientWriteException exception = assertThrows(TCPClientWriteException.class, () -> dataWriter.writeString(longString));

        assertEquals("Write failed. String length exceeds server limit.", exception.getMessage(),
            "Expected exception message for exceeding string length.");
    }

    /**
     * Tests writing a boolean value.
     */
    @Test
    void testWriteBoolean() throws Exception {
        TcpGeneral booleanTrue = TcpGeneral.ofSimple(SimpleType.BOOLEAN, true);
        dataWriter.writeGeneral(booleanTrue);

        byte[] actual = byteOut.toByteArray();
        byte typeDef = (byte) (SimpleType.BOOLEAN.getTypeId());
        byte expectedValue = 0x01;

        byte[] expected = new byte[]{typeDef, expectedValue};

        assertArrayEquals(expected, actual, "Expected TypeDefinition byte followed by 0x01 for true.");
    }

    /**
     * Tests writing an integer value.
     */
    @Test
    void testWriteIntSuccess() throws Exception {
        int value = 123456789;
        TcpGeneral intValue = TcpGeneral.ofSimple(SimpleType.INT, value);
        dataWriter.writeGeneral(intValue);

        byte[] actual = byteOut.toByteArray();

        byte typeDef = (byte) (SimpleType.INT.getTypeId());
        byte[] intBytes = ByteBuffer.allocate(serverInfo.getIntSize())
            .order(ByteOrder.BIG_ENDIAN)
            .putInt(value)
            .array();

        byte[] expected = new byte[1 + serverInfo.getIntSize()];
        expected[0] = typeDef;
        System.arraycopy(intBytes, 0, expected, 1, serverInfo.getIntSize());

        assertArrayEquals(expected, actual, "Expected TypeDefinition byte followed by int bytes.");
    }

    /**
     * Test writing an integer that exceeds the server's allowed range.
     */
    @Test
    void testWriteIntExceedsLimit() {
        int value = Integer.MAX_VALUE;
        serverInfo.setIntSize(3);
        TcpGeneral intValue = TcpGeneral.ofSimple(SimpleType.INT, value);

        TCPClientWriteException exception = assertThrows(TCPClientWriteException.class, () -> dataWriter.writeGeneral(intValue));

        assertTrue(exception.getMessage().contains("Failed to write int"),
            "Expected exception message to mention integer out of range.");

        serverInfo.setIntSize(4);
    }

    /**
     * Tests writing a double value as 8 bytes.
     */
    @Test
    void testWriteDoubleSuccess_Double() throws Exception {
        serverInfo.setDoubleSize(8);
        double value = 12345.6789;
        TcpGeneral doubleValue = TcpGeneral.ofSimple(SimpleType.DOUBLE, value);
        dataWriter.writeGeneral(doubleValue);

        byte[] actual = byteOut.toByteArray();

        byte typeDef = (byte) (SimpleType.DOUBLE.getTypeId());
        byte[] doubleBytes = ByteBuffer.allocate(serverInfo.getDoubleSize())
            .order(ByteOrder.BIG_ENDIAN)
            .putDouble(value)
            .array();

        byte[] expected = new byte[1 + serverInfo.getDoubleSize()];
        expected[0] = typeDef;
        System.arraycopy(doubleBytes, 0, expected, 1, serverInfo.getDoubleSize());

        assertArrayEquals(expected, actual, "Expected TypeDefinition byte followed by double bytes.");
    }

    /**
     * Tests writing a double value as 4 bytes (float).
     */
    @Test
    void testWriteDouble_Success_Float() throws Exception {
        serverInfo.setDoubleSize(4);
        double value = 12345.67;
        TcpGeneral doubleValue = TcpGeneral.ofSimple(SimpleType.DOUBLE, value);
        dataWriter.writeGeneral(doubleValue);

        byte[] actual = byteOut.toByteArray();
        // TypeDefinition for SIMPLE DOUBLE: typeId | flags (no COMPLEX)
        byte typeDef = (byte) (SimpleType.DOUBLE.getTypeId());
        byte[] floatBytes = ByteBuffer.allocate(4)
            .order(ByteOrder.BIG_ENDIAN)
            .putFloat((float) value)
            .array();

        byte[] expected = new byte[1 + 4];
        expected[0] = typeDef;
        System.arraycopy(floatBytes, 0, expected, 1, 4);

        assertArrayEquals(expected, actual, "Expected TypeDefinition byte followed by float bytes.");
        serverInfo.setDoubleSize(8);
    }

    /**
     * Tests handling of a double value exceeding the float range.
     */
    @Test
    void testWriteDoubleExceedsFloatRange() {
        serverInfo.setDoubleSize(4);
        double largeDouble = Float.MAX_VALUE * 2;
        TcpGeneral doubleValue = TcpGeneral.ofSimple(SimpleType.DOUBLE, largeDouble);

        TCPClientWriteException exception = assertThrows(TCPClientWriteException.class, () -> dataWriter.writeGeneral(doubleValue));

        assertTrue(exception.getMessage().contains("out of range"),
            "Expected exception message to mention double out of range.");
        serverInfo.setDoubleSize(8);
    }

    /**
     * Tests writing a valid unsigned integer.
     */
    @Test
    void testWriteUnsignedIntSuccess() throws Exception {
        long unsignedInt = 4000000000L; // 0xEE6B2800
        dataWriter.writeUnsignedInt(unsignedInt);

        byte[] actual = byteOut.toByteArray();
        byte[] expected = ByteBuffer.allocate(4)
            .order(ByteOrder.BIG_ENDIAN)
            .putInt((int) unsignedInt)
            .array();

        assertArrayEquals(expected, actual, "Expected unsigned int bytes to match.");
    }

    /**
     * Tests handling of an unsigned integer out of range.
     */
    @Test
    void testWriteUnsignedIntOutOfRange() {
        long invalidUnsignedInt = 5000000000L; // Exceeds 0xFFFFFFFF
        IllegalArgumentException exception = assertThrows(IllegalArgumentException.class, () -> dataWriter.writeUnsignedInt(invalidUnsignedInt));

        assertTrue(exception.getMessage().contains("out of range"),
            "Expected exception message to mention unsigned int out of range.");
    }

    /**
     * Tests reading a key map and validating its structure.
     */
    @Test
    void testReadGeneralKeyMapSuccess() throws Exception {
        HashMap<String, TcpGeneral> keyMap = new HashMap<>();
        keyMap.put("key1", TcpGeneral.ofSimple(SimpleType.STRING, "value1"));
        keyMap.put("key2", TcpGeneral.ofSimple(SimpleType.INT, 42));

        TcpGeneral keyMapGeneral = TcpGeneral.ofKeyMap(keyMap);
        dataWriter.writeGeneral(keyMapGeneral);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOut.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        TcpGeneral result = dataReader.readGeneral();
        assertEquals(TcpGeneralType.KEY_MAP, result.getType(), "Expected TcpGeneral type to be KEY_MAP.");

        HashMap<String, TcpGeneral> resultMap = result.getKeyMap();
        assertEquals(2, resultMap.size(), "Expected key map size to be 2.");
        assertTrue(resultMap.containsKey("key1"), "Expected key1 in the map.");
        assertTrue(resultMap.containsKey("key2"), "Expected key2 in the map.");
        assertEquals("value1", resultMap.get("key1").getValue(), "Expected value1 for key1.");
        assertEquals(42, resultMap.get("key2").getValue(), "Expected 42 for key2.");
    }

    /**
     * Test reading an array of simple types successfully.
     */
    @Test
    void testReadGeneralArraySimpleSuccess() throws Exception {
        List<TcpGeneral> array = Arrays.asList(
            TcpGeneral.ofSimple(SimpleType.INT, 10),
            TcpGeneral.ofSimple(SimpleType.INT, 20),
            TcpGeneral.ofSimple(SimpleType.INT, 30)
        );

        TcpGeneral arrayGeneral = TcpGeneral.ofArray(array);
        dataWriter.writeGeneral(arrayGeneral);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOut.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        TcpGeneral result = dataReader.readGeneral();
        assertEquals(TcpGeneralType.ARRAY, result.getType(), "Expected TcpGeneral type to be ARRAY.");

        List<TcpGeneral> resultArray = result.getArray();
        assertEquals(3, resultArray.size(), "Expected array size to be 3.");
        assertEquals(10, resultArray.get(0).getValue(), "Expected first element to be 10.");
        assertEquals(20, resultArray.get(1).getValue(), "Expected second element to be 20.");
        assertEquals(30, resultArray.get(2).getValue(), "Expected third element to be 30.");
    }

    /**
     * Tests reading an array of complex types (key maps).
     */
    @Test
    void testReadGeneralArrayComplexSuccess() throws Exception {
        // Prepare array of key-value maps
        // Map1: {"k1": "v1"}
        // Map2: {"k2": 200}
        HashMap<String, TcpGeneral> map1 = new HashMap<>();
        map1.put("k1", TcpGeneral.ofSimple(SimpleType.STRING, "v1"));

        HashMap<String, TcpGeneral> map2 = new HashMap<>();
        map2.put("k2", TcpGeneral.ofSimple(SimpleType.INT, 200));

        List<TcpGeneral> array = Arrays.asList(
            TcpGeneral.ofKeyMap(map1),
            TcpGeneral.ofKeyMap(map2)
        );

        TcpGeneral arrayGeneral = TcpGeneral.ofArray(array);
        dataWriter.writeGeneral(arrayGeneral);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOut.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        TcpGeneral result = dataReader.readGeneral();
        assertEquals(TcpGeneralType.ARRAY, result.getType(), "Expected TcpGeneral type to be ARRAY.");

        List<TcpGeneral> resultArray = result.getArray();
        assertEquals(2, resultArray.size(), "Expected array size to be 2.");

        TcpGeneral resultMap1 = resultArray.get(0);
        assertEquals(TcpGeneralType.KEY_MAP, resultMap1.getType(), "Expected first element to be KEY_MAP.");
        HashMap<String, TcpGeneral> resultMap1Contents = resultMap1.getKeyMap();
        assertEquals(1, resultMap1Contents.size(), "Expected Map1 size to be 1.");
        assertTrue(resultMap1Contents.containsKey("k1"), "Expected Map1 to contain 'k1'.");
        assertEquals("v1", resultMap1Contents.get("k1").getValue(), "Expected value for 'k1' to be 'v1'.");

        TcpGeneral resultMap2 = resultArray.get(1);
        assertEquals(TcpGeneralType.KEY_MAP, resultMap2.getType(), "Expected second element to be KEY_MAP.");
        HashMap<String, TcpGeneral> resultMap2Contents = resultMap2.getKeyMap();
        assertEquals(1, resultMap2Contents.size(), "Expected Map2 size to be 1.");
        assertTrue(resultMap2Contents.containsKey("k2"), "Expected Map2 to contain 'k2'.");
        assertEquals(200, resultMap2Contents.get("k2").getValue(), "Expected value for 'k2' to be 200.");
    }

    /**
     * Tests reading a pair of simple types.
     */
    @Test
    void testReadGeneralPairSimpleSuccess() throws Exception {
        TcpGeneral pair = TcpGeneral.ofPair(
            TcpGeneral.ofSimple(SimpleType.STRING, "key"),
            TcpGeneral.ofSimple(SimpleType.INT, 256)
        );

        dataWriter.writeGeneral(pair);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOut.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        TcpGeneral result = dataReader.readGeneral();
        assertEquals(TcpGeneralType.PAIR, result.getType(), "Expected TcpGeneral type to be PAIR.");

        Pair<TcpGeneral, TcpGeneral> resultPair = result.getPair();
        assertEquals("key", resultPair.getLeft().getValue(), "Expected first element of pair to be 'key'.");
        assertEquals(256, resultPair.getRight().getValue(), "Expected second element of pair to be 256.");
    }

    /**
     * Tests reading a pair of complex types.
     */
    @Test
    void testReadGeneralPairComplexSuccess() throws Exception {
        // Prepare pair: ({"innerKey": "innerValue"}, "simpleValue")
        HashMap<String, TcpGeneral> innerMap = new HashMap<>();
        innerMap.put("innerKey", TcpGeneral.ofSimple(SimpleType.STRING, "innerValue"));

        TcpGeneral pair = TcpGeneral.ofPair(
            TcpGeneral.ofKeyMap(innerMap),
            TcpGeneral.ofSimple(SimpleType.STRING, "simpleValue")
        );

        dataWriter.writeGeneral(pair);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOut.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        TcpGeneral result = dataReader.readGeneral();
        assertEquals(TcpGeneralType.PAIR, result.getType(), "Expected TcpGeneral type to be PAIR.");

        Pair<TcpGeneral, TcpGeneral> resultPair = result.getPair();

        TcpGeneral first = resultPair.getLeft();
        assertEquals(TcpGeneralType.KEY_MAP, first.getType(), "Expected first element to be KEY_MAP.");
        HashMap<String, TcpGeneral> firstMap = first.getKeyMap();
        assertEquals(1, firstMap.size(), "Expected first map size to be 1.");
        assertTrue(firstMap.containsKey("innerKey"), "Expected first map to contain 'innerKey'.");
        assertEquals("innerValue", firstMap.get("innerKey").getValue(), "Expected value for 'innerKey' to be 'innerValue'.");

        TcpGeneral second = resultPair.getRight();
        assertEquals(TcpGeneralType.SIMPLE, second.getType(), "Expected second element to be SIMPLE.");
        assertEquals(SimpleType.STRING, second.getSimpleType(), "Expected second element to be STRING.");
        assertEquals("simpleValue", second.getValue(), "Expected value to be 'simpleValue'.");
    }

    /**
     * Tests handling of invalid type definitions.
     */
    @Test
    void testReadGeneralInvalidTypeDefinitionArrayAndPair() throws Exception {
        ByteArrayOutputStream byteOutManual = new ByteArrayOutputStream();
        DataOutputStream dataOutManual = new DataOutputStream(byteOutManual);

        byte invalidTypeDef = (byte) (TypeFlag.ARRAY.getBitmask() | TypeFlag.PAIR.getBitmask() | SimpleType.INT.getTypeId());
        dataOutManual.writeByte(invalidTypeDef);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOutManual.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        Exception exception = assertThrows(TCPClientReadException.class, () -> dataReader.readGeneral());

        assertTrue(exception.getMessage().contains("cannot have both Array and Pair flags set"),
            "Expected exception message to mention invalid TypeDefinition.");
    }

    /**
     * Tests handling of unknown simple types.
     */
    @Test
    void testReadGeneralUnknownSimpleType() throws Exception {
        ByteArrayOutputStream byteOutManual = new ByteArrayOutputStream();
        DataOutputStream dataOutManual = new DataOutputStream(byteOutManual);

        byte invalidTypeDef = (byte) 7; // 7 is not a valid SimpleType
        dataOutManual.writeByte(invalidTypeDef);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOutManual.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        // Read and assert exception
        Exception exception = assertThrows(TCPClientReadException.class, () -> dataReader.readGeneral());

        assertTrue(exception.getMessage().contains("Read failed. Unknown SimpleType in Type-Definition-Block."),
            "Expected exception message to mention unsupported SimpleType.");
    }

    /**
     * Tests handling of incomplete data for simple types.
     */
    @Test
    void testReadGeneralSimpleTypeIncompleteData() throws Exception {
        ByteArrayOutputStream byteOutManual = new ByteArrayOutputStream();
        DataOutputStream dataOutManual = new DataOutputStream(byteOutManual);

        byte typeDef = (byte) (SimpleType.INT.getTypeId());
        dataOutManual.writeByte(typeDef);
        dataOutManual.writeShort(1234); // Should be int, not short

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOutManual.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        // Read and assert exception
        Exception exception = assertThrows(TCPClientReadException.class, () -> dataReader.readGeneral());

        assertTrue(exception.getMessage().contains("Failed to read int"),
            "Expected exception message to mention failure to read int.");
    }

    /**
     * Tests handling of pairs with missing elements.
     */
    @Test
    void testReadGeneralPairMissingSecondElement() {
        Exception exception = assertThrows(NullPointerException.class, () -> TcpGeneral.ofPair(
            TcpGeneral.ofSimple(SimpleType.STRING, "key"),
            null // Missing second element
        ));

        assertTrue(exception.getMessage().contains("Second element cannot be null for PAIR TcpGeneral"),
            "Expected exception message to mention failure to read.");
    }

    /**
     * Tests handling of invalid key lengths in key maps.
     */
    @Test
    void testReadGeneralKeyMapInvalidKeyLength() throws Exception {
        ByteArrayOutputStream byteOutManual = new ByteArrayOutputStream();
        DataOutputStream dataOutManual = new DataOutputStream(byteOutManual);

        byte keyMapTypeDef = (byte) (SimpleType.UNKNOWN.getTypeId() | TypeFlag.KEY.getBitmask());
        dataOutManual.writeByte(keyMapTypeDef);

        byte[] lengthBytes = Utility.createTCPEUIByteArray(1);
        dataOutManual.write(lengthBytes);

        byte[] keyLength = Utility.createTCPEUIByteArray(serverInfo.getStringSize() + 1);
        byte[] keyBytes = new byte[serverInfo.getStringSize() + 1];
        Arrays.fill(keyBytes, (byte) 'a');
        dataOutManual.write(keyLength);
        dataOutManual.write(keyBytes);

        byte valueTypeDef = (byte) (SimpleType.STRING.getTypeId());
        byte[] valueLength = Utility.createTCPEUIByteArray("value".length());
        byte[] valueBytes = "value".getBytes(StandardCharsets.ISO_8859_1);
        dataOutManual.writeByte(valueTypeDef);
        dataOutManual.write(valueLength);
        dataOutManual.write(valueBytes);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOutManual.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        Exception exception = assertThrows(TCPClientReadException.class, () -> dataReader.readGeneral());

        assertTrue(exception.getMessage().contains("Invalid string length"),
            "Expected exception message to mention invalid string length.");
    }

    /**
     * Tests handling of incomplete data for boolean types.
     */
    @Test
    void testReadGeneralSimpleBooleanIncompleteData() throws Exception {
        ByteArrayOutputStream byteOutManual = new ByteArrayOutputStream();
        DataOutputStream dataOutManual = new DataOutputStream(byteOutManual);

        byte typeDef = (byte) (SimpleType.BOOLEAN.getTypeId());
        dataOutManual.writeByte(typeDef);


        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOutManual.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        Exception exception = assertThrows(TCPClientReadException.class, () -> dataReader.readGeneral());

        assertTrue(exception.getMessage().contains("Failed to read boolean"),
            "Expected exception message to mention failure to read boolean.");
    }
}
