import React, { useState } from "react";
import "../styles/pages/AclEditor.css";

const AclEditor = () => {
    const [value, setValue] = useState("12");
    const [valueType, setValueType] = useState("Integer");
    const [permissions, setPermissions] = useState([
        { user: "JH", read: false, update: false, delete: false },
        { user: "Group1", read: false, update: false, delete: false },
        { user: "@ALL", read: false, update: false, delete: false },
    ]);

    const togglePermission = (index, permissionType) => {
        const updatedPermissions = [...permissions];
        updatedPermissions[index][permissionType] = !updatedPermissions[index][permissionType];
        setPermissions(updatedPermissions);
    };

    return (
        <div className="acl-editor">
            <h2>Key: Key1</h2>
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
                            <button className="delete-btn">🗑</button>
                        </td>
                    </tr>
                ))}
                </tbody>
            </table>

            <button className="add-btn">Add</button>
            <div className="action-buttons">
                <button className="cancel-btn">Cancel</button>
                <button className="update-btn">Update</button>
            </div>
        </div>
    );
};

export default AclEditor;
