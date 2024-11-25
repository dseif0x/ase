package com.ase.driver;

import com.ase.exception.HandshakeException;
import com.ase.exception.ServerErrorException;
import com.ase.exception.TCPClientException;
import com.ase.utility.DynamicDataReader;
import com.ase.utility.ServerInformation;
import com.ase.utility.TcpErrorCode;
import com.ase.utility.Utility;
import java.io.*;
import java.net.Socket;

/**
 * Manages the TCP connection and communication with a database server.
 */
public class DatabaseClient {
    /** The TCP socket used for server communication. */
    private final Socket socket;

    /** Input stream for reading data from the server. */
    private final DataInputStream reader;

    /** Output stream for sending data to the server. */
    private final DataOutputStream writer;

    /** Indicates whether a connection to the server has been established. */
    private boolean isLoggedIn = false;

    /** Reader for reading data from the server based on server-provided size information. */
    private DynamicDataReader dataReader;

    /**
     * Constructs a new DatabaseClient and establishes a socket connection.
     *
     * @param host The hostname or IP address of the database server
     * @param port The port number to connect to
     * @throws IOException If there is an error creating the socket or streams
     */
    public DatabaseClient(String host, int port) throws IOException {
        this.socket = new Socket(host, port);
        try {
            reader = new DataInputStream(this.socket.getInputStream());
            writer =  new DataOutputStream(this.socket.getOutputStream());
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    /**
     * Initiates connection with the server through a handshake process.
     *
     * @param version The client version to be sent during handshake
     * @param flags Byte representing client access permissions and capabilities
     * @throws TCPClientException If there are issues during the connection process
     */
    public void connect(int version, byte flags) throws TCPClientException {
        byte[] clientInfo = Utility.createClientInfoPacket(version, flags);
        try {
            writer.write(clientInfo);
            writer.flush();

            byte response = reader.readByte();
            if (response == Utility.NAK) {
                int errorCode = reader.readUnsignedShort();
                TcpErrorCode errorCodeEnum = TcpErrorCode.fromCode(errorCode);
                throw new ServerErrorException(errorCodeEnum.toString());
            } else if (response == Utility.AK) {
                ServerInformation serverInfo = ServerInformation.readServerInformation(reader);
                if (!serverInfo.validate()) {
                    writer.write(Utility.NAK);
                    throw new HandshakeException("Validation failed for server information. Sizes of server are too large.");
                } else {
                    this.dataReader = new DynamicDataReader(serverInfo, reader);

                    writer.write(Utility.AK);
                    writer.flush();
                }
            } else throw new HandshakeException("Server responded with unknown response.");
        } catch (IOException e) {
            throw new TCPClientException("Failed to send client info packet.", e);
        }
    }

    /**
     * Authenticates a user with the server using provided credentials.
     *
     * @param username The username for authentication
     * @param password The password for authentication
     * @throws TCPClientException If authentication fails or connection is not established
     */
    public void login(String username, String password) throws TCPClientException, ServerErrorException {
        byte[] userInfo = Utility.createUserInfoPacket(username, password);
        try {
            writer.write(userInfo);
            writer.flush();

            byte response = reader.readByte();
            if (response == Utility.NAK) {
                this.dataReader.readServerError();
            } else if (response == Utility.AK) {
                this.isLoggedIn = true;
            } else throw new TCPClientException("Server responded with unknown response.");
        } catch (IOException e) {
            throw new TCPClientException("Failed to send user info packet.", e);
        }
    }

    /**
     * Closes the socket connection to the server.
     */
    public void disconnect() {
        try {
            this.socket.close();
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }
}