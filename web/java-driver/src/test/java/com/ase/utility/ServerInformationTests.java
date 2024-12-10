package com.ase.utility;

import com.ase.exception.HandshakeException;
import org.junit.jupiter.api.Test;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;

import static org.junit.jupiter.api.Assertions.*;

class ServerInformationTest {

    @Test
    void testReadServerInformation() throws HandshakeException {
        byte[] serverInfoBytes = new byte[]{
            (byte) 0x01,       // booleanSize
            (byte) 0x04,       // intSize
            (byte) 0x02,       // charSize
            (byte) 0x08,       // doubleSize
            (byte) 0x04,       // semaphoreSize
            (byte) 0x00, (byte) 0xFF, // stringSize (255)
            (byte) 0x01, (byte) 0x00  // arraySize (256)
        };

        ByteArrayInputStream byteIn = new ByteArrayInputStream(serverInfoBytes);
        DataInputStream dataInputStream = new DataInputStream(byteIn);

        ServerInformation serverInfo = ServerInformation.readServerInformation(dataInputStream);

        // Verify that the server information is read correctly
        assertEquals(1, serverInfo.getBooleanSize(), "Incorrect booleanSize");
        assertEquals(4, serverInfo.getIntSize(), "Incorrect intSize");
        assertEquals(2, serverInfo.getCharSize(), "Incorrect charSize");
        assertEquals(8, serverInfo.getDoubleSize(), "Incorrect doubleSize");
        assertEquals(4, serverInfo.getSemaphoreSize(), "Incorrect semaphoreSize");
        assertEquals(255, serverInfo.getStringSize(), "Incorrect stringSize");
        assertEquals(256, serverInfo.getArraySize(), "Incorrect arraySize");
    }

    @Test
    void testReadServerInformationInvalidData() {
        byte[] invalidServerInfoBytes = new byte[]{0x01, 0x04};

        ByteArrayInputStream byteIn = new ByteArrayInputStream(invalidServerInfoBytes);
        DataInputStream dataInputStream = new DataInputStream(byteIn);

        assertThrows(HandshakeException.class, () -> {
            ServerInformation.readServerInformation(dataInputStream);
        }, "Expected HandshakeException due to insufficient data");
    }

    @Test
    void testValidateInvalidData() {
        ServerInformation serverInfo = new ServerInformation();
        serverInfo.setBooleanSize(1);
        serverInfo.setIntSize(5); // Larger than 4
        serverInfo.setCharSize(2);
        serverInfo.setDoubleSize(8);
        serverInfo.setSemaphoreSize(4);

        boolean isValid = serverInfo.validate();

        assertFalse(isValid, "ServerInformation should be invalid with missing sizes");
    }

    @Test
    void testValidateValidData() {
        ServerInformation serverInfo = new ServerInformation();
        serverInfo.setBooleanSize(1);
        serverInfo.setIntSize(4);
        serverInfo.setCharSize(2);
        serverInfo.setDoubleSize(8);
        serverInfo.setSemaphoreSize(4);

        boolean isValid = serverInfo.validate();

        assertTrue(isValid, "ServerInformation should be valid with correct sizes");
    }

    @Test
    void testGettersAndSetters() {
        ServerInformation serverInfo = new ServerInformation();

        serverInfo.setBooleanSize(1);
        serverInfo.setIntSize(4);
        serverInfo.setCharSize(2);
        serverInfo.setDoubleSize(8);
        serverInfo.setSemaphoreSize(4);
        serverInfo.setStringSize(1024);
        serverInfo.setArraySize(2048);

        assertEquals(1, serverInfo.getBooleanSize(), "getBooleanSize() returned incorrect value");
        assertEquals(4, serverInfo.getIntSize(), "getIntSize() returned incorrect value");
        assertEquals(2, serverInfo.getCharSize(), "getCharSize() returned incorrect value");
        assertEquals(8, serverInfo.getDoubleSize(), "getDoubleSize() returned incorrect value");
        assertEquals(4, serverInfo.getSemaphoreSize(), "getSemaphoreSize() returned incorrect value");
        assertEquals(1024, serverInfo.getStringSize(), "getStringSize() returned incorrect value");
        assertEquals(2048, serverInfo.getArraySize(), "getArraySize() returned incorrect value");
    }
}
