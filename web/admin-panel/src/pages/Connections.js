import React, { useState, useEffect } from "react";
import "../styles/pages/Connections.css";
import {useNavigate} from "react-router-dom";
import ConnectionConfigDTO from "../dto/ConnectionConfigDTO";
import {connect, deleteKey, disconnect, isLoggedIn, list, set, get_acl} from "../api/Api";
import type WebReturnDto from "../dto/WebReturnDto";
import {async, Subject, zip} from "rxjs";
import {get} from "../api/Api";
import {toast, ToastContainer} from "react-toastify";
import "../styles/modals/ConfigModal.css";
import { IoPersonAdd } from "react-icons/io5";
import {TbPlugConnected, TbPlugConnectedX} from "react-icons/tb";
import {FiRefreshCw} from "react-icons/fi";
import {IoMdAddCircleOutline} from "react-icons/io";
import {MdDelete, MdEdit} from "react-icons/md";
import { GoTriangleRight, GoTriangleDown } from "react-icons/go";
import {queue} from "../utils/Queue";

async function cleanupConnections() {

    queue(async () => {
        const savedConnections = JSON.parse(localStorage.getItem("connections")) || [];
        const updatedConnections = [];

        for (const conn of savedConnections) {
            try {
                const response = await isLoggedIn(conn.connectionId)
                conn.loggedIn = response.ok;
                updatedConnections.push(conn);
            } catch (error) {
                console.error(
                    `error with id ${conn.id}:`,
                    error
                );
            }
        }
        localStorage.setItem("connections", JSON.stringify(updatedConnections));
    });
}

const buildHierarchy = (obj, prefix = "") => {
    const result = [];
    for (const key in obj) {
        if (obj.hasOwnProperty(key)) {
            const currentKey = (prefix && prefix !== ".") ? `${prefix}.${key}` : key;
            const { type, value, children, processedKey, noEdit, displayType } = obj[key];
            const { parentKey, path } = obj[key] || {};

            const elem = {
                displayKey: key,
                key: processedKey || currentKey,
                value: value || "",
                type: type,
                displayType: displayType ?? type,
                children: buildHierarchy(children || {}, currentKey),
                collapsed: true,
                noEdit: noEdit,
                parentKey: parentKey,
                path: path,
            };

            if (elem.displayKey === ".") elem.displayKey = ". (DOT)";

            result.push(elem);
        }
    }
    return result;
};

