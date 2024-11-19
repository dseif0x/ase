package com.ase.driver;

import com.ase.exception.HandshakeException;
import com.ase.exception.TCPClientException;
import com.ase.utility.ServerInformation;
import com.ase.utility.TcpErrorCode;
import com.ase.utility.Utility;
import java.io.*;
import java.net.Socket;

public class DatabaseClient {
    private final Socket socket;
    private final DataInputStream reader;
    private final DataOutputStream writer;

    private ServerInformation serverInformation;

    private boolean isConnected = false;
    private boolean isLoggedIn = false;

    public DatabaseClient(String host, int port) throws IOException {
        this.socket = new Socket(host, port);
        try {
            reader = new DataInputStream(this.socket.getInputStream());
            writer =  new DataOutputStream(this.socket.getOutputStream());
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    public void connect(int version, byte flags) throws TCPClientException {
        byte[] clientInfo = Utility.createClientInfoPacket(version, flags);
        try {
            writer.write(clientInfo);
            writer.flush();

            byte response = reader.readByte();
            if (response == Utility.NAK) {
                int errorCode = reader.readUnsignedShort();
                if (errorCode == 0x8000) throw new HandshakeException("Invalid Client Version.");
                else throw new HandshakeException("Server responded with NAK. Error code: " + errorCode);
            } else if (response == Utility.AK) {
                int boolSize = reader.readUnsignedByte();
                int intSize = reader.readUnsignedByte();
                int charSize = reader.readUnsignedByte();
                int doubleSize = reader.readUnsignedByte();
                int semaphoreSize = reader.readUnsignedByte();
                int maxStringSize = reader.readUnsignedShort();
                int maxArrayLength = reader.readUnsignedShort();

                this.serverInformation = new ServerInformation(boolSize, intSize, charSize, doubleSize, semaphoreSize, maxStringSize, maxArrayLength);
                writer.write(Utility.AK);
                writer.flush();
                this.isConnected = true;
            } else throw new HandshakeException("Server responded with unknown response.");
        } catch (IOException e) {
            throw new TCPClientException("Failed to send client info packet.", e);
        }
    }

    public void login(String username, String password) throws TCPClientException {
        if (!this.isConnected) throw new HandshakeException("Client is not connected to the server.");
        byte[] userInfo = Utility.createUserInfoPacket(username, password);
        try {
            writer.write(userInfo);
            writer.flush();

            byte response = reader.readByte();
            if (response == Utility.NAK) {
                int receivedErrorCode = reader.readUnsignedShort();
                TcpErrorCode errorCode = TcpErrorCode.fromCode(receivedErrorCode);

                throw new TCPClientException(errorCode.toString());
            } else if (response == Utility.AK) {
                this.isLoggedIn = true;
            } else throw new TCPClientException("Server responded with unknown response.");
        } catch (IOException e) {
            throw new TCPClientException("Failed to send user info packet.", e);
        }
    }

    public void disconnect() {
        try {
            this.socket.close();
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }
}
