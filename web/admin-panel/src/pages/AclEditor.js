import React, { useState, useEffect } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import "../styles/pages/AclEditor.css";
import {get, set} from "../api/Api";

const AclEditor = () => {
    const location = useLocation();
    const navigate = useNavigate();

    const { keyData } = location.state || {};
    const initialPermissions = [
        { user: "JH", read: false, update: false, delete: false },
        { user: "Group1", read: false, update: false, delete: false },
        { user: "@ALL", read: false, update: false, delete: false },
    ];

    const [value, setValue] = useState("");
    const [valueType, setValueType] = useState("String");
    const [permissions, setPermissions] = useState(initialPermissions);


    useEffect(() => {
        if (keyData) {
            console.log("current key: " + keyData);
            get(keyData).then(val => {
                setValue(val.data?.value ?? "");
            })
        }
    }, [keyData]);

    const togglePermission = (index, permissionType) => {
        const updatedPermissions = [...permissions];
        updatedPermissions[index][permissionType] = !updatedPermissions[index][permissionType];
        setPermissions(updatedPermissions);
    };

    const handleUpdate = () => {

        set(keyData, parseInt(value), true).then(response => {
            navigate(-1);
        });

    };

    const handleCancel = () => {
        navigate(-1);
    };

    return (
        <div className="acl-editor">
            <h2>Key: {keyData}</h2>
            <div className="value-section">
                <label>Value:</label>
                <input
                    type="text"
                    value={value}
                    onChange={(e) => setValue(e.target.value)}
                />
                <select
                    value={valueType}
                    onChange={(e) => setValueType(e.target.value)}
                >
                    <option>Integer</option>
                    <option>String</option>
                    <option>Boolean</option>
                </select>
            </div>

            <h3>Current ACL Permissions</h3>
            <table className="permissions-table">
                <thead>
                <tr>
                    <th>User/Group</th>
                    <th>Read</th>
                    <th>Update</th>
                    <th>Delete</th>
                    <th>Remove</th>
                </tr>
                </thead>
                <tbody>
                {permissions.map((permission, index) => (
                    <tr key={index}>
                        <td>{permission.user}</td>
                        {["read", "update", "delete"].map((perm) => (
                            <td key={perm}>
                                <input
                                    type="checkbox"
                                    checked={permission[perm]}
                                    onChange={() => togglePermission(index, perm)}
                                />
                            </td>
                        ))}
                        <td>
                            <button
                                className="delete-btn"
                                onClick={() => {
                                    const updatedPermissions = permissions.filter(
                                        (_, i) => i !== index
                                    );
                                    setPermissions(updatedPermissions);
                                }}
                            >
                                <span className="trash">&#128465;</span>

                            </button>
                        </td>
                    </tr>
                ))}
                </tbody>
            </table>

            <button
                className="add-btn"
                onClick={() =>
                    setPermissions([
                        ...permissions,
                        { user: `NewUser/Group${permissions.length + 1}`, read: false, update: false, delete: false },
                    ])
                }
            >
                Add
            </button>
            <div className="action-buttons">
                <button className="cancel-btn" onClick={handleCancel}>
                    Cancel
                </button>
                <button className="update-btn" onClick={handleUpdate}>
                    Update
                </button>
            </div>
        </div>
    );
};

export default AclEditor;
