package com.ase.driver;

import com.ase.exception.HandshakeException;
import com.ase.exception.ServerErrorException;
import com.ase.exception.TCPClientException;
import com.ase.utility.*;
import com.ase.utility.tcp.SimpleType;
import com.ase.utility.tcp.TcpErrorCode;
import com.ase.utility.tcp.TcpGeneral;
import org.apache.commons.lang3.tuple.Pair;

import java.io.*;
import java.net.Socket;
import java.util.Optional;

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

    /** Reader for reading data from the server based on server-provided size information. */
    private DynamicDataReader dataReader;
    private DynamicDataWriter dataWriter;

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
            } else if (response == Utility.ACK) {
                ServerInformation serverInfo = ServerInformation.readServerInformation(reader);
                if (!serverInfo.validate()) {
                    writer.write(Utility.NAK);
                    throw new HandshakeException("Validation failed for server information. Sizes of server are too large.");
                } else {
                    this.dataReader = new DynamicDataReader(serverInfo, reader);
                    this.dataWriter = new DynamicDataWriter(serverInfo, writer);
                    writer.write(Utility.ACK);
                    writer.flush();
                }
            } else throw new HandshakeException("Server responded with unknown response: " + String.format("0x%02X", response));
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
     * @throws ServerErrorException If the server responds with an error
     */
    public void login(String username, String password) throws TCPClientException, ServerErrorException {
        byte[] userInfo = Utility.createUserInfoPacket(username, password);
        try {
            writer.write(userInfo);
            writer.flush();

            byte response = reader.readByte();
            if (response == Utility.NAK) {
                this.dataReader.readServerError(true);
            } else if (response != Utility.ACK) throw new TCPClientException("Server responded with unknown response: " + String.format("0x%02X", response));
        } catch (IOException e) {
            throw new TCPClientException("Failed to send user info packet.", e);
        }
    }

    /**
     * Sends a GET command to the server to retrieve the value associated with a specific key.
     *
     * @param key The key whose associated value is to be retrieved from the server. Must not be {@code null} or empty.
     * @return A {@link WebReturn} object indicating the outcome of the GET operation:
     *         <ul>
     *             <li><strong>Success:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code true}.</li>
     *                     <li>{@code message} is {@code null}.</li>
     *                     <li>{@code data} contains the {@link TcpGeneral} value associated with the key.</li>
     *                 </ul>
     *             </li>
     *             <li><strong>Failure:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code false}.</li>
     *                     <li>{@code message} contains the error description, possibly returned by server.</li>
     *                     <li>{@code data} contains the error code returned by the server encapsulated in a {@link TcpGeneral} Object, if there is any returned by server.</li>
     *                 </ul>
     *             </li>
     *         </ul>
     */
    public WebReturn<TcpGeneral> get(String key) {
        if (key == null || key.isEmpty()) {
            return WebReturn.error("Key must not be null or empty.");
        }

        try {
            writer.write(Utility.GET_COMMAND);
            writer.flush();

            dataWriter.writeString(key);

            byte response = reader.readByte();
            if (response == Utility.NAK) {
                WebReturn<Integer> error = readError();
                return WebReturn.error("Server responded with NAK.", TcpGeneral.ofSimple(SimpleType.INT, error.getData()));
            } else if (response == Utility.ACK) {
                TcpGeneral tcpGeneral = dataReader.readGeneral();
                return WebReturn.success(tcpGeneral);
            } else return WebReturn.error("Server responded with unknown response: " + String.format("0x%02X", response));
        } catch (IOException e) {
            return WebReturn.error("Failed to send get command.");
        } catch (TCPClientException e) {
            return WebReturn.error(e.getMessage());
        }
    }

    /**
     * Sends a command to the server to set a key-value pair.
     *
     * @param key   The key of the key-value pair to be set. Must not be {@code null} or empty.
     * @param value The value of the key-value pair to be set. Must not be {@code null}.
     * @return A {@link WebReturn} object indicating the outcome of the SET operation:
     *         <ul>
     *             <li><strong>Success:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code true}.</li>
     *                     <li>{@code message} is {@code null}.</li>
     *                     <li>{@code data} is {@code null}.</li>
     *                 </ul>
     *             </li>
     *             <li><strong>Failure:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code false}.</li>
     *                     <li>{@code message} contains the error description, possibly returned by server.</li>
     *                     <li>{@code data} contains the error code returned by the server encapsulated in a {@link TcpGeneral} Object, if there is any returned by server.</li>
     *                 </ul>
     *             </li>
     *         </ul>
     */
    public WebReturn<Integer> set(String key, TcpGeneral value, boolean force) {
        if (key == null || key.isEmpty()) {
            return WebReturn.error("Key must not be null or empty.");
        }

        try {
            if (force) writer.write(Utility.FORCED_SET_COMMAND);
            else writer.write(Utility.SET_COMMAND);

            writer.flush();

            dataWriter.writeString(key);
            dataWriter.writeGeneral(value);

            byte response = reader.readByte();
            if (response == Utility.NAK) {
                return readError();
            } else if (response == Utility.ACK) {
                return WebReturn.success();
            } else return WebReturn.error("Server responded with unknown response.", (int) response);
        } catch (IOException e) {
            return WebReturn.error("Failed to send set command.");
        } catch (TCPClientException e) {
            return WebReturn.error(e.getMessage());
        }
    }

    /**
     * Sends a command to the server to delete a key-value pair.
     *
     * @param key The key of the value to be deleted from the server.
     * @return A {@link WebReturn} object indicating the outcome:
     *         <ul>
     *             <li>Success:
     *                 <ul>
     *                     <li>{@code ok} is {@code true}.</li>
     *                     <li>{@code message} is {@code null}.</li>
     *                     <li>{@code data} is {@code null}.</li>
     *                 </ul>
     *             </li>
     *             <li><strong>Failure: </strong>
     *                  <ul>
     *                      <li>See {@link #readError()}</li>
     *                  </ul>
     *             </li>
     *         </ul>
     */
    public WebReturn<Integer> delete(String key) {
        try {
            writer.write(Utility.DELETE_COMMAND);
            writer.flush();

            dataWriter.writeString(key);


            byte response = reader.readByte();
            if (response == Utility.NAK) {
                return readError();
            } else if (response == Utility.ACK) {
                return WebReturn.success();
            } else return WebReturn.error("Server responded with unknown response.", (int) response);
        } catch (IOException e) {
            return WebReturn.error("Failed to send delete command.");
        } catch (TCPClientException e) {
            return WebReturn.error(e.getMessage());
        }
    }

    /**
     * Sends a command to the server to update the scope permissions of a user.
     *
     * @param username   The username of the user whose scope permissions are to be updated.
     * @param key        The scope key to be added, edited, or deleted.
     * @param options    The permission options for the scope:
     *                   <ul>
     *                       <li>0x01: read-permission</li>
     *                       <li>0x02: write-permission</li>
     *                       <li>0x04: delete-permission</li>
     *                   </ul>
     * @return A {@link WebReturn} object indicating the outcome:
     *         <ul>
     *             <li><strong>Success:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code true}.</li>
     *                     <li>{@code message} is {@code null}.</li>
     *                     <li>{@code data} is {@code null}.</li>
     *                 </ul>
     *             </li>
     *             <li><strong>Failure: </strong>
     *                  <ul>
     *                      <li>See {@link #readError()}</li>
     *                  </ul>
     *             </li>
     *         </ul>
     */
    public WebReturn<Integer> updateScope(String username, String key, byte options) {
        try {
            writer.write(Utility.UPDATE_SCOPE_COMMAND);

            writer.flush();

            dataWriter.writeString(username);
            dataWriter.writeString(key);

            writer.write(options);

            byte response = reader.readByte();

            if (response == Utility.ACK) {
                return WebReturn.success();
            } else if (response == Utility.NAK) {
                return readError();
            } else return WebReturn.error("Server responded with unknown response", (int) response);

        } catch (IOException e) {
            return WebReturn.error("Failed to send update scope command.");
        } catch (TCPClientException e) {
            return WebReturn.error(e.getMessage());
        }
    }

    /**
     * Sends a command to the server to create a new user. A new user can be created as an administrator.
     *
     * @param username   The username of the user to be created.
     * @param password   The password of the user to be created.
     * @param options    The permission options:
     *                   <ul>
     *                       <li>0x01: new user will be administrator</li>
     *                   </ul>
     * @return A {@link WebReturn} object indicating the outcome:
     *         <ul>
     *             <li><strong>Success:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code true}.</li>
     *                     <li>{@code message} is {@code null}.</li>
     *                     <li>{@code data} is {@code null}.</li>
     *                 </ul>
     *             </li>
     *             <li><strong>Failure: </strong>
     *                  <ul>
     *                      <li>See {@link #readError()}</li>
     *                  </ul>
     *             </li>
     *         </ul>
     */
    public WebReturn<Integer> createUser(String username, String password, byte options) {
        try {
            writer.write(Utility.CREATE_USER_COMMAND);

            writer.flush();

            dataWriter.writeString(username);
            dataWriter.writeString(password);

            writer.write(options);


            byte response = reader.readByte();

            if (response == Utility.ACK) {
                return WebReturn.success();
            } else if (response == Utility.NAK) {
                return readError();
            } else return WebReturn.error("Server responded with unknown response", (int) response);
        } catch (IOException e) {
            return WebReturn.error("Failed to send update scope command.");
        } catch (TCPClientException e) {
            return WebReturn.error(e.getMessage());
        }
    }

    /**
     * Sends a PING command to the server to verify communication and synchronization.
     *
     * @param packageToSend The unsigned integer data payload to send with the PING command.
     * @return A {@link WebReturn} object indicating the outcome of the PING operation:
     *         <ul>
     *             <li><strong>Success:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code true}.</li>
     *                     <li>{@code message} is {@code null}.</li>
     *                     <li>{@code data} is {@code null}.</li>
     *                 </ul>
     *             </li>
     *             <li><strong>Failure: </strong>
     *                  <ul>
     *                      <li>See {@link #readError()}</li>
     *                  </ul>
     *             </li>
     *         </ul>
     */
    public WebReturn<Integer> ping(long packageToSend) {
        try {
            writer.write(Utility.PING_COMMAND);
            writer.flush();

            dataWriter.writeUnsignedInt(packageToSend);

            byte response = reader.readByte();
            if (response == Utility.ACK) {
                long responsePackage = dataReader.readUnsignedInt();
                if (responsePackage == packageToSend) return WebReturn.success();
                else return WebReturn.error("Server responded with " + responsePackage + " which is not equal to " + packageToSend + ".");
            } else if (response == Utility.NAK) {
                return readError();
            } else return WebReturn.error("Server responded with unknown response", (int) response);
        } catch (IOException e) {
            return WebReturn.error("Failed to send ping command.");
        } catch (TCPClientException e) {
            return WebReturn.error(e.getMessage());
        }
    }

    /**
     * Sends a STOP command to the server to halt any ongoing data transmission.
     *
     * @return A {@link WebReturn} object indicating the outcome of the STOP operation:
     *         <ul>
     *             <li><strong>Success:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code true}.</li>
     *                     <li>{@code message} is {@code null}.</li>
     *                     <li>{@code data} is {@code null}.</li>
     *                 </ul>
     *             </li>
     *             <li><strong>Failure: </strong>
     *                  <ul>
     *                      <li>See {@link #readError()}</li>
     *                  </ul>
     *             </li>
     *         </ul>
     */
    public WebReturn<Integer> stop() {
        try {
            writer.writeByte(Utility.STOP_COMMAND);
            writer.flush();

            final int MAX_BYTES_TO_READ = 1024; // Maximum number of bytes to read before giving up
            int bytesRead = 0;

            while (bytesRead < MAX_BYTES_TO_READ) {
                if (reader.available() > 0) {
                    byte responseByte = reader.readByte();
                    bytesRead++;

                    if (responseByte == Utility.ACK) {
                        return WebReturn.success();
                    } else if (responseByte == Utility.NAK) {
                        return readError();
                    }
                } else {
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        return WebReturn.error("Thread interrupted while waiting for STOP response.");
                    }
                }
            }

            return WebReturn.error("ACK/NAK not received after sending STOP command.");
        } catch (IOException e) {
            return WebReturn.error("Failed to send STOP command.");
        } catch (TCPClientException e) {
            return WebReturn.error(e.getMessage());
        }
    }

    /**
     * Sends disconnect to the server and closes the socket connection to the server.
     */
    public void disconnect() {
        try {
            writer.write(Utility.DISCONNECT_COMMAND);
            writer.flush();
            this.socket.close();
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    /**
     * Reads a server error package from the input stream.
     *
     * @return A {@link WebReturn} object containing the error details:
     *         - {@code ok}: {@code false}.
     *         - {@code message}: The error message returned by the server.
     *         - {@code data}: contains the error code returned by the server.
     * @throws TCPClientException If an error occurs while reading the server error string.
     * @throws IOException If an I/O error occurs while reading the server error.
     */
    private WebReturn<Integer> readError() throws TCPClientException, IOException {
        Optional<Pair<String, Integer>> errorOpt = this.dataReader.readServerError(false);
        if (errorOpt.isPresent()) {
            Pair<String, Integer> error = errorOpt.get();
            return WebReturn.error(error.getLeft(), error.getRight());
        } else {
            throw new TCPClientException("Received NAK but failed to read server error.");
        }
    }

}