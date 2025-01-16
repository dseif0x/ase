import React, { useState, useEffect } from "react";
import "../styles/pages/Connections.css";
import { useNavigate } from "react-router-dom";
import ConnectionConfigDTO from "../dto/ConnectionConfigDTO";
import {connect, disconnect, isLoggedIn, list} from "../api/Api";
import type WebReturnDto from "../dto/WebReturnDto";

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
            const { type, value } = obj[key];

            if (type === "KEY_MAP") {
                result.push({
                    key: currentKey,
                    value: null,
                    type: type,
                    children: buildHierarchy(value, currentKey),
                    collapsed: true,
                });
            } else if (type === "ARRAY") {
                result.push({
                    key: currentKey,
                    value: JSON.stringify(value),
                    type: type,
                    children: [],
                    collapsed: false,
                });
            } else if (type?.startsWith("SIMPLE")) {
                result.push({
                    key: currentKey,
                    value: value,
                    type: type,
                    children: [],
                    collapsed: false,
                });
            }
        }
    }
    return result;
};

const renderHierarchy = (nodes, toggleCollapse, navigate, connectionId, level = 0.5) => {
    return nodes.map((node, index) => (
        <React.Fragment key={node.key + index}>
            <tr>
                <td style={{paddingLeft: `${level * 20}px`}}>{node.key}</td>
                <td>{node.value !== null ? node.value : ""}</td>
                <td>{node.type}</td>
                <td>
                    <button
                        className="edit-button"
                        onClick={() => navigate(`/edit/${node.key}`, {
                            state: {
                                keyData: node.key,
                                connectionId: connectionId
                            }
                        })}
                    >
                        Edit
                    </button>

                    {node.children.length > 0 && (
                        <button
                            className="list-button"
                            onClick={() => toggleCollapse(node)}
                        >
                            {node.collapsed ? "List" : "Close"}
                        </button>
                    )}
                </td>
            </tr>
            {!node.collapsed &&
                renderHierarchy(node.children, toggleCollapse, navigate, connectionId, level + 1)}
        </React.Fragment>
    ));
};

function App() {
    const [connections, setConnections] = useState([]);
    const [selectedConnection, setSelectedConnection] = useState([]);
    const [selectedConnectionId, setSelectedConnectionId] = useState(0);
    const [hierarchicalData, setHierarchicalData] = useState([]);
    const [selectedConnection, setSelectedConnection] = useState(0);
    const [otherDataStructure, setOtherDataStructure] = useState({});

    const navigate = useNavigate();

    useEffect(() => {
        cleanupConnections().then(r => {

            let savedConnections = JSON.parse(localStorage.getItem("connections")) || [];
            let selectedConnectionId = JSON.parse(localStorage.getItem("selectedConnectionId")) || 1;

            setSelectedConnectionId(selectedConnectionId);
            setConnections(savedConnections);

            let found = false;
            savedConnections.forEach((item) => {
                if (item.id === selectedConnectionId) {
                    found = true;
                    setSelectedConnection(item);
                }
            });

            if (!found) {
                if (savedConnections.length > 0) {
                    setSelectedConnection(savedConnections[0]);
                    setSelectedConnectionId(savedConnections[0].id);
                }
                setSelectedConnectionId(0);
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
                node.collapsed = toggle;
                if (node.children.length > 0) {
                    openAll(node.children);
                }
            });
        };
        openAll(hierarchicalData);
        setHierarchicalData([...hierarchicalData]);
    };

    // just for development purposes
    const addDevConnetion = () => {
        const config:ConnectionConfigDTO = new ConnectionConfigDTO();
        config.host = "localhost"
        config.port = 42069;
        config.username = "admin";
        config.password = "rootpwd";

        if (connectionExists(config.host, config.port, config.username)) {
            alert("this connection already exists!");
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
            } else console.error("Failed to add connection:", response);
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
        if (config.host && config.port && config.username && config.password) {
            console.log("config: " + config);
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
                } else console.error("Failed to add connection:", response);
            }).catch((error) => {
                console.error("Failed to add connection:", error);
            });
        } else {
            console.error("Invalid connection details.");
        }
    };

    async function buildKeyDataStructure(key = ".") {
        try {
            const response = await list(key, selectedConnection.connectionId);
            if (!response.ok || !response.data || response.data.type !== "KEY_MAP") {
                return {};
            }

            const keyMap = response.data.keyMap;
            const result = {};

            for (const subKey in keyMap) {
                const subKeyData = keyMap[subKey];
                if (subKeyData.type === "KEY_MAP") {
                    const newKey = key === "." ? subKey : `${key}.${subKey}`;
                    result[subKey] = {
                        type: "KEY_MAP",
                        value: await buildKeyDataStructure(newKey),
                    };
                } else if (subKeyData.type === "ARRAY") {
                    result[subKey] = {
                        type: "ARRAY",
                        value: subKeyData.array || [],
                    };
                } else if (subKeyData.type === "SIMPLE") {
                    result[subKey] = {
                        type: `SIMPLE (${subKeyData.simpleType || "Unknown"})`,
                        value: subKeyData.value,
                    };
                } else {
                    console.log("no such type:", subKey);
                }
            }

            return result;
        } catch (error) {
            console.error("error:", error);
            return {};
        }
    }

    async function loadData() {
        const keyDataStructure = await buildKeyDataStructure();
        setOtherDataStructure(keyDataStructure);
    }

    return (
        <div className="app-container">
            <div className="connections-panel">
                <h3>Connections</h3>
                {connections.map((connection) => (
                    <div
                        key={connection.id}
                        className={`connection ${selectedConnection?.id === connection.id ? "selected" : ""}`}
                        onClick={() => {setSelectedConnection(connection);}
                        }
                    >
                        <input
                            type="radio"
                            name="connection"
                            value={connection.id}
                            checked={selectedConnection?.id === connection.id}
                            onChange={() => setSelectedConnection(connection)}
                        />
                        {`${connection.ip}:${connection.port}:${connection.username}`}
                        {connection.id !== 0 && (
                            <button
                                className="delete-button"
                                onClick={() => deleteConnection(connection.id, connection.connectionId)}
                            >
                                X
                            </button>
                        )}
                    </div>
                ))}
                <button className="add-connection-button" onClick={addConnection}>
                    + Add Connection
                </button>
            </div>
            <div className="data-panel">
                <div className="keys-header">
                    <h3>Keys</h3>
                    <span>
                        <button className="refresh-button" onClick={loadData}>
                            Refresh
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
        </div>
    );
}

export default App;
