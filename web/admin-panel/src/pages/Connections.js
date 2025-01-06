import React, { useState, useEffect } from "react";
import "../styles/pages/Connections.css";
import { useNavigate } from "react-router-dom";
import ConnectionConfigDTO from "../dto/ConnectionConfigDTO";
import {connect, disconnect, get, set, isLoggedIn, list} from "../api/Api";

const keyDataStructure = {
    a: {
        b: {
            c: 3,
            d: "Hello",
            c1: 7,
            d1: "Hello Again",
        },
        e: "World",
        e1: "Universe",
        e2: {
            nested1: true,
            nested2: [1, 2, 3],
        },
    },
    f: 42,
    f1: 99,
    g: {
        h: {
            i: "Help",
            i1: "List",
        },
        h1: {
            i2: {
                deeplyNested: "Deep",
            },
        },
    },
    j: "Wednesday",
    k: [1, 2, { x: 5, y: "test" }],
};

let toggle = true;

const buildHierarchy = (obj, prefix = "") => {
    const result = [];
    for (const key in obj) {
        if (obj.hasOwnProperty(key)) {
            const currentKey = prefix ? `${prefix}.${key}` : key;
            if (typeof obj[key] === "object" && obj[key] !== null) {
                result.push({
                    key: currentKey,
                    value: null,
                    children: buildHierarchy(obj[key], currentKey),
                    collapsed: true,
                });
            } else {
                result.push({ key: currentKey, value: obj[key], children: [], collapsed: false });
            }
        }
    }
    return result;
};

const renderHierarchy = (nodes, toggleCollapse, navigate, level = 0.5) => {
    return nodes.map((node, index) => (
        <React.Fragment key={node.key + index}>
            <tr>
                <td style={{ paddingLeft: `${level * 20}px` }}>{node.key}</td>
                <td>{node.value !== null ? node.value : ""}</td>
                <td>
                    <button
                        className="edit-button"
                        onClick={() => navigate(`/edit/${node.key}`, {state: {keyData: node.key}})}
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
                renderHierarchy(node.children, toggleCollapse, navigate, level + 1)}
        </React.Fragment>
    ));
};

function App() {
    const [connections, setConnections] = useState([]);
    const [hierarchicalData, setHierarchicalData] = useState([]);
    const [selectedConnection, setSelectedConnection] = useState(0);
    const [otherDataStructure, setOtherDataStructure] = useState({});

    const navigate = useNavigate();

    useEffect(() => {
        const dataToUse =
            selectedConnection?.id === 0
                ? keyDataStructure
                : otherDataStructure;

        const hierarchy = buildHierarchy(dataToUse);
        setHierarchicalData(hierarchy);
    }, [selectedConnection]);


    useEffect(() => {
        let savedConnections = JSON.parse(localStorage.getItem("connections")) || [];

        if (!savedConnections.some((connection) => connection.id === 0)) {
            const testConnection = {
                id: 0,
                ip: "testconn.ase",
                port: "25565",
                username: "test",
            };
            savedConnections = [testConnection];
            localStorage.setItem("connections", JSON.stringify(savedConnections));
        }

        setConnections(savedConnections);
        const hierarchy = buildHierarchy(keyDataStructure);
        setHierarchicalData(hierarchy);

        const testConnection = savedConnections.find((connection) => connection.id === 0);
        setSelectedConnection(testConnection);

    }, []);

    const deleteConnection = async (id) => {
        await disconnect().then(r => console.log("logged out"));

        const updatedConnections = connections.filter((connection) => connection.id !== id);

        setConnections(updatedConnections);
        localStorage.setItem("connections", JSON.stringify(updatedConnections));

        let savedConnections = JSON.parse(localStorage.getItem("connections")) || [];
        const testConnection = savedConnections.find((connection) => connection.id === 0);
        setSelectedConnection(testConnection);
    };

    useEffect(() => {
        if (selectedConnection?.id === 0) {
            const hierarchy = buildHierarchy(keyDataStructure);
            setHierarchicalData(hierarchy);
        } else if (selectedConnection) {
            const hierarchy = buildHierarchy(otherDataStructure);
            setHierarchicalData(hierarchy);
        } else {
            setHierarchicalData([]);
        }
    }, [selectedConnection, otherDataStructure]);

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

    const addConnection = () => {
        const config:ConnectionConfigDTO = new ConnectionConfigDTO();
        config.host = prompt("Enter Host:");
        config.port = parseInt(prompt("Enter Port:"));
        config.username = prompt("Enter Username:");
        config.password = prompt("Enter Password:");
        if (config.host && config.port && config.username && config.password) {
            console.log("config: " + config);
            connect(config).then(async (response) => {
                const newConnection = {
                    id: connections.length + 1,
                    ip: config.host,
                    port: config.port,
                    username: config.username,
                };

                const updatedConnections = [...connections, newConnection];
                setConnections(updatedConnections);

                localStorage.setItem("connections", JSON.stringify(updatedConnections));

                await set("x", 42, false);
            }).catch((error) => {
                console.error("Failed to add connection:", error);
            });
        } else {
            console.error("Invalid connection details.");
        }
    };

    async function buildKeyDataStructure(key = ".") {
        try {
            const response = await list(key);
            if (!response || !response.data || response.data.type !== "KEY_MAP") {
                return {};
            }

            const keyMap = response.data.keyMap;
            const result = {};

            for (const subKey in keyMap) {
                const subKeyData = keyMap[subKey];
                if (subKeyData.type === "KEY_MAP") {
                    const newKey = key === "." ? subKey : `${key}.${subKey}`;
                    result[subKey] = await buildKeyDataStructure(newKey);
                } else if (subKeyData.type === "ARRAY") {
                    result[subKey] = subKeyData.array || [];
                } else if (subKeyData.type === "SIMPLE") {
                    result[subKey] = subKeyData.value;
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
        if (!isLoggedIn()) {
            return;
        }
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
                        onClick={() => {
                                setSelectedConnection(connection);
                                loadData();
                            }
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
                                onClick={() => deleteConnection(connection.id)}
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
                    {renderHierarchy(hierarchicalData, toggleCollapse, navigate)}
                    </tbody>
                </table>
            </div>
        </div>
    );
}

export default App;
