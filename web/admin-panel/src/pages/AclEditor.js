import React, {useEffect, useState} from "react";
import {useLocation, useNavigate} from "react-router-dom";
import "../styles/pages/AclEditor.css";
import {get, set} from "../api/Api";
import { toast, ToastContainer } from "react-toastify";
import "react-toastify/dist/ReactToastify.css";

const SIMPLE_TYPE_MAP = {
    BOOLEAN: "Boolean",
    INT: "Integer",
    STRING: "String",
    DOUBLE: "Double",
    SEMAPHORE: "Semaphore",
    REFERENCE: "Reference",
    UNKNOWN: "Unknown",
};

const AclEditor = () => {


    const location = useLocation();
    const navigate = useNavigate();

    const { keyData, connectionId } = location.state || {};
    const initialPermissions = [
        { user: "JH", read: false, update: false, delete: false },
        { user: "Group1", read: false, update: false, delete: false },
        { user: "@ALL", read: false, update: false, delete: false },
    ];

    const [value, setValue] = useState("");
    const [simpleType, setSimpleType] = useState("STRING");
    const [type, setType] = useState("SIMPLE");
    const [permissions, setPermissions] = useState(initialPermissions);


    useEffect(() => {
        if (keyData) {
            get(keyData, connectionId).then(val => {
                if (val.ok) {
                    setValue(val.data?.value ?? "");
                    setSimpleType(val.data?.simpleType || "STRING");
                    setType(val.data?.type || "SIMPLE");
                } else {
                    toast.error(val.message);
                    console.error()
                }

            })
        }
    }, [keyData]);

    const handleTypeChange = (e) => {
        const selectedType = e.target.value;
        setType(selectedType);

        if (selectedType !== "SIMPLE") {
            setValue("");
        }
    };

    const togglePermission = (index, permissionType) => {
        const updatedPermissions = [...permissions];
        updatedPermissions[index][permissionType] = !updatedPermissions[index][permissionType];
        setPermissions(updatedPermissions);
    };

    const handleUpdate = () => {
        let toUpdate;

        if (type === "SIMPLE") {
            switch (simpleType) {
                case "INT":
                    let parsedInt = parseInt(value, 10);

                    if (isNaN(parsedInt)) {
                        toast.error("Invalid Integer value.");
                        return;
                    }
                    toUpdate = {
                        type: "SIMPLE",
                        value: parsedInt,
                        simpleType: "INT",
                    }
                    break;
                case "BOOLEAN":
                    let parsedBoolean = value.toLowerCase() === "true";
                    toUpdate = {
                        type: "SIMPLE",
                        value: parsedBoolean,
                        simpleType: "BOOLEAN",
                    }
                    break;
                case "DOUBLE":
                    let parsedDouble = parseFloat(value);
                    if (isNaN(parsedDouble)) {
                        toast.error("Invalid Double value.");
                        return;
                    }
                    toUpdate = {
                        type: "SIMPLE",
                        value: parsedDouble,
                        simpleType: "DOUBLE",
                    }
                    break;
                case "STRING":
                    toUpdate = {
                        type: "SIMPLE",
                        value: value,
                        simpleType: "STRING",
                    }
                    break;
                default:
                    toast.error("Unsupported value type.");
                    return;
            }
        } else if (type === "ARRAY" || type === "PAIR" || type === "KEY_MAP") {
            toUpdate = {
                type: type,
                value: [],
            };
        }


        set(keyData, toUpdate, true, connectionId).then((response) => {
            if (!response.ok) {
                toast.error(response.message);
            } else toast.success("Value updated successfully.");
        });


    };

    const handleCancel = () => {
        navigate(-1);
    };

    return (
        <div className="acl-editor">
            <h2>Key: {keyData}</h2>
            <div className="value-section">
                <label>Type:</label>
                <select value={type} onChange={handleTypeChange}>
                    <option value="SIMPLE">Simple</option>
                    <option value="ARRAY">Array</option>
                    <option value="PAIR">Pair</option>
                    <option value="KEY_MAP">Key Map</option>
                </select>
                {type === "SIMPLE" && (
                    <>
                        <label>Value:</label>

                        {simpleType === "BOOLEAN" ? (
                            <select
                                value={value.toString()}
                                onChange={(e) => setValue(e.target.value)}
                            >
                                <option value="true">True</option>
                                <option value="false">False</option>
                            </select>
                        ) : (
                            <input
                                type="text"
                                value={value}
                                onChange={(e) => setValue(e.target.value)}
                                placeholder={`Enter a ${SIMPLE_TYPE_MAP[simpleType]}`}
                            />
                        )}

                        <label>SIMPLE Type:</label>
                        <select
                            value={simpleType}
                            onChange={(e) => setSimpleType(e.target.value)}
                        >
                            <option value="INT">Integer</option>
                            <option value="STRING">String</option>
                            <option value="BOOLEAN">Boolean</option>
                            <option value="DOUBLE">Double</option>
                        </select>
                    </>
                )}


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
                        {
                            user: `NewUser/Group${permissions.length + 1}`,
                            read: false,
                            update: false,
                            delete: false,
                        },
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

};

export default AclEditor;
