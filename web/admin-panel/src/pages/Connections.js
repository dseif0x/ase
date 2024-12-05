import React, { useState } from "react";
import "../styles/pages/Connections.css";
import {useNavigate} from "react-router-dom";

function App() {
    const [connections, setConnections] = useState([
        { id: 1, address: "127.0.0.1:658369" },
    ]);
    const [data, setData] = useState([
        { key: "Key1", value: "12" },
        { key: "Key2", value: "Hello" },
        { key: "KeyC", value: "12,3" },
        { key: "KeyD", value: "World" },
    ]);

    const navigate = useNavigate();

    const handleEdit = (index) => {
        navigate(`/edit/Key1`);
    };

    const addConnection = () => {
        const newConnection = prompt("Enter new connection address:");
        if (newConnection) {
            setConnections([...connections, { id: connections.length + 1, address: newConnection }]);
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
