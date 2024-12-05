import React, { useState, useEffect } from "react";
import "../styles/pages/Connections.css";
import { useNavigate } from "react-router-dom";

function App() {
    const connectionsDataStructure = [
        { ip: "127.0.0.1", port: "65836", username: "user1" },
        { ip: "192.168.1.100", port: "12345", username: "user2" },
        { ip: "10.0.0.1", port: "45678", username: "user3" },
    ];

    const keyDataStructure = {
        Key1: "12",
        Key2: "Hello",
        Key3: "42",
        Key4: "World",
    };

    const [connections, setConnections] = useState([]);
    const [data, setData] = useState([]);

    const navigate = useNavigate();

    useEffect(() => {
        const loadedConnections = connectionsDataStructure.map((connection, index) => ({
            id: index + 1,
            ...connection,
        }));
        setConnections(loadedConnections);

        const extractedData = Object.entries(keyDataStructure).map(([key, value]) => ({ key, value }));
        setData(extractedData);
    }, []);

    const handleEdit = (index) => {
        navigate(`/edit/${data[index].key}`);
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
                {connections.map((connection) => (
                    <div key={connection.id} className="connection">
                        {connection.address}
                    </div>
                ))}
                <button className="add-connection-button" onClick={addConnection}>
                    + Add Connection
                </button>
            </div>
            <div className="data-panel">
                <table>
                    <thead>
                    <tr>
                        <th>Key</th>
                        <th>Value</th>
                        <th></th>
                    </tr>
                    </thead>
                    <tbody>
                    {data.map((item, index) => (
                        <tr key={index}>
                            <td>{item.key}</td>
                            <td>{item.value}</td>
                            <td>
                                <button className="edit-button" onClick={() => handleEdit(index)}>
                                    Edit
                                </button>
                            </td>
                        </tr>
                    ))}
                    </tbody>
                </table>
            </div>
        </div>
    );
}

export default App;