function App() {
    const [connections, setConnections] = useState([]);
    const [selectedConnection, setSelectedConnection] = useState([]);
    const [selectedConnectionId, setSelectedConnectionId] = useState(0);
    const [hierarchicalDataRoots, setHierarchicalDataRoots] = useState([]);
    const [hierarchicalData, setHierarchicalData] = useState([]);
    const [isAdmin, setIsAdmin] = useState(false);

    const [isModalOpen, setIsModalOpen] = useState(false);
    const [reconnectMode, setReconnectMode] = useState(false);
    const [connectionToReconnect, setConnectionToReconnect] = useState(null);

    const [toggle, setToggle] = useState(false);

    const [host, setHost] = useState("");
    const [port, setPort] = useState("");
    const [username, setUsername] = useState("");
    const [password, setPassword] = useState("");

    const navigate = useNavigate();

    useEffect(() => {
        cleanupConnections().then(() => {

            let savedConnections = JSON.parse(localStorage.getItem("connections")) || [];
            let selectedConnectionId = JSON.parse(localStorage.getItem("selectedConnectionId")) || 1;

            setSelectedConnectionId(selectedConnectionId);
            setConnections(savedConnections);

            let found = false;
            savedConnections.forEach((item) => {
                if (item.id === selectedConnectionId && !found) {
                    found = true;
                    setSelectedConnection(item);
                }
            });

            if (!found) {
                if (savedConnections.length > 0) {
                    setSelectedConnection(savedConnections[0]);
                    setSelectedConnectionId(savedConnections[0].id);
                } else {
                    setSelectedConnection([]);
                    setSelectedConnectionId(0);
                }
            }
        });
    }, []);

    async function checkLogin() {
        return await isLoggedIn(selectedConnection.connectionId);
    }

    const handleSelect = (connection) => {
        setSelectedConnection(connection);
        setSelectedConnectionId(connection.id);
        localStorage.setItem("selectedConnectionId", connection.id);
    }

    const deleteConnection = async (id, connectionId) => {
        await disconnect(connectionId).then((r: WebReturnDto) => {
            const updatedConnections = connections.filter((connection) => connection.id !== id);

            setConnections(updatedConnections);
            localStorage.setItem("connections", JSON.stringify(updatedConnections));
            if (selectedConnectionId === id) {
                if (updatedConnections.length > 0) {
                    setSelectedConnectionId(updatedConnections[0].id);
                    setSelectedConnection(updatedConnections[0]);
                } else {
                    setSelectedConnectionId(0);
                    setSelectedConnection([]);
                    setHierarchicalDataRoots([]);
                    setHierarchicalData([]);
                }
            }
        });
    };

    const toggleCollapse = (node, toggleTo = null) => {
        node.collapsed = !node.collapsed;
        if (toggleTo) {
            node.collapsed = toggleTo;
        }
        let toggleKeyList = JSON.parse(localStorage.getItem("toggleKeyList")) ?? [];
        if (node.collapsed) {
            toggleKeyList = toggleKeyList.filter((item) => (!item.startsWith(node.key + '.') && item !== node.key));
        }
        if (!node.collapsed) {
            toggleKeyList.push(node.key);
        }
        localStorage.setItem("toggleKeyList", JSON.stringify(toggleKeyList));
        setHierarchicalData([...hierarchicalData]);
    };

    const toggleFromStorage = (data = null) => {
        const toggleKeyList = localStorage.getItem("toggleKeyList") ?? [];

        const openFromStorage = (nodes) => {
            nodes.forEach((node) => {
                if (toggleKeyList.includes(node.key)) {
                    node.collapsed = false;
                    if (node.children.length > 0) {
                        openFromStorage(node.children);
                    }
                }
            });
        };

        const elem = data ? data : hierarchicalData;
        openFromStorage(elem);
        setHierarchicalData([...elem]);
    }


    const toggleBtn = () => {
        toggleAll();
    }

    const toggleAll = () => {
        setToggle((prev) => !prev);
        const openAll = (nodes) => {
            nodes.forEach((node) => {
                toggleCollapse(node, toggle);
                if (node.children.length > 0) {
                    openAll(node.children);
                }
            });
        };
        openAll(hierarchicalData, toggle);
        setHierarchicalData([...hierarchicalData]);
    };

    const handleCreateUser = () => {
        navigate("/create-user", {
            state: {
                connectionId: selectedConnection.connectionId,
            }
        });
    };

    function connectionExists(host, port, username) {
        const savedConnections = JSON.parse(localStorage.getItem("connections")) || [];

        return savedConnections.some(
            (conn) =>
                conn.ip === host &&
                conn.port === parseInt(port) &&
                conn.username === username
        );
    }

    const addConnection = () => {
        setIsModalOpen(true);
        setHost('');
        setPort('');
        setUsername('');
        setPassword('');
    };

    const handleReconnect = (connection) => {
        setReconnectMode(true);
        setConnectionToReconnect(connection);
        setHost(connection.ip);
        setPort(connection.port);
        setUsername(connection.username);
        setPassword(""); // do not store password
        setIsModalOpen(true);
    }

    function processTcpGeneral(data, key) {
        if (data.type === "KEY_MAP") {
            const processedKeyMap = {};
            for (const subKey in data.keyMap) {
                const currentKey = key === "." ? subKey : `${key}.${subKey}`;
                processedKeyMap[subKey] = processTcpGeneral(data.keyMap[subKey], currentKey);
            }
            return {
                type: "KEY_MAP",
                value: null,
                children: processedKeyMap,
            };
        } else if (data.type === "ARRAY") {
            return processArray(data, key, []);
        } else if (data.type === "PAIR") {
            return processPair(data, key);
        } else if (data.type === "SIMPLE") {
            let valueToShow = data.value?.toString() || "";
            if (data.simpleType === "BOOLEAN") valueToShow = (data.value ? "True" : "False");

            return {
                processedKey: key,
                type: `${data.simpleType || "Unknown"}`,
                value: valueToShow,
                children: [],
            };
        }
        return { type: data.type, value: null, children: [] };
    }


    function processArray(data, key, parentPath = [], rootParentKey = key) {
        const children = data.array.map((item, index) => {
            const childPath = [...parentPath, index];

            const childKey = `${key}[${index}]`;

            let childNode;

            if (item.type === "ARRAY") {
                childNode = processArray(item, childKey, childPath, rootParentKey);
                childNode.parentKey = rootParentKey;
                childNode.path = childPath;
            } else {
                childNode = processTcpGeneral(item, childKey);

                // Attach metadata for non-array elements.
                childNode.parentKey = rootParentKey;
                childNode.path = childPath;
            }

            return childNode;
        });

        return {
            processedKey: key,
            type: `ARRAY`,
            displayType: `ARRAY [${data.array.length}]`,
            value: null,
            children,
            parentKey: null,
            path: null,
        };
    }

    function processPair(data, key) {
        return {
            type: "PAIR",
            value: null,
            processedKey: key,
            children: [
                {
                    processedKey: `${key}.left`,
                    ...processTcpGeneral(data.pair.left, `${key}.left`),
                    noEdit: true
                },
                {
                    processedKey: `${key}.right`,
                    ...processTcpGeneral(data.pair.right, `${key}.right`),
                    noEdit: true
                },
            ],
        };
    }

    function getLastElementAfterDot(key) {
        if (key === ".")
            return key;
        if (key.includes(".")) {
            const parts = key.split(".");
            return parts[parts.length - 1];
        }
        return key;
    }

    async function buildKeyDataStructure(key = ".") {
        try {
            if (key !== ".") {
              const tempData = await get(key, selectedConnection.connectionId);
              if (tempData?.ok && tempData.data?.type) {
                if (tempData.data.type !== "KEY_MAP") {
                  if (tempData.data.type === "ARRAY") {
                    return {[key]: processArray(tempData.data, key)};
                  } else if (tempData.data.type === "PAIR") {
                    return {[key]: processPair(tempData.data, key)};
                  } else if (tempData.data.type === "SIMPLE") {
                    let valueToShow = tempData.data.value?.toString() || "";
                    if (tempData.data.simpleType === "BOOLEAN") valueToShow = (tempData.data.value ? "True" : "False");
                    return {[key]: {
                      type: `${tempData.data.simpleType || "Unknown"}`,
                      value: valueToShow,
                      children: [],
                    }};
                  }
                }
              } else {
                console.error("error in buildKeyDataStructure", key, tempData);
                if (tempData.message !== "Key not found.")
                  toast.error(tempData.data + "- " + tempData.message);
                return {}
              }
            }
            const response = await list(key, selectedConnection.connectionId);
            if (!response.ok || !response.data || response.data.type !== "KEY_MAP") {
                if (!response.ok) {
                    toast.error(response.data + "- " + response.message);
                }
                return {};
            }

            const keyMap = response.data.keyMap;
            const result = {};

            for (const subKey in keyMap) {
                const subKeyData = keyMap[subKey];
                const newKey = key === "." ? subKey : `${key}.${subKey}`;

                if (subKeyData.type === "KEY_MAP") {
                    Object.assign(result, await buildKeyDataStructure(newKey));
                } else if (subKeyData.type === "ARRAY") {
                    const fetchedData = await get(newKey, selectedConnection.connectionId);
                    if (fetchedData.ok && fetchedData.data) {
                        result[subKey] = processArray(fetchedData.data, newKey);
                    } else {
                        result[subKey] = {
                            type: "ARRAY",
                            displayType: `ARRAY [${subKeyData.array?.length || 0}]"`,
                            value: null,
                            children: [],
                        };
                    }
                } else if (subKeyData.type === "PAIR") {
                    const fetchedData = await get(newKey, selectedConnection.connectionId);
                    if (fetchedData.ok && fetchedData.data) {
                        result[subKey] = processPair(fetchedData.data, newKey);
                    } else {
                        result[subKey] = {
                            type: "PAIR",
                            value: null,
                            children: [],
                        };
                    }
                } else if (subKeyData.type === "SIMPLE") {
                    let valueToShow = subKeyData.value?.toString() || "";
                    if (subKeyData.simpleType === "BOOLEAN") valueToShow = (subKeyData.value ? "True" : "False");

                    result[subKey] = {
                        type: `${subKeyData.simpleType || "Unknown"}`,
                        value: valueToShow,
                        children: [],
                    };
                } else {
                }
            }

            return {[getLastElementAfterDot(key)]: {type: "KEY_MAP", children: result}};
        } catch (error) {
            console.error("Error in buildKeyDataStructure:", error);
            return {};
        }
    }

  function getRoots(aclKeyData) {
    if (!aclKeyData) return [];
    const res = [];
    const candidates = [{"key": ".", "value": aclKeyData}]; // acts like a queue
    while (candidates.length > 0) {
      const curr = candidates.splice(0, 1)[0]; // pop the first one
      if ((curr.value.keyMap["$"].value & 0b1) !== 0)
        res.push(curr.key);
      else {
        for (const nextKey in curr.value.keyMap) {
          if (nextKey === "$") continue;
          candidates.push({"key": (curr["key"] !== "." ? curr["key"]+".": "")+nextKey, "value": curr.value.keyMap[nextKey]});
        }
      }
    }
    return res;
  }

    useEffect(() => {
        (async () => {
            if (selectedConnection && selectedConnection.connectionId) {
                queue(async () => {
                    const loggedIn = await checkLogin();
                    if (loggedIn.ok) {

                        const acl = await get_acl(selectedConnection.connectionId, [selectedConnection.username]);
                        if (acl.ok) {
                            const dataRoots = getRoots(acl.data?.keyMap?.[selectedConnection.username]?.pair?.right);
                            setHierarchicalDataRoots(dataRoots);
                            setIsAdmin(acl.data?.keyMap?.[selectedConnection.username]?.pair?.left?.value);
                        } else setHierarchicalDataRoots([]);
                    } else {
                        toast.error(loggedIn.message);
                        setHierarchicalDataRoots([]);
                        selectedConnection.loggedIn = false;
                    }
                });
            }
        })();
    }, [selectedConnection]);

    useEffect(() => {
      queue(async() => {
          let newHierarchicalData = []
          for (const key of hierarchicalDataRoots) {
            const data = await buildKeyDataStructure(key);
            const hierarchy = await buildHierarchy(data);
            newHierarchicalData = newHierarchicalData.concat(hierarchy);
          }
          toggleFromStorage(newHierarchicalData);
      });
    }, [hierarchicalDataRoots]);

    async function refresh() {
        if (selectedConnection && selectedConnection.connectionId) {
            queue(async () => {
                const loggedIn = await checkLogin();
                if (loggedIn.ok) {
                    const acl = await get_acl(selectedConnection.connectionId, [selectedConnection.username]);
                    if (acl.ok) {
                        const dataRoots = getRoots(acl.data?.keyMap?.[selectedConnection.username]?.pair?.right);
                        setHierarchicalDataRoots(dataRoots);
                        setIsAdmin(acl.data?.keyMap?.[selectedConnection.username]?.pair?.left?.value);
                    } else setHierarchicalDataRoots([]);
                } else {
                    toast.error(loggedIn.message);
                    setHierarchicalDataRoots([]);
                    selectedConnection.loggedIn = false;
                }
            })
        }
    }

    async function handleAdd(node, connectionId) {
        navigate(`/edit/key`, {
            state: {
                keyData: node.key,
                connectionId: connectionId,
                addKey: true,
                parentKey: node.parentKey || node.key,
                arrayPath: node.path,
                isArray: node.type === "ARRAY",
            }
        })
    }

    function printDisplayKey(node, parentNode) {
        if (parentNode && parentNode.type === "PAIR") {
            return node.displayKey === '0' ? "left" : "right";
        }
        return node.path ? '[' + node.path + ']' : node.displayKey;
    }

    const removeElementFromArray = (rootArray, deletePath) => {
        const clone = structuredClone(rootArray)
        let current = clone;
        for (let i = 0; i < deletePath.length-1; i++) {
            const index = deletePath[i];
            current = current.array[index];
        }
        current.array.splice(deletePath[deletePath.length-1], 1);
        return clone;
    }

    async function handleDelete(node, connectionId) {
        if (node.path) {
            const res = await get(node.parentKey, connectionId);
            const elem = removeElementFromArray(res.data, node.path);

            queue(async () => set(node.parentKey, elem, true, connectionId).then((response) => {
                if (!response.ok) {
                    toast.error(response.message);
                } else {
                    toast.success("Array element updated successfully.");
                }
                refresh();
            }));
        } else {
            const response = await deleteKey(node.key, connectionId);
            if (!response.ok) {
                toast.error(response.message);
            } else {
                toast.success("Key deleted successfully.");
            }
            await refresh();
        }

        let toggleKeyList = JSON.parse(localStorage.getItem("toggleKeyList"));
        toggleKeyList = toggleKeyList.filter((item) => (!item.startsWith(node.key + '.') && item !== node.key));
        localStorage.setItem("toggleKeyList", JSON.stringify(toggleKeyList));
    }

    const renderHierarchy = (nodes, toggleCollapse, navigate, connectionId, level = 0, parentNode = null) => {
        return nodes.map((node, index) => (
            <React.Fragment key={node.key + index}>
                <tr>
                    <td colSpan="4" onClick={() => {
                      if (node.children.length > 0)
                        toggleCollapse(node)
                    }} style={{paddingLeft: `${level * 20}px`}}><div>{node.children.length > 0 ? (node.collapsed ? (<GoTriangleRight />) : (<GoTriangleDown />)) : ""}{printDisplayKey(node, parentNode)}</div></td>
                    <td colSpan="2">{node.value !== null ? node.value : ""}</td>
                    <td colSpan="2">{node.displayType}</td>
                    <td colSpan="3">
                        {!node.noEdit && (
                            <button
                                className="edit-button"
                                onClick={() => {
                                    navigate(`/edit/${node.key === "." ? "-" : node.key}`, {
                                        state: {
                                            parentKey: node.parentKey,
                                            arrayPath: node.path,
                                            keyData: node.key,
                                            connectionId: connectionId,
                                            addKey: false
                                        }
                                    });
                                }}
                            >
                                <MdEdit title="Edit Item"/>
                            </button>

                        )}
                        {!node.noEdit && node.key !== '.' && (
                        <button
                            className="edit-button red"
                            onClick={() => {
                                handleDelete(node, connectionId);
                            }}
                        >
                            <MdDelete title="Delete Item"/>
                        </button>
                        )}
                        {(node.type === "KEY_MAP" || node.type === "ARRAY") && (
                            <button onClick={async () => handleAdd(node, connectionId)} className="add-key-button">
                                <IoMdAddCircleOutline title="Add Item"/>
                            </button>
                        )}
                    </td>
                </tr>
                {!node.collapsed &&
                    renderHierarchy(node.children, toggleCollapse, navigate, connectionId, level + 1, node)}

            </React.Fragment>
        ));
    };

    const onClose = () => {
        setIsModalOpen(false);
        setReconnectMode(false);
        setConnectionToReconnect(null);
        setIsModalOpen(false);
        setHost('');
        setPort('');
        setUsername('');
        setPassword('');
    }

    const handleSave = () => {
        if (!host || !port || !username || !password) {
            toast.error("Please fill out all fields.");
            return;
        }

        const config:ConnectionConfigDTO = new ConnectionConfigDTO();
        config.host = host;
        config.port = parseInt(port);
        config.username = username;
        config.password = password;

        if (reconnectMode && connectionToReconnect) {
            connect(config)
                .then((response: WebReturnDto) => {
                    if (response.ok) {
                        connectionToReconnect.connectionId = response.data;
                        connectionToReconnect.ip = host;
                        connectionToReconnect.port = port;
                        connectionToReconnect.username = username;
                        connectionToReconnect.loggedIn = true;

                        const updatedList = connections.map((conn) =>
                            conn.id === connectionToReconnect.id ? connectionToReconnect : conn
                        );
                        localStorage.setItem("connections", JSON.stringify(updatedList));

                        setSelectedConnectionId(connectionToReconnect.id);
                        setSelectedConnection(connectionToReconnect);

                        toast.success("Reconnected successfully!");
                    } else {
                        toast.error(response.data ? `${response.data} - ${response.message}` : response.message);
                    }
                })
                .catch((error) => {
                    toast.error("Failed to reconnect.");
                    console.error("Failed to reconnect:", error);
                })
                .finally(() => {
                    onClose();
                });
        } else {
            if (connectionExists(host, port, username)) {
                toast.error("this connection already exists!");
                return;
            }

            connect(config).then(async (response: WebReturnDto) => {
                if (response.ok) {
                    const newConnection = {
                        id: connections.length + 1,
                        connectionId: response.data,
                        ip: host,
                        port: port,
                        username: username,
                        loggedIn: true
                    };

                    const updatedConnections = [...connections, newConnection];
                    setConnections(updatedConnections);

                    localStorage.setItem("connections", JSON.stringify(updatedConnections));
                    localStorage.setItem("selectedConnectionId", newConnection.id);

                    setSelectedConnectionId(newConnection.id);
                    setSelectedConnection(newConnection);
                } else {
                    if (response.data) toast.error(response.data + " - " + response.message);
                    else toast.error(response.message);
                }
            }).catch((error) => {
                toast.error("Failed to add connection.");
                console.error("Failed to add connection:", error);
            }).finally(() => {
                onClose();
            });
        }
    };

    return (
        <div className="app-container">
            <div className="connections-panel">
                <h3>Connections</h3>
                {connections.map((connection) => (
                    <div
                        key={connection.id}
                        className={`connection ${selectedConnectionId === connection.id ? "selected" : ""}`}
                        onClick={() => {
                            handleSelect(connection);
                        }}
                    >
                        <input
                            type="radio"
                            name="connection"
                            value={connection.id}
                            checked={selectedConnectionId === connection.id}
                            onChange={() => {
                                handleSelect(connection);
                            }}                        />
                        {`${connection.ip}:${connection.port}:${connection.username}`}
                        {!connection.loggedIn && (
                            <button
                                className="delete-button"
                                onClick={(e) => {
                                    e.stopPropagation();
                                    handleReconnect(connection);
                                }}
                            >
                                <TbPlugConnected /> Reconnect
                            </button>
                        )}
                        {connection.id !== 0 && (
                            <button
                                className="delete-button"
                                onClick={() => deleteConnection(connection.id, connection.connectionId)}
                            >
                                <TbPlugConnectedX />
                            </button>
                        )}
                    </div>
                ))}
                <div className="footer-container">
                    <button className="add-connection-button" onClick={addConnection}>
                        <TbPlugConnected /> Add Connection
                    </button>

                    {selectedConnectionId !== 0 && isAdmin && (
                        <button disabled={!selectedConnection} onClick={handleCreateUser} className="add-connection-button">
                            <IoPersonAdd /> Create User
                        </button>
                    )}

                </div>
            </div>
            <div className="data-panel">
                <div className="keys-header">
                    <h3>Keys</h3>
                    <span>
                        <button className="refresh-button" onClick={refresh}>
                            <FiRefreshCw /> Refresh
                        </button>
                    </span>
                </div>
                <table>
                    <thead>
                    <tr>
                        <th colSpan="4">Key</th>
                        <th colSpan="2">Value</th>
                        <th colSpan="2">Type</th>
                        <th colSpan="3">
                            <span>
                                <button
                                    className="list-button list-all"
                                    onClick={toggleBtn}>
                                    {!toggle ? 'Open All' : 'Close All'}
                                </button>
                            </span>
                        </th>
                    </tr>
                    </thead>
                    <tbody>
                    {renderHierarchy(hierarchicalData, toggleCollapse, navigate, selectedConnection.connectionId)}
                    </tbody>
                </table>
            </div>
            <div>
                {isModalOpen &&
                    <div className="modal-backdrop">
                        <div className="modal">
                            {reconnectMode && (<h2>Reconnect Configuration</h2>)}
                            {!reconnectMode && (<h2>Enter Configuration</h2>)}
                            <div className="modal-content">
                                <label>
                                    Host:
                                    <input
                                        type="text"
                                        placeholder="Enter Host"
                                        value={host}
                                        onChange={(e) => setHost(e.target.value)}
                                        onKeyDown={(e) => {
                                            if (e.key === "Enter") {
                                                handleSave();
                                            }
                                        }}
                                    />
                                </label>
                                <label>
                                    Port:
                                    <input
                                        type="number"
                                        placeholder="Enter Port"
                                        value={port}
                                        onChange={(e) => setPort(e.target.value)}
                                        onKeyDown={(e) => {
                                            if (e.key === "Enter") {
                                                handleSave();
                                            }
                                        }}
                                    />
                                </label>
                                <label>
                                    Username:
                                    <input
                                        type="text"
                                        placeholder="Enter Username"
                                        value={username}
                                        onChange={(e) => setUsername(e.target.value)}
                                        onKeyDown={(e) => {
                                            if (e.key === "Enter") {
                                                handleSave();
                                            }
                                        }}
                                    />
                                </label>
                                <label>
                                    Password:
                                    <input
                                        type="password"
                                        placeholder="Enter Password"
                                        value={password}
                                        onChange={(e) => setPassword(e.target.value)}
                                        onKeyDown={(e) => {
                                            if (e.key === "Enter") {
                                                handleSave();
                                            }
                                        }}
                                    />
                                </label>
                            </div>
                            <div className="modal-actions">
                                <button onClick={onClose} className="cancel-button">Cancel</button>
                                {reconnectMode && (<button onClick={handleSave} className="save-button">Reconnect</button>)}
                                {!reconnectMode && (<button onClick={handleSave} className="save-button">Add</button>)}
                            </div>
                        </div>
                    </div>
                }
            </div>
            <ToastContainer
                position="bottom-center"
                autoClose={3000}
                hideProgressBar={false}
                newestOnTop={false}
                closeOnClick
                rtl={false}
                pauseOnFocusLoss
                draggable
                pauseOnHover
                theme="colored"
            />
        </div>
    );
}

export default App;
