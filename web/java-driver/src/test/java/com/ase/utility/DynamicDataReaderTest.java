package com.ase.utility;

import com.ase.exception.ServerErrorException;
import com.ase.exception.TCPClientException;
import com.ase.exception.TCPClientReadException;
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

public class DynamicDataReaderTest {
    private static ServerInformation serverInfo;
    private DynamicDataReader dataReader;
    private DynamicDataWriter dataWriter;
    private ByteArrayOutputStream byteOut;

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

    @BeforeEach
    void setUp() {
        byteOut = new ByteArrayOutputStream();
        dataWriter = new DynamicDataWriter(serverInfo, byteOut);
    }

    @Test
    void testReadTCPEUISingleByte() throws Exception {
        byte[] bytes = Utility.createTCPEUIByteArray(5);
        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        long value = dataReader.readTCPEUI();
        assertEquals(5, value, "Expected TCP_EUI value to be 5.");
    }

    @Test
    void testReadTCPEUIMultiByte() throws Exception {
        byte[] bytes = Utility.createTCPEUIByteArray(300);
        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        long value = dataReader.readTCPEUI();
        assertEquals(300, value, "Expected TCP_EUI value to be 300.");
    }

    @Test
    void testReadTCPEUIInvalidEncoding() throws Exception {
        byte[] bytes = new byte[]{(byte) 0xFF};
        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        Exception exception = assertThrows(TCPClientException.class, () -> {
            dataReader.readTCPEUI();
        });

        assertTrue(exception.getMessage().contains("Read failed. Failed to read TCP_EUI encoded integer."),
            "Expected exception message to mention too many leading ones.");
    }

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

        Exception exception = assertThrows(TCPClientReadException.class, () -> {
            dataReader.readString();
        });

        assertTrue(exception.getMessage().contains("Invalid string length"),
            "Expected exception message to mention invalid string length.");
    }

    @Test
    void testReadBoolean() throws Exception {
        byte[] bytes = new byte[]{0x01};
        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        boolean value = dataReader.readBoolean();
        assertTrue(value, "Expected boolean value to be true.");
    }

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

    @Test
    void testReadIntInvalidSize() throws Exception {
        // Provide fewer bytes than expected
        byte[] bytes = new byte[]{0x00, 0x01}; // Only 2 bytes, but server expects 4
        ByteArrayInputStream byteIn = new ByteArrayInputStream(bytes);
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        Exception exception = assertThrows(TCPClientReadException.class, () -> {
            dataReader.readInt();
        });

        assertTrue(exception.getMessage().contains("Failed to read int"),
            "Expected exception message to mention failure to read int.");
    }

    /**
     * Test reading a double value successfully (8 bytes).
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
     * Test reading a double value successfully as float (4 bytes).
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
    }

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
     * Test reading a server error and throwing an exception.
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

        ServerErrorException exception = assertThrows(ServerErrorException.class, () -> {
            dataReader.readServerError(true);
        });

        assertEquals(errorCode, exception.getErrorCode(), "Expected error code to match.");
        assertEquals("Server Error 65535: " + errorMessage, exception.getMessage(), "Expected error message to match.");
    }

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

    @Test
    void testReadGeneralInvalidTypeDefinitionArrayAndPair() throws Exception {
        ByteArrayOutputStream byteOutManual = new ByteArrayOutputStream();
        DataOutputStream dataOutManual = new DataOutputStream(byteOutManual);

        byte invalidTypeDef = (byte) (TypeFlag.ARRAY.getBitmask() | TypeFlag.PAIR.getBitmask() | SimpleType.INT.getTypeId());
        dataOutManual.writeByte(invalidTypeDef);

        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOutManual.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        Exception exception = assertThrows(TCPClientReadException.class, () -> {
            dataReader.readGeneral();
        });

        assertTrue(exception.getMessage().contains("cannot have both Array and Pair flags set"),
            "Expected exception message to mention invalid TypeDefinition.");
    }

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
        Exception exception = assertThrows(TCPClientReadException.class, () -> {
            dataReader.readGeneral();
        });

        assertTrue(exception.getMessage().contains("Read failed. Unknown SimpleType in Type-Definition-Block."),
            "Expected exception message to mention unsupported SimpleType.");
    }

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
        Exception exception = assertThrows(TCPClientReadException.class, () -> {
            dataReader.readGeneral();
        });

        assertTrue(exception.getMessage().contains("Failed to read int"),
            "Expected exception message to mention failure to read int.");
    }

    @Test
    void testReadGeneralPairMissingSecondElement() {
        Exception exception = assertThrows(NullPointerException.class, () -> {
            TcpGeneral pair = TcpGeneral.ofPair(
                TcpGeneral.ofSimple(SimpleType.STRING, "key"),
                null // Missing second element
            );
        });

        assertTrue(exception.getMessage().contains("Second element cannot be null for PAIR TcpGeneral"),
            "Expected exception message to mention failure to read.");
    }

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

        Exception exception = assertThrows(TCPClientReadException.class, () -> {
            dataReader.readGeneral();
        });

        assertTrue(exception.getMessage().contains("Invalid string length"),
            "Expected exception message to mention invalid string length.");
    }

    @Test
    void testReadGeneralSimpleBooleanIncompleteData() throws Exception {
        ByteArrayOutputStream byteOutManual = new ByteArrayOutputStream();
        DataOutputStream dataOutManual = new DataOutputStream(byteOutManual);

        byte typeDef = (byte) (SimpleType.BOOLEAN.getTypeId());
        dataOutManual.writeByte(typeDef);


        ByteArrayInputStream byteIn = new ByteArrayInputStream(byteOutManual.toByteArray());
        DataInputStream dataIn = new DataInputStream(byteIn);
        dataReader = new DynamicDataReader(serverInfo, dataIn);

        Exception exception = assertThrows(TCPClientReadException.class, () -> {
            dataReader.readGeneral();
        });

        assertTrue(exception.getMessage().contains("Failed to read boolean"),
            "Expected exception message to mention failure to read boolean.");
    }
}
