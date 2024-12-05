import React, { useState, useEffect } from "react";
import "../styles/pages/Connections.css";
import { useNavigate } from "react-router-dom";

const keyDataStructure = {
    a: {
        b: {
            c: 3,
            d: "Hello",
        },
        e: "World",
    },
    f: 42,
    g: {
        h: {
            i: "DeepValue",
        },
    },
};

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
                        onClick={() => navigate(`/edit/${node.key}`)}
                    >
                        Edit
                    </button>
                    {node.children.length > 0 && (
                        <button
                            className="list-button"
                            onClick={() => toggleCollapse(node)}
                        >
                            List
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

    const navigate = useNavigate();

    useEffect(() => {
        const hierarchy = buildHierarchy(keyDataStructure);
        setHierarchicalData(hierarchy);

        const connectionsDataStructure = [
            { ip: "127.0.0.1", port: "65836", username: "user1" },
            { ip: "192.168.1.100", port: "12345", username: "user2" },
            { ip: "10.0.0.1", port: "45678", username: "user3" },
        ];
        const loadedConnections = connectionsDataStructure.map((connection, index) => ({
            id: index + 1,
            ...connection,
        }));
        setConnections(loadedConnections);
    }, []);

    const toggleCollapse = (node) => {
        node.collapsed = !node.collapsed;
        setHierarchicalData([...hierarchicalData]);
    };

    const addConnection = () => {
        const newIp = prompt("Enter IP Address:");
        const newPort = prompt("Enter Port:");
        const newUsername = prompt("Enter Username:");
        if (newIp && newPort && newUsername) {
            setConnections([
                ...connections,
                { id: connections.length + 1, ip: newIp, port: newPort, username: newUsername },
            ]);
        }
    };

    return (
        <div className="app-container">
            <div className="connections-panel">
                <h3>Connections</h3>
                {connections.map((connection) => (
                    <div key={connection.id} className="connection">
                        {`${connection.ip}:${connection.port}:${connection.username}`}
                    </div>
                ))}
                <button className="add-connection-button" onClick={addConnection}>
                    + Add Connection
                </button>
            </div>
            <div className="data-panel">
                <h3>Keys</h3>
                <table>
                    <thead>
                    <tr>
                        <th>Key</th>
                        <th>Value</th>
                        <th></th>
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
