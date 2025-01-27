 /**
  * @page tcp_protocol TCP Communication Protocol - T(CP)^2
  *
  * ## General - Work in Progress - v1.13 (17.1.2025)
  *
  * The purpose of this protocol is to define the communication between the ASE-Core and TCP drivers of different applications.
  *
  * For the transmission of information over TCP the values are wrapped into types named `tcp_<type>` as can be seen with more details later on.
  *
  * ## Connection Initiation & Handshake
  *
  * ### Overview
  *
  * The ASE-Core, from now on also called **Server**, will listen for incoming TCP connections from **Clients**. After a client is done creating the connection, they will sind a Information Package to the server. If the server finds the information acceptable, they will send an ACK followed by their Information-Package. If the server finds the client unacceptable, they will send a NAK followed by a server-error-package. If the client finds the server acceptable, they will answer with ACK, otherwise with NAK. There is no error-code necessary, as the server really does not care.
  *
  * ```mermaid
  * sequenceDiagram
  * 	Client-->>Server: Initiate TCP-Connection
  * 	Client->>Server: Client Information
  * 	Server->>Client: ACK & Server Information
  * 	Client->>Server: ACK & User Information
  * 	Server->>Client: ACK
  *
  * ```
  *
  * **Handshake Step by Step:** 0. The TCP connection is established.
  *
  * 1. The client sends its client-information to the server.
  * 2. The server either accepts it (continues on 3.) or answers with NAK and an server-error-package.
  * 3. If the server accepts the package, it will answer with ACK and the server-information.
  * 4. If the client accepts the servers information, it will answer with another ACK followed by the users information. If the client does not accept, it sends a NAK to the server, but no error message is needed and the connection can be closed right after.
  * 5. If the server can authenticate the user, it will answer with ACK and the connection initiation handshake is complete.
  *
  * ### Packages
  *
  * #### ACK and NAK
  *
  * These two packages are just 1 Byte each, containing the 1s or 0s, representing success and failure.
  *
  * ```txt
  * ACK: 1111.1111 = Success
  * NAK: 0000.0000 = Failure
  * ```
  *
  * #### Client Information Package
  *
  * This package contains all information needed to convince the server that the client is able to handle the connection as well as some additional information for the server. The access permissions are a way for the client to avoid accidentally doing something unwanted. This allows a user which for example has the delete permissions to open a connection with only a read permission and getting permission errors if write or delete operations are executed.
  *
  * | Name | Type | Description |
  * |------|------|-------------|
  * | version | `tcp_ui16` | The DB version, the client is expecting. |
  * | flags | `tcp_bits8` | Binary Options explained below. |
  * | read-access | `0x01` | True if the client wants to read data. |
  * | write-access | `0x02` | True if the client wants to write data. |
  * | delete-permission | `0x04` | True if the client wants to delete data. |
  * | admin-permission | `0x08` | True if the client wants to edit the acl. |
  *
  * ```txt
  * [tcp_ui16][tcp_bits8]
  *
  * 0                16
  * +----------------+--------+
  * | version        |....ADWR|
  * +----------------+--------+
  * ```
  *
  * | Error Code | Name | Message Text | Description |
  * |------------|------|--------------|-------------|
  * | 0xFFFF | General Protocol Error | \<can be anything\> | Is returned in case some unexpected error occurred. |
  * | 0xFFF0 | Permission Denied (self) | Connection denies permission. | The connection was opened in a specific way, which does not allow the used operation. |
  * | 0xFF01 | Invalid Version | Invalid client version. | Is used to tell the user to change their client version |
  *
  * #### Server Information Package
  *
  * This package contains all information needed for the client to decide if communication is possible with the server.
  *
  * | Name | Type | Description |
  * |------|------|-------------|
  * | boolean size | `tcp_ui8` | Boolean size in Bytes. |
  * | integer size | `tcp_ui8` | Integer size in Bytes. |
  * | char size | `tcp_ui8` | Char size in Bytes. |
  * | double size | `tcp_ui8` | Double size in Bytes. |
  * | semaphore size | `tcp_ui8` | Semaphore value size in Bytes. |
  * | max string size | `tcp_ui16` | The maximal length of a string. |
  * | max array length | `tcp_ui16` | The maximal length of an array. |
  *
  * ```txt
  * 0        8        16       24       32       40               56
  * +--------+--------+--------+--------+--------+----------------+----------------+
  * | bool   | int    | char   | double | sem    | max string len | max array len  |
  * +--------+--------+--------+--------+--------+----------------+----------------+
  * ```
  *
  * ### User Information Package
  *
  * This package is used to authenticate the user using the client to the server. The server requires the username and password of the user.
  *
  * ```txt
  * [tcp_str][tcp_str]
  * username pwd
  * ```
  *
  * | Error Code | Name | Message Text | Description |
  * |------------|------|--------------|-------------|
  * | 0xFF02 | user not found. | There is no user with that username. |  |
  * | 0xFF03 | wrong password | The provided password is invalid. |  |
  * | 0xFF04 | already connected | The provided user is already connected to the DB. |  |
  *
  * ### Generic Server Error Package
  *
  * Is used to communicate an error from the server to the client. It includes an error code followed by a string to be shown to the user.
  *
  * ```txt
  * [tcp_ui16][tcp_str]
  * ```
  *
  * ## TCP Datatypes
  *
  * To not waste precious bandwidth, all data is efficiently packaged into binary representations of themselves. The types are carefully designed to perform well in most programming languages, if used correctly. All types if not otherwise stated use big-endian and MSB-first.
  *
  * ### Data typing (Type-Definition-Block)
  *
  * If it is not known, what type a specific value has, it has to be transmitted alongside the value itself. Therefore the type-definition-block exists. It is a self expanding block which allows to type any kind of value. It is at least on Byte large, but may be larger depending on the represented type. If the value is a data structure, like for example a pair of arrays, type-definition-blocks will be used in multiple points in the structure to define all values right before the data itself.
  *
  * #### Type Definition
  *
  * If the value is an array or a pair, the corresponding flags must be set. It is **not possible** to set the flags for array and pair at the same time. Only if the array or pair flag is set, the complexity flag is checked. The complexity flag is set, in the following instances:
  *
  * - Array flag is set and the array includes complex types -\> array, pair
  * - Pair flag is set and the pair's first value is a complex types -\> array, pair
  * - Array flag is set and the array consists of multiple data types (regardless of complex or simple types) In the case of the Array: If the complexity flag is not set, the value field declares the type of all values. If the complexity flag is set, each value will receive its own complexity flag. In the case of the Pair: If the complexity flag is not set, the value field declares the type of the first value. If the complexity flag is set, the first value will receive its own complexity flag. For both cases, there will be another type-definition-block for the second value. **Options:**
  * - If the array flag is set and the complex flag is not set, the value-field defines the simple type of the array.
  * - If the array flag is set and the complexity flag is set, each value will have its own type-definition-block.
  * - If the pair flag is set and the complexity flag is not set, the value-field defines the type of the first value of the pair. Another type-definition block follows for the second value.
  * - If the pair flag is set and the complexity flag is set, the type of the first value is set by a type-definition block following right after. For the second value another type-definition block is used.
  * - If neither the pair nor the array flags are set, the complexity flag does not have to be checked and the value is a **simple type**. Simple types include all types, which have a defined length. Strings are also simple types, even though they may be of variable length.
  *
  * If the key-flag is used, the value is a key-map-type which will be followed by a length (tcp_eui) which describes the amount of keys in it. After that a key (tcp_str) follows, and after that a type-definition-block describing the value for the key.
  *
  * If the type-only flag is used, the Type-definition block only describes the type of a value and the actual value does not follow. This is **not permitted** by default and must be only used in specific commands like the LIST-command. Therefore this flag may only be checked in the specifically defined situations which are described in the affected commands.
  *
  * | Name | Type/Value | Description |
  * |------|------------|-------------|
  * | is array | `flag: 0x80` | If True, the value is an array. |
  * | is pair | `flag: 0x40` | If True, the value is a pair. |
  * | is complex | `flag: 0x20` | If True, array or pair is complex. |
  * | is key | `flag: 0x10` | If True, key follow instead of value. |
  * | is only type | `flag: 0x08` | If True, Type is defined, but no values follow. |
  * | is reference | `val: 6` | Value is of type reference. |
  * | is semaphore | `val: 5` | Value is of type semaphore. |
  * | is double | `val: 4` | Value is of type double. |
  * | is string | `val: 3` | Value is of type string. |
  * | is int | `val: 2` | Value is of type int. |
  * | is boolean | `val: 1` | Value is of type boolean. |
  *
  * ```txt
  * 0        8
  * +--------+
  * |APCKTval|
  * +--------+
  *
  * [tcp_bits5][tcp_ui3] -> 8 Bit
  * ```
  *
  * ### Integers: `tcp_i<len>, tcp_ui<len>, tcp_int`
  *
  * Integers are exactly what we think they are. `tcp_i<len>`represents signed integers and `tcp_ui<len>` represent unsigned integers. They are mostly used by the protocol itself and always have the defined length in bits. `tcp_int` is an integer of the size defined by the DB. The size of this type is communicated to the client in the Server Information Package.
  *
  * ### Self expanding Integers `tcp_eui`
  *
  * In some cases to not wast precious bandwidth we will use self expanding integers, similar to what UTF-8 encoding does. The base size is always one Byte, and it can expand up to 9 Bytes. The amount of Bits set to 1 at the beginning of the Byte defines how many other bytes follow. The rest of the first byte as well as all bits of the following Bytes are used to store the value. For simplicity all values always start at 0, independent of how many bytes are used.
  *
  * ```txt
  * x: value bits
  *
  * 0xxx.xxxx                     0-127
  * 10xx.xxxx xxxx.xxxx           0-16384
  * 110x.xxxx xxxx.xxxx xxxx.xxxx 0-2097151
  * ...                           ...
  * ```
  *
  * ### Strings & Arrays: `tcp_str, tcp_arr_<type>`
  *
  * An array declares a length using the expanding `tcp_eui` type and follows it up with the data. `tcp_str`is equivalent to `tcp_arr_char`. For both of these types the simple data type which fills the array is already known. Usually, the contents of an array are not known before hand, and therefore a general variable will be used.
  *
  * ```txt
  * [tcp_eui][val 0][val 1][val ...]
  *
  * Example tcp_str "HI!"
  * 0000.0011 0100.1000 0100.1001 0010.0001
  * len: 3    val0: H   val1: I   val2: !
  *
  * Array Example for General Value:
  * In this exammples the integers are only 8Bits.
  *
  * Data: [[1, 2], [3, 4, 5]]
  *
  *    0        8        16       24             Byte 0: array & complex
  *    +--------+--------+--------+--------+     Byte 1:   len(array) = 2
  * 0  |1010.???|00000010|1000.010|00000010|     Byte 2:   array & !complex & int
  *    +--------+--------+--------+--------+     Byte 3:     len(array) = 2
  * 32 |00000001|00000010|1000.010|00000011|     Byte 4-5:   values: 1, 2
  *    +--------+--------+--------+--------+     Byte 6:   array & !complex & int
  * 64 |00000011|00000100|00000101|        |     Byte 7:     len(array) = 3
  *    +--------+--------+--------+--------+     Byte 8-10:  values: 3, 4, 5
  *
  *
  * Data: [["AB", "CD"], 4]
  *
  *    0        8        16       24             Byte 0: array & complex
  *    +--------+--------+--------+--------+     Byte 1:   len(array) = 2
  * 0  |1010.???|00000010|1000.011|00000010|     Byte 2:   array & !complex & str
  *    +--------+--------+--------+--------+     Byte 3:     len(array) = 2
  * 32 |00000010|01000001|01000010|00000010|     Bste 4:     len(str) = 2
  *    +--------+--------+--------+--------+     Byte 5-6:     values: 'A', 'B'
  * 64 |01000011|01000100|0000.010|00000100|     Byte 7:     len(str) = 2
  *    +--------+--------+--------+--------+     Byte 8-9:     values: 'C', 'D'
  *                                              Byte 10:  type int
  *                                              Byte 11:    value: 4
  * ```
  *
  * ### Pair:
  *
  * The pair is able to carry two different values of each other. Pairs are not used internally, so here are only examples of Pairs as General Values. Nested pairs are stored as a tree which can be correctly reassembled with a depth search.
  *
  * ```txt
  *
  * Pair Examples for General Value:
  * In this example the integers are only 8 Bits.
  *
  * Data: (1, 2)
  *
  *    0        8        16       24             Byte 0: pair & !complex & int
  *    +--------+--------+--------+--------+     Byte 1:   value 1: 1
  * 0  |0100.010|00000001|00?0.010|00000010|     Byte 2:   type v2: int
  *    +--------+--------+--------+--------+     Byte 3:   value 2: 2
  * 32 |        |        |        |        |     Byte 4
  *    +--------+--------+--------+--------+     Byte 6
  * 64 |        |        |        |        |     Byte 7
  *    +--------+--------+--------+--------+     Byte 8
  *
  *
  * Data: (("A", 1), ("B", [2, 3]))
  *
  *    0        8        16       24             Byte 0: pair & complex
  *    +--------+--------+--------+--------+     Byte 1:   type v1: pair & !c & str
  * 0  |0110.???|0100.011|00000001|01000001|     Byte 2:     len(str) = 1
  *    +--------+--------+--------+--------+     Byte 3:       values: 'A'
  * 32 |00?0.010|00000001|0110.???|00?0.011|     Byte 4:     type v2: int
  *    +--------+--------+--------+--------+     Byte 5:     value 2: 1
  * 64 |00000001|01000010|1000.010|00000010|     Byte 6:   type v2: pair & complex
  *    +--------+--------+--------+--------+     Byte 7:     type v1: str
  * 96 |00000010|00000011|        |        |     Byte 8:     len(str) = 1
  *    +--------+--------+--------+--------+     Byte 9:       values: 'B'
  *                                              Byte 10:    type v2: arr & !c & int
  *                                              Byte 11:    len(array) = 2
  *                                              Byte 12-13:   values: 2, 3
  * ```
  *
  * ### General Value: `tcp_general`
  *
  * The general value is used to represent any type of data. It may be a simple integer or a nested array. It starts with a type-definition package and is followed with the data and other type definition packages if needed. This type is used for general information exchange between server and client. Examples on how this looks for arrays of pairs can be seen above.
  *
  * ### Examples for General Value:
  *
  * ```txt
  * Example containing keys:
  *
  * Data: {a1: [1, 2, 3], a2: {b1: "lol", b2: ("4", [5, "6"])}}
  *
  *              local root
  *             /          \
  *           a1           a2
  *          /           /    \
  *     [1, 2, 3]      b1     b2
  *                   /         \
  *                "lol"   ("4", [5, "6"])
  *
  * HEX representation:
  * 10 02 02 6131 82 03 00000001 00000002 00000003 02 6132 10 02 02 6231 03 03
  * 6c6f6c 02 6232 43 01 34 a0 02 02 00000005 03 01 36
  *
  * HEX Description:
  * 10 02 : first info block [00010000] showing a key structure, followed by count of keys
  * 02 6131 : tcp_str: len & val for first key
  *   82 03 00000001 00000002 00000003 : info block for first key-value [10000010] showing simple
  *                                       array with int vals; length of array followed by 3 4Byte ints
  * 02 6132 : tcp_str: len & val for second key
  *   10 02 : info block [00010000] showing a key structure, containing also 2 keys.
  *     02 6231 : first key
  *       03 03 6c6f6c : info block [00000011] showing string, len of string, value of string
  *     02 6232 : second key
  *       43 : info block [01000011] showing pair with basic first type (str)
  *         01 34 : length and value of string
  *         a0 02 : info block [10100000] showing complex array for value type of second pair value
  *                  followed by a array length
  *           02 00000005 : info block [00000010] showing int followed by int value
  *           03 01 36 : info block [00000011] showing str, str len, str val
  * ```
  *
  * ### List Type `tcp_list`
  *
  * The tcp list is used to describe an entire layer of key-value pairs of a specific key. It contains all sub-keys of the key as well as their value-types and their simple values. Complex values are usually not transmitted. To show which type definition blocks are followed by values, the T-Flag is used. If the T-flag is set, the type definition block describes a value, but all normally following information (the value for basic types, length for arrays, etc) is not present.
  *
  * ```
  * Example tcp_list:
  *
  *              local root
  *             /          \
  *           a1           a2
  *          /           /    \
  *     [1, 2, 3]      b1     b2
  *                   /         \
  *                "lol"   ("4", [5, "6"])
  *
  * Listing information for a2-Key:
  *
  * {b1: "lol"; b2: <pair>}
  *
  * HEX rep: 10 02 02 6231 03 03 6c6f6c 02 6232 48
  *
  * [00010000] 02 : map structure; 2 keys present
  *   02 6231 : key b1
  *     [00000011] 03 6c6f6c : str; data transmitted because Tflag not set; len 3; vals
  *   02 6232 : key b2
  *     [01001000] : pair + T-flag set -> no data follows
  *
  * ```
  *
  * ### ACL User Perms Type `tcp_perms`
  *
  * Used to return the user permissions to the requesting client. Contains all user permissions including if users are admins.
  *
  * ```
  * Example data tree:
  *
  *              local root
  *             /          \
  *           a1           a2
  *          /           /    \
  *     [1, 2, 3]      b1     b2
  *                   /         \
  *                "lol"   ("4", [5, "6"])
  * ```
  *
  * The structure is easy to understand. It is **wrapped in a tcp_general** and can be sent as such. We start with a map which contains the usernames as keys and their permission information as values. The permission information are represented as a pair of types bool and tcp_general. The boolean represents if the user is an administrator. The tcp general contains the permission of the user on the data tree. It always contains at least the permissions for the root directory. The permission for the current layer are saved as the value of the $-key. The $-symbol is an illegal character for creating keys in the tree-structure and is therefore always distinctly identifiable from other sub-keys. If there are other permissions on sub-levels, the sub-level-keys are available as keys which hold values for the next layer of permissions.
  *
  * ```
  * Example of permission representation:
  *
  * {
  * admin: (true, {$: 0x7}),
  * read_user: (false, {$: 0x01, a2: {$: 0x00, b1: {$: 0x02}}}),
  * test_user: (false, {$: 0x00, a1: {$: 0x07}})
  * }
  * ```
  *
  * ##### Permission evaluation:
  *
  * When checking if a user has certain permissions, we always start searching from top to bottom. As soon as all permissions are found, the tree does not have to be searched further. This means, that adding permissions is easy, but removing them is more difficult. For example, a user which has 0x07 (all) permissions on root, will never look further for his/her permissions than in the root $-key, as all permissions are granted there. If one would like to restrict the users permission, the root 0x07 permission would have to be overridden first.
  *
  * ##### Evaluation Examples:
  *
  * The user called admin has admin permissions (true) and read (0x01), write (0x02) and delete (0x04) permissions on the root and therefor for all data. The user read_user is not administrator and holds read rights over all data and write permissions over .a2.b1. test_user has all permissions for subtree .a1 but nothing else.
  *
  * Question: Can read_user execute a command requiring read&write permissions on key .a2.b1? Answer: Yes. We start at root and check the permissions: Read is found, so only write is missing. We traverse into a2 and check the permissions: No write permission; we continue to b1 and check: write is found -\> user has permission
  *
  * ## Commands:
  *
  * Each command can be identified by a specific Byte. The server is not in the business of correcting errors or checking that on the client side everything went good. If something went wrong, the client can simply repeat the request and deal with the problem on its end.
  *
  * ### STOP `0xFE`
  *
  * If the client sends the server a stop command, the server will stop whatever it is sending at the moment as soon as possible. The client may get data related to the previous command before the ACK for the stop comes through. It is currently only possible to stop the sending process of the server, not the actual calculations and activities in the background.
  *
  * ```txt
  * STOP Command-Byte: 0xFE (254)
  *
  * Client sends:
  *   [tcp_ui8(command)]
  *   example: 0xFE
  *
  * Server responds:
  *   {might be random bytes here}[tcp_ui8(ACK)]
  *   example: 0x12 0x34 0x56 0xFF
  * ```
  *
  * ### PING `0xFD`
  *
  * To make sure the server is still ready to communicate or to check if the server is synchronous with the clients commands, the ping command may be used. The client transmits 4 Bytes of data, which will be returned by the server in the same order after an ACK.
  *
  * ```txt
  * PING Command-Byte: 0xFD (253)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_ui32]
  *   example: 0xFD 0x11 0x22 0x33 0x44
  *
  * Server responds:
  *   [tcp_ui8(ACK)][tcp_ui32]
  *   example: 0xFF 0x11 0x22 0x33 0x44
  * ```
  *
  * ### DISCONNECT `0xFC`
  *
  * The server will close the connection immediately.
  *
  * ```txt
  * DISCONNECT Command-Byte: 0xFC (252)
  *
  * Client sends:
  *   [tcp_ui8(command)]
  *   example: 0xFC
  *
  * Server responds:
  *   Connection closed
  * ```
  *
  * ### BATCHING-ON `0xBA`
  *
  * The server will go into a mode where the writing takes place in batches. It only writes if the user sends a batching-flush command or batching mode is closed. During batching mode, only the db commands and batching control commands will be available. If unexpected errors appear, which destroy the consistency of the write or read buffer, an error message will be sent and informing which of the buffers was destroyed. If this happens some of the sent commands may already have been executed and other not. Try to avoid these errors at all cost, as the recovery process will take a lot of time, if at all possible.
  *
  * ```txt
  * DISCONNECT Command-Byte: 0xBA (186)
  *
  * Client sends:
  *   [tcp_ui8(command)]
  *   example: 0xBA
  *
  * Server responds:
  *   [tcp_ui8(ACK)]
  *   example: 0xFF
  * ```
  *
  * ### BATCHING-OFF `0xB0`
  *
  * The server will leave batching mode. Data left in the buffer will be written.
  *
  * ```txt
  * DISCONNECT Command-Byte: 0xB0 (176)
  *
  * Client sends:
  *   [tcp_ui8(command)]
  *   example: 0xB0
  *
  * Server responds:
  *   [tcp_ui8(ACK)]
  *   example: 0xFF
  * ```
  *
  * ### BATCHING-FLUSH `0xBF`
  *
  * The server will write the current buffered data.
  *
  * ```txt
  * DISCONNECT Command-Byte: 0xB0 (191)
  *
  * Client sends:
  *   [tcp_ui8(command)]
  *   example: 0xBF
  *
  * Server responds:
  *   [tcp_ui8(ACK)]
  *   example: 0xFF
  * ```
  *
  * ### GET `0x01`
  *
  * Get is used by the client to get the value of a key. The key must be provided for the request. There after, the server answers with ACK or NAK and the data in form of a `tcp_general` value. In case of a NAK, the server will add an error package. The user using the connection needs to have **read permissions** for the requested scope.
  *
  * ```txt
  * GET Command-Byte: 0x01 (1)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_str(key)]
  *   example: 0x01 "test.domain.and.key"
  *
  * Server responds:
  *   [tcp_ui8(ACK)][tcp_general]
  *   example: 0xFF (4, (2, [1, 2, "test"]))
  * OR
  *   [tcp_ui8(NAK)][error]
  *   example: 0x00 0x1234 "Test error text."
  * ```
  *
  * | Error Code | Name | Message Text | Description |
  * |------------|------|--------------|-------------|
  * | 0xFF00 | Permission denied | Permission denied. |  |
  * | 0x0100 | Invalid Key | Invalid key. |  |
  * | 0x0101 | Key not found | Key not found. |  |
  *
  * ### SET `0x02`, Forced SET`0x03`
  *
  * Set is used to put a new value into the db at the position of the provided key. If the provided key already exists and SET is used, an error code will be returned. If Forced SET is used in this situation, the value will be replaced by the new one. For the SET-Command **write permissions** are needed. For the forced-SET-Command **write and delete permissions** are needed.
  *
  * ```txt
  * SET Command-Byte: 0x02,0x03 (2,3)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_str(key)][tcp_general(value)]
  *   example 1: 0x02 "test.domain.and.key" ["this", "is", "value"]
  *   example 2: 0x03 "test.domain.and.key" ["this", "is", "value"]
  *
  * Server responds:
  *   [tcp_ui8(ACK)]
  *   example 1: 0xFF
  *   example 2: 0xFF
  * OR
  *   [tcp_ui8(NAK)][error]
  *   example 1: 0x00 0x1234 "Key already exists."
  *   example 2: 0x00 0x1234 "Some other error."
  * ```
  *
  * | Error Code | Name | Message Text | Description |
  * |------------|------|--------------|-------------|
  * | 0x0100 | Invalid Key |  | See GET |
  * | 0xFF00 | Permission denied |  | See GET |
  * | 0x0200 | Key already exists | Key already exists. |  |
  * | 0x0201 | Invalid Value | Invalid value. Could not extract a valid value. |  |
  *
  * ### DELETE `0x04`
  *
  * Delete is used to delete the value of a key. For the DELETE-Command, **delete permissions** are needed.
  *
  * ```txt
  * DELETE Command-Byte: 0x04 (4)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_str(key)]
  *   example 1: 0x02 "test.domain.and.key"
  *
  * Server responds:
  *   [tcp_ui8(ACK)]
  *   example 1: 0xFF
  * OR
  *   [tcp_ui8(NAK)][error]
  *   example 1: 0x00 0x1234 "Some error."
  * ```
  *
  * | Error Code | Name | Message Text | Description |
  * |------------|------|--------------|-------------|
  * | 0x0100 | Invalid Key |  | See GET |
  * | 0xFF00 | Permission denied |  | See GET |
  *
  * ### LIST `0x05`
  *
  * Returns a list of first level sub keys in a map. A key must be provided. The returned list might be empty (length of 0) and the values of the keys might be empty (**T-Flag** check required; values start as general types but may have no following data; see `tcp_list` type).
  *
  * ```txt
  * LIST Command-Byte: 0x05 (5)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_str(key)]
  *   example 1: 0x05 .a.specific.key
  *
  * Server responds:
  *   [tcp_ui8(ACK)][tcp_list(key_map)]
  *   example 1: 0xFF {tcp_list}
  * OR
  *   [tcp_ui8(NAK)][error]
  *   example 1: 0x00 0x1234 "Some error."
  * ```
  *
  * ### SAFE FOLLOW `0x06`
  *
  * Follows a reference for up to the given amount of jumps. For each of the jumps the permissions are checkt and the ACL is enforced. This significantly slows the process down. For higher access speed use FORCED FOLLOW command.
  *
  * Users need **read permissions** for the complete path.
  *
  * ```txt
  * SAFE FOLLOW Command-Byte: 0x06 (6)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_str(key)][tcp_eui(max_jumps)]
  *   example 1: 0x06 .a.specific.key 100
  *
  * Server responds:
  *   [tcp_ui8(ACK)][tcp_general(result)]
  *   example 1: 0xFF "some string value"
  * OR
  *   [tcp_ui8(NAK)][error]
  *   example 1: 0x00 0x1234 "Some error."
  * ```
  *
  * | Error Code | Name | Message Text | Description |
  * |------------|------|--------------|-------------|
  * | 0x0201 | Invalid value |  | if the given max jump value is 0 |
  *
  * ### FORCED FOLLOW `0x07`
  *
  * Follows a reference for up to the given amount of jumps. The provided key itself is checked for permissions, no other permission checks are done. This performs better than SAFE FOLLOW command, but is not ACL conform and therefore needs administrator permissions.
  *
  * Users need **administrator permissions** and **read permissions** for the provided key.
  *
  * ```txt
  * FORCED FOLLOW Command-Byte: 0x07 (7)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_str(key)][tcp_eui(max_jumps)]
  *   example 1: 0x07 .a.specific.key 100
  *
  * Server responds:
  *   [tcp_ui8(ACK)][tcp_general(result)]
  *   example 1: 0xFF "some string value"
  * OR
  *   [tcp_ui8(NAK)][error]
  *   example 1: 0x00 0x1234 "Some error."
  * ```
  *
  * ### DMA Create User `0x80`
  *
  * Creates a new user. The new username and password must be provided, as well as the information if the user is an administrator. Options:
  *
  * - 0x01: If true, new user will be administrator.
  *
  * A user creating another user **must be an administrator**.
  *
  * ```txt
  * DMA Create User Command-Byte: 0x80 (128)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_str(username)][tcp_str(pwd)][tcp_ui8(options)]
  *   example 1: 0x80 new_user pwd 0x00
  *
  * Server responds:
  *   [tcp_ui8(ACK)]
  *   example 1: 0xFF
  * OR
  *   [tcp_ui8(NAK)][error]
  *   example 1: 0x00 0x1234 "Some error."
  * ```
  *
  * | Error Code | Name | Message Text | Description |
  * |------------|------|--------------|-------------|
  * | 0xFF00 | Permission denied |  | See GET |
  * | 0xFF05 | Username already exists. |  |  |
  *
  * ### DMA Update Scope `0x81`
  *
  * This function is used to edit the scope permissions of users. It requires the scope (a key), the user and permissions for this scope. The function can be used to add new scopes, edit existing ones or delete them.
  *
  * Options:
  *
  * - 0x01: read-permission
  * - 0x02: write-permission
  * - 0x04: delete-permission
  *
  * A user updating a users scope **must be an administrator**.
  *
  * ```txt
  * DMA Update Scope Command-Byte: 0x81 (129)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_str(username)][tcp_str(key)][tcp_ui(options)]
  *   example 1: 0x81 user .example.domain 0000.0011
  *
  * Server responds:
  *   [tcp_ui8(ACK)]
  *   example 1: 0xFF
  * OR
  *   [tcp_ui8(NAK)][error]
  *   example 1: 0x00 0x1234 "Some error."
  * ```
  *
  * | Error Code | Name | Message Text | Description |
  * |------------|------|--------------|-------------|
  * | 0x0100 | Invalid Key |  | See GET |
  * | 0xFF00 | Permission denied |  | See GET |
  * | 0xFF02 | User not found |  | See User Package |
  *
  * ### DMA Delete User `0x82`
  *
  * Deletes a user. The username must be provided.
  *
  * A user deleting another user **must be an administrator**.
  *
  * ```txt
  * DMA Create User Command-Byte: 0x82 (130)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_str(username)]
  *   example 1: 0x82 username
  *
  * Server responds:
  *   [tcp_ui8(ACK)]
  *   example 1: 0xFF
  * OR
  *   [tcp_ui8(NAK)][error]
  *   example 1: 0x00 0x1234 "Some error."
  * ```
  *
  * | Error Code | Name | Message Text | Description |
  * |------------|------|--------------|-------------|
  * | 0xFF00 | Permission denied |  | See GET |
  * | 0xFF02 | User not found |  | See User Package |
  * | 0x8201 | User is connected | The user is currently connected and can not be deleted. |  |
  *
  * ### DMA Change Password `0x83`
  *
  * Changes the password of a user.
  *
  * Every user can change their own password.
  *
  * ```txt
  * DMA Change Password Command-Byte: 0x83 (131)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_str(new_pwd)]
  *   example 1: 0x83 passwd123
  *
  * Server responds:
  *   [tcp_ui8(ACK)]
  *   example 1: 0xFF
  * OR
  *   [tcp_ui8(NAK)][error]
  *   example 1: 0x00 0x1234 "Some error."
  * ```
  *
  * | Error Code | Name | Message Text | Description |
  * |------------|------|--------------|-------------|
  * | 0xFF00 | Permission denied |  | See GET |
  *
  * ### DMA Get User-Perms `0x84`
  *
  * Requests the permissions for the specified users.
  *
  * User requesting this information for other users must be administrators.
  *
  * ```txt
  * DMA Change Password Command-Byte: 0x84 (132)
  *
  * Client sends:
  *   [tcp_ui8(command)][tcp_array_str(users)]
  *   example 1: 0x84 0x02 "admin" "user1"
  *   example 2: 0x84 0x00 // requesting all information
  *
  * Server responds:
  *   [tcp_ui8(ACK)]
  *   example 1: 0xFF {admin: (true, {$: 0x07}), user1: (false, {$: 0x03})}
  *   example 2: 0xFF {admin: (true, {$: 0x07}), user1: (false, {$: 0x03}), <other users>}
  * OR
  *   [tcp_ui8(NAK)][error]
  *   example 1&2: 0x00 0x1234 "Some error."
  * ```
  *
  * | Error Code | Name | Message Text | Description |
  * |------------|------|--------------|-------------|
  * | 0xFF00 | Permission denied |  | See GET |
  */