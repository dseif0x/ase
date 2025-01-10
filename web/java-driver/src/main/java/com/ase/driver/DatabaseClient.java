package com.ase.driver;

import com.ase.exception.HandshakeException;
import com.ase.exception.ServerErrorException;
import com.ase.exception.TCPClientException;
import com.ase.utility.*;
import com.ase.utility.tcp.SimpleType;
import com.ase.utility.tcp.TcpErrorCode;
import com.ase.utility.tcp.TcpGeneral;
import com.ase.utility.tcp.TcpGeneralType;
import org.apache.commons.lang3.tuple.Pair;

import java.io.*;
import java.net.Socket;
import java.util.*;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicBoolean;

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
    /** Writer for writing data to the server based on server-provided size information. */
    private DynamicDataWriter dataWriter;

    private final AtomicBoolean isStopped = new AtomicBoolean(false);

    /** Executor service for handling asynchronous operations. */
    private final ExecutorService executorService = Executors.newCachedThreadPool();

    /** List of ongoing futures for asynchronous get and set operations. */
    private final List<Future<?>> ongoingFutures = Collections.synchronizedList(new ArrayList<>());

    /**
     * Constructs a new DatabaseClient and establishes a socket connection.
     *
     * @param host The hostname or IP address of the database server
     * @param port The port number to connect to
     * @throws IOException If there is an error creating the socket or streams
     */
    public DatabaseClient(String host, int port) throws IOException {
        this(new Socket(host, port));
    }

    /**
     * Constructs a new DatabaseClient using an existing socket connection.
     *
     * @param socket The socket connection to the database server
     * @throws IOException If there is an error creating the streams
     */
    DatabaseClient(Socket socket) throws IOException {
        this.socket = socket;
        this.reader = new DataInputStream(socket.getInputStream());
        this.writer = new DataOutputStream(socket.getOutputStream());
    }

    /**
     * Adds a Future to the list of ongoing get/set operations.
     *
     * @param future The Future to add.
     */
    private void addOngoingFuture(Future<?> future) {
        ongoingFutures.add(future);
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
                throw new ServerErrorException(errorCodeEnum.getCode(), errorCodeEnum.getDescription());
            } else if (response == Utility.ACK) {
                ServerInformation serverInfo = ServerInformation.readServerInformation(reader);
                if (!serverInfo.validate()) {
                    writer.write(Utility.NAK);
                    throw new HandshakeException("Validation failed for server information. Sizes of server are too large.");
                } else {
                    this.dataReader = new DynamicDataReader(serverInfo, reader, isStopped);
                    this.dataWriter = new DynamicDataWriter(serverInfo, writer, isStopped);
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

            while (true) {
                if (isStopped.get()) {
                    return WebReturn.error("GET operation was stopped.");
                }

                int available = reader.available();
                if (available > 0) {
                    byte response = reader.readByte();
                    if (response == Utility.ACK) {
                        TcpGeneral tcpGeneral = dataReader.readGeneral();
                        return WebReturn.success(tcpGeneral);
                    } else if (response == Utility.NAK) {
                        WebReturn<Integer> error = readError();
                        return WebReturn.error(error.getMessage(), TcpGeneral.ofSimple(SimpleType.INT, error.getData()));
                    } else {
                        return WebReturn.error("Server responded with unknown response.", TcpGeneral.ofSimple(SimpleType.INT, (int) response));
                    }
                } else {
                    Thread.sleep(50);
                }
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return WebReturn.error("GET operation interrupted.");
        } catch (TCPClientException | IOException e) {
            if (isStopped.get()) {
                return WebReturn.error("GET operation was stopped.");
            }
            return WebReturn.error("Failed to send GET command: " + e.getMessage());
        }
    }

    /**
     * Asynchronously performs a GET operation.
     * @see #get(String) get() for more Information.
     *
     * @param key The key to retrieve.
     * @return A Future representing the pending result of the GET operation.
     */
    public Future<WebReturn<TcpGeneral>> getAsync(String key) {
        Callable<WebReturn<TcpGeneral>> task = () -> get(key);
        Future<WebReturn<TcpGeneral>> future = executorService.submit(task);
        addOngoingFuture(future);
        return future;
    }

    /**
     * Asynchronously performs a SET operation.
     * @see #set(String, TcpGeneral, boolean) set() for more Information.
     *
     * @param key    The key to set.
     * @param value  The value to associate with the key.
     * @param force  Whether to force the set operation.
     * @return A Future representing the pending result of the SET operation.
     */
    public Future<WebReturn<Integer>> setAsync(String key, TcpGeneral value, boolean force) {
        Callable<WebReturn<Integer>> task = () -> set(key, value, force);
        Future<WebReturn<Integer>> future = executorService.submit(task);
        addOngoingFuture(future);
        return future;
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
        } else if (value == null) {
            return WebReturn.error("Value must not be null.");
        }

        try {
            if (force) writer.write(Utility.FORCED_SET_COMMAND);
            else writer.write(Utility.SET_COMMAND);

            writer.flush();

            dataWriter.writeString(key);
            dataWriter.writeGeneral(value);

            while (true) {
                if (isStopped.get()) {
                    return WebReturn.error("SET operation was stopped.");
                }

                int available = reader.available();
                if (available > 0) {
                    byte response = reader.readByte();
                    if (response == Utility.ACK) {
                        return WebReturn.success();
                    } else if (response == Utility.NAK) {
                        return readError();
                    } else {
                        return WebReturn.error("Server responded with unknown response.", (int) response);
                    }
                } else {
                    Thread.sleep(50);
                }
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return WebReturn.error("SET operation interrupted.");
        } catch (TCPClientException | IOException e) {
            if (isStopped.get()) {
                return WebReturn.error("SET operation was stopped.");
            }
            return WebReturn.error("Failed to send SET command: " + e.getMessage());
        }

    }

    /**
     * Sends the LIST command asynchronously.
     *
     * @param key The key to list sub-keys for.
     * @return A Future representing the pending result of the LIST operation.
     */
    public Future<WebReturn<TcpGeneral>> listAsync(String key) {
        Callable<WebReturn<TcpGeneral>> task = () -> {
            return list(key);
        };
        Future<WebReturn<TcpGeneral>> future = executorService.submit(task);
        addOngoingFuture(future);
        return future;
    }

    /**
     * Sends the LIST command synchronously.
     *
     * @param key The key to list sub-keys for.
     * @return A WebReturn containing the map of sub-keys to TcpGeneral values.
     */
    public WebReturn<TcpGeneral> list(String key) {
        if (key == null || key.isEmpty()) {
            return WebReturn.error("Key must not be null or empty.");
        }

        if (isStopped.get()) {
            return WebReturn.error("Operation aborted due to stop command.");
        }

        try {
            writer.writeByte(Utility.LIST_COMMAND);
            dataWriter.writeString(key);
            writer.flush();

            byte response = reader.readByte();

            if (response == Utility.ACK) {
                TcpGeneral keyMap = dataReader.readGeneral();
                return WebReturn.success(keyMap);
            } else if (response == Utility.NAK) {
                WebReturn<Integer> error = readError();
                return WebReturn.error(error.getMessage(), TcpGeneral.ofSimple(SimpleType.INT, error.getData()));
            } else {
                return WebReturn.error("Server responded with unknown response", TcpGeneral.ofSimple(SimpleType.INT, (int) response));
            }

        } catch (IOException e) {
            if (isStopped.get()) {
                return WebReturn.error("LIST operation was stopped.");
            }
            return WebReturn.error("Failed to send LIST command: " + e.getMessage());
        } catch (TCPClientException e) {
            if (isStopped.get()) {
                return WebReturn.error("LIST operation was stopped.");
            }
            return WebReturn.error("Failed to execute LIST command: " + e.getMessage());
        }
    }

    /**
     * Sends a FOLLOW command to the server, either SAFE or FORCED based on the provided flag.
     *
     * @param key       The reference key to follow.
     * @param maxJumps  The maximum number of jumps to follow.
     * @param isSafe    {@code true} for SAFE FOLLOW, {@code false} for FORCED FOLLOW.
     * @return A {@link WebReturn} object indicating the outcome:
     *         <ul>
     *             <li><strong>Success:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code true}.</li>
     *                     <li>{@code message} is {@code null}.</li>
     *                     <li>{@code data} contains the {@link TcpGeneral} value of the result.</li>
     *                 </ul>
     *             </li>
     *             <li><strong>Failure:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code false}.</li>
     *                     <li>{@code message} contains the error description.</li>
     *                     <li>{@code data} contains the error code encapsulated in a {@link TcpGeneral} Object, if applicable.</li>
     *                 </ul>
     *             </li>
     *         </ul>
     */
    public WebReturn<TcpGeneral> follow(String key, long maxJumps, boolean isSafe) {
        if (key == null || key.isEmpty()) {
            return WebReturn.error("Key must not be null or empty.");
        }
        if (maxJumps <= 0) {
            return WebReturn.error("Invalid maxJumps value. It must be greater than 0.");
        }

        try {
            writer.write(isSafe ? Utility.FOLLOW_COMMAND : Utility.FORCE_FOLLOW_COMMAND);
            writer.flush();

            dataWriter.writeString(key);

            byte[] tcpEUI = Utility.createTCPEUIByteArray(maxJumps);
            writer.write(tcpEUI);

            byte response = reader.readByte();
            if (response == Utility.ACK) {
                TcpGeneral result = dataReader.readGeneral();
                return WebReturn.success(result);
            } else if (response == Utility.NAK) {
                WebReturn<Integer> error = readError();
                return WebReturn.error(error.getMessage(), TcpGeneral.ofSimple(SimpleType.INT, error.getData()));
            } else {
                return WebReturn.error("Server responded with unknown response", TcpGeneral.ofSimple(SimpleType.INT, (int) response));
            }
        } catch (IOException e) {
            return WebReturn.error("Failed to send FOLLOW command.");
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

    /** Sends a command to the server to change a user's password.
     *
     * @param username The username of the user whose password is to be changed.
     * @param password The new password for the user.
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
    public WebReturn<Integer> changePassword(String username, String password) {
        try {
            writer.write(Utility.CHANGE_PASSWORD_COMMAND);

            writer.flush();

            dataWriter.writeString(username);
            dataWriter.writeString(password);

            byte response = reader.readByte();

            if (response == Utility.ACK) {
                return WebReturn.success();
            } else if (response == Utility.NAK) {
                return readError();
            } else return WebReturn.error("Server responded with unknown response", (int) response);
        } catch (IOException e) {
            return WebReturn.error("Failed to send change password command.");
        } catch (TCPClientException e) {
            return WebReturn.error(e.getMessage());
        }
    }

    /**
     * Sends a command to the server to delete a user.
     *
     * @param username The username of the user to be deleted.
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
    public WebReturn<Integer> deleteUser(String username) {
        try {
            writer.write(Utility.DELETE_USER_COMMAND);

            writer.flush();

            dataWriter.writeString(username);

            byte response = reader.readByte();

            if (response == Utility.ACK) {
                return WebReturn.success();
            } else if (response == Utility.NAK) {
                return readError();
            } else return WebReturn.error("Server responded with unknown response", (int) response);
        } catch (IOException e) {
            return WebReturn.error("Failed to send delete user command.");
        } catch (TCPClientException e) {
            return WebReturn.error(e.getMessage());
        }
    }

    /**
     * Requests the permissions for the specified users from the server.
     * Users requesting this information for others must have administrator privileges.
     *
     * @param usernames The list of usernames to query permissions for. If the list is empty, permissions for all users are requested.
     * @return A {@link WebReturn} object containing the permissions as a {@link TcpGeneral}.
     *         <ul>
     *             <li><strong>Success:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code true}.</li>
     *                     <li>{@code message} is {@code null}.</li>
     *                     <li>{@code data} contains the permissions as a {@link TcpGeneral}.</li>
     *                 </ul>
     *             </li>
     *             <li><strong>Failure:</strong>
     *                 <ul>
     *                     <li>{@code ok} is {@code false}.</li>
     *                     <li>{@code message} contains the error description.</li>
     *                     <li>{@code data} contains the error code if applicable.</li>
     *                 </ul>
     *             </li>
     *         </ul>
     */
    public WebReturn<TcpGeneral> getUserPermissions(TcpGeneral usernames) {
        try {
            if (usernames == null) {
                return WebReturn.error("Usernames must not be null.");
            }

            if (usernames.getType() != TcpGeneralType.ARRAY) {
                return WebReturn.error("Usernames must be of type array.");
            }

            writer.writeByte(Utility.GET_USER_PERMISSIONS_COMMAND);
            if (usernames.getArray().isEmpty()) {
                writer.writeByte(0);
            } else {
                dataWriter.writeGeneral(usernames);
            }

            byte response = reader.readByte();

            if (response == Utility.ACK) {
                TcpGeneral result = dataReader.readGeneral();
                return WebReturn.success(result);
            } else if (response == Utility.NAK) {
                WebReturn<Integer> error = readError();
                return WebReturn.error(error.getMessage(), TcpGeneral.ofSimple(SimpleType.INT, error.getData()));
            } else {
                return WebReturn.error("Server responded with unknown response", TcpGeneral.ofSimple(SimpleType.INT, (int) response));
            }
        } catch (IOException e) {
            return WebReturn.error("Failed to send GET USER PERMS command.");
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
     * @param packageToSend The unsigned integer data payload to send with the STOP command. Used to check synchronization.
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
    public WebReturn<Integer> stop(long packageToSend) {
        if (isStopped.getAndSet(true)) {
            return WebReturn.error("Stop command has already been issued.");
        }

        synchronized (ongoingFutures) {
            for (Future<?> future : ongoingFutures) {
                future.cancel(true);
            }
            ongoingFutures.clear();
        }

        try {
            writer.writeByte(Utility.STOP_COMMAND);
            writer.flush();

            int bytesRead = 0;

            while (bytesRead < Utility.MAX_BYTES_TO_READ) {
                if (reader.available() > 0) {
                    byte responseByte = reader.readByte();
                    bytesRead++;

                    if (responseByte == Utility.ACK) {
                        WebReturn<Integer> pingResult = ping(packageToSend);
                        isStopped.set(false);

                        if (pingResult.isOk()) {
                            return WebReturn.success();
                        } else {
                            return WebReturn.error("Ping failed and thus not in sync with server. " +
                                "Restart may be needed (try ping again). Error: " + pingResult.getMessage(), pingResult.getData());
                        }
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


    /**
     * Sets the data reader for the client.
     *
     * @param dataReader The data reader to set.
     */
    public void setDataReader(DynamicDataReader dataReader) {
        this.dataReader = dataReader;
    }

    /**
     * Sets the data writer for the client.
     *
     * @param dataWriter The data writer to set.
     */
    public void setDataWriter(DynamicDataWriter dataWriter) {
        this.dataWriter = dataWriter;
    }
}