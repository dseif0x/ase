import React, { useState, useEffect } from "react";
import "../styles/pages/Connections.css";
import {Link, useNavigate} from "react-router-dom";
import ConnectionConfigDTO from "../dto/ConnectionConfigDTO";
import {connect, deleteKey, disconnect, isLoggedIn, list, set} from "../api/Api";
import type WebReturnDto from "../dto/WebReturnDto";
import {Subject, zip} from "rxjs";
import {get} from "../api/Api";
import {toast, ToastContainer} from "react-toastify";
import { IoPersonAdd } from "react-icons/io5";
import {TbPlugConnected, TbPlugConnectedX} from "react-icons/tb";
import {FiRefreshCw} from "react-icons/fi";
import {IoMdAddCircleOutline} from "react-icons/io";
import {MdDelete, MdEdit} from "react-icons/md";


const asyncSub = new Subject();
const nextAsyncSub = new Subject();
zip(asyncSub, nextAsyncSub).subscribe({
  next: ([fn]) => {
    fn().finally(() => nextAsyncSub.next())
  }
});
nextAsyncSub.next();
function queue(fn) {
  asyncSub.next(fn);
}

async function cleanupConnections() {
    const savedConnections = JSON.parse(localStorage.getItem("connections")) || [];

    const updatedConnections = [];

    for (const conn of savedConnections) {
        try {
            const response = await isLoggedIn(conn.connectionId);
            if (response) {
                updatedConnections.push(conn);
            }
        } catch (error) {
            console.error(
                `error with id ${conn.id}:`,
                error
            );
        }
    }

    localStorage.setItem("connections", JSON.stringify(updatedConnections));
}

let toggle = true;

const buildHierarchy = (obj, prefix = "") => {
    const result = [];
    for (const key in obj) {
        if (obj.hasOwnProperty(key)) {
            const currentKey = prefix ? `${prefix}.${key}` : key;
            const { type, value, children, processedKey, noEdit } = obj[key];
            const { parentKey, path } = obj[key] || {};

            const elem = {
                displayKey: key,
                key: processedKey || currentKey,
                value: value || "",
                type: type,
                children: buildHierarchy(children || {}, currentKey),
                collapsed: true,
                noEdit: noEdit,
                parentKey: parentKey,
                path: path,
            };

            result.push(elem);
        }
    }
    return result;
};

function App() {
    const [connections, setConnections] = useState([]);
    const [selectedConnection, setSelectedConnection] = useState([]);
    const [selectedConnectionId, setSelectedConnectionId] = useState(0);
    const [hierarchicalData, setHierarchicalData] = useState([]);

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
                    setSelectedConnectionId(item.id);
                }
            });

            if (!found) {
                if (savedConnections.length > 0) {
                    setSelectedConnection(savedConnections[0]);
                    setSelectedConnectionId(savedConnections[0].id);
                } else {
                    setSelectedConnectionId(0);
                }
            }
        });
    }, []);

    async function checkLogin() {
        return await isLoggedIn(selectedConnection.connectionId);
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
                    setHierarchicalData([]);
                }
            }
        });
    };

    const toggleCollapse = (node) => {
        node.collapsed = !node.collapsed;
        setHierarchicalData([...hierarchicalData]);
    };

    const toggleAll = () => {
        toggle = !toggle;
        const openAll = (nodes) => {
            nodes.forEach((node) => {
                node.collapsed = !toggle;
                if (node.children.length > 0) {
                    openAll(node.children);
                }
            });
        };
        openAll(hierarchicalData);
        setHierarchicalData([...hierarchicalData]);
    };

    const handleCreateUser = () => {
        navigate("/create-user");
    };

    // just for development purposes
    const addDevConnetion = () => {
        const config:ConnectionConfigDTO = new ConnectionConfigDTO();
        config.host = "localhost"
        config.port = 42069;
        config.username = "admin";
        config.password = "rootpwd";

        if (connectionExists(config.host, config.port, config.username)) {
            toast.error("this connection already exists!");
            return;
        }

        connect(config).then(async (response: WebReturnDto) => {
            if (response.ok) {
                const newConnection = {
                    id: connections.length + 1,
                    connectionId: response.data,
                    ip: config.host,
                    port: config.port,
                    username: config.username,
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
            console.error("Failed to add connection:", error);
        });
    }

    function connectionExists(host, port, username) {
        const savedConnections = JSON.parse(localStorage.getItem("connections")) || [];

        return savedConnections.some(
            (conn) =>
                conn.ip === host &&
                conn.port === port &&
                conn.username === username
        );
    }

    const addConnection = () => {
        const config:ConnectionConfigDTO = new ConnectionConfigDTO();
        config.host = prompt("Enter Host:");
        config.port = parseInt(prompt("Enter Port:"));
        config.username = prompt("Enter Username:");
        config.password = prompt("Enter Password:");

        if (connectionExists(config.host, config.port, config.username)) {
            alert("this connection already exists!");
            return;
        }

        if (config.host && config.port && config.username && config.password) {
            connect(config).then(async (response: WebReturnDto) => {
                if (response.ok) {
                    const newConnection = {
                        id: connections.length + 1,
                        connectionId: response.data,
                        ip: config.host,
                        port: config.port,
                        username: config.username,
                    };

                    const updatedConnections = [...connections, newConnection];
                    setConnections(updatedConnections);

                    localStorage.setItem("connections", JSON.stringify(updatedConnections));
                    localStorage.setItem("selectedConnectionId", newConnection.id);

                    setSelectedConnectionId(newConnection.id);
                    setSelectedConnection(newConnection);
                } else console.error("Failed to add connection:", response);
            }).catch((error) => {
                console.error("Failed to add connection:", error);
            });
        } else {
            console.error("Invalid connection details.");
        }
    };

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
                type: `SIMPLE (${data.simpleType || "Unknown"})`,
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
            type: "ARRAY",
            value: `Array (${data.array.length} items)`,
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


    async function buildKeyDataStructure(key = ".") {
        try {
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
                    result[subKey] = {
                        type: "KEY_MAP",
                        value: null,
                        children: await buildKeyDataStructure(newKey),
                    };
                } else if (subKeyData.type === "ARRAY") {
                    const fetchedData = await get(newKey, selectedConnection.connectionId);
                    if (fetchedData.ok && fetchedData.data) {
                        result[subKey] = processArray(fetchedData.data, newKey);
                    } else {
                        console.error("Error fetching array data:", fetchedData);
                        result[subKey] = {
                            type: "ARRAY",
                            value: `Array (${subKeyData.array?.length || 0} items)`,
                            children: [],
                        };
                    }
                } else if (subKeyData.type === "PAIR") {
                    const fetchedData = await get(newKey, selectedConnection.connectionId);
                    if (fetchedData.ok && fetchedData.data) {
                        result[subKey] = processPair(fetchedData.data, newKey);
                    } else {
                        console.error("Error fetching pair data:", fetchedData);
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
                        type: `SIMPLE (${subKeyData.simpleType || "Unknown"})`,
                        value: valueToShow,
                        children: [],
                    };
                } else {
                    console.warn("Unknown type:", subKeyData.type);
                }
            }

            return result;
        } catch (error) {
            console.error("Error in buildKeyDataStructure:", error);
            return {};
        }
    }


    useEffect(() => {
        (async () => {
            if (selectedConnection && selectedConnection.connectionId) {
                const loggedIn = await checkLogin();
                if (loggedIn) {
                  queue(async() => {
                    const data = await buildKeyDataStructure();
                    setHierarchicalData(buildHierarchy(data));
                  })
                }
            }
        })();
    }, [selectedConnection]);

    async function refresh() {
        if (await checkLogin()) {
          queue(async() => {
            const data = await buildKeyDataStructure();
            setHierarchicalData(buildHierarchy(data) || {});
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
            await deleteKey(node.key, connectionId);
            await refresh();
        }
    }

    const renderHierarchy = (nodes, toggleCollapse, navigate, connectionId, level = 0.5, parentNode = null) => {
        return nodes.map((node, index) => (
            <React.Fragment key={node.key + index}>
                <tr>
                    <td style={{paddingLeft: `${level * 20}px`}}>{printDisplayKey(node, parentNode)}</td>
                    <td>{node.value !== null ? node.value : ""}</td>
                    <td>{node.type}</td>
                    <td>
                        {!node.noEdit && (
                            <button
                                className="edit-button"
                                onClick={() => {
                                    navigate(`/edit/${node.key}`, {
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
                        <button
                            className="edit-button red"
                            onClick={() => {
                                handleDelete(node, connectionId);
                            }}
                        >
                            <MdDelete title="Delete Item"/>
                        </button>
                        {node.children.length > 0 && (
                            <button
                                className="list-button"
                                onClick={() => toggleCollapse(node)}
                            >
                                {node.collapsed ? "List" : "Close"}
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

    return (
        <div className="app-container">
            <div className="connections-panel">
                <h3>Connections</h3>
                {connections.map((connection) => (
                    <div
                        key={connection.id}
                        className={`connection ${selectedConnectionId === connection.id ? "selected" : ""}`}
                        onClick={() => {
                            setSelectedConnection(connection);
                            setSelectedConnectionId(connection.id);
                        }}
                    >
                        <input
                            type="radio"
                            name="connection"
                            value={connection.id}
                            checked={selectedConnectionId === connection.id}
                            onChange={() => setSelectedConnection(connection)}
                        />
                        {`${connection.ip}:${connection.port}:${connection.username}`}
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
                    <button onClick={handleCreateUser} className="add-connection-button">
                        <IoPersonAdd /> Create User
                    </button>
                    <button className="add-connection-button blue-button" onClick={addDevConnetion}>
                        Dev Login
                    </button>
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
                        <th>Key</th>
                        <th>Value</th>
                        <th>Type</th>
                        <th>
                            <span>
                                <button
                                    className="list-button list-all"
                                    onClick={toggleAll}>
                                    Toggle All
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
