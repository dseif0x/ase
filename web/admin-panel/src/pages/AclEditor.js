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

const TcpGeneralType = ["SIMPLE", "ARRAY", "PAIR", "KEY_MAP"];
const SimpleTypes = ["BOOLEAN", "INT", "STRING", "DOUBLE"];

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
    const [pair, setPair] = useState({
        left: { type: "SIMPLE", simpleType: "STRING", value: "" },
        right: { type: "SIMPLE", simpleType: "STRING", value: "" },
    });

    useEffect(() => {
        if (keyData) {
            get(keyData, connectionId).then(val => {
                if (val.ok) {
                    const data = val.data || {};
                    setType(data.type || "SIMPLE");

                    if (data.type === "PAIR") {
                        setPair({
                            left: {
                                type: data.pair?.left?.type || "SIMPLE",
                                simpleType: data.pair?.left?.simpleType || "STRING",
                                value: data.pair?.left?.value || "",
                            },
                            right: {
                                type: data.pair?.right?.type || "SIMPLE",
                                simpleType: data.pair?.right?.simpleType || "STRING",
                                value: data.pair?.right?.value || "",
                            },
                        });
                    } else {
                        setValue(data.value || "");
                    }

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

    const handlePairChange = (side, field, newValue) => {
        setPair((prev) => ({
            ...prev,
            [side]: { ...prev[side], [field]: newValue },
        }));
    };

    const togglePermission = (index, permissionType) => {
        const updatedPermissions = [...permissions];
        updatedPermissions[index][permissionType] = !updatedPermissions[index][permissionType];
        setPermissions(updatedPermissions);
    };

    const parseSimpleType = (simpleType, value) => {
        let toUpdate;
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
                return null;
        }

        return toUpdate;
    }
    const handleUpdate = () => {
        let toUpdate;

        if (type === "SIMPLE") {
            toUpdate = parseSimpleType(simpleType, value);
            if (!toUpdate) return;
        } else if (type === "ARRAY" || type === "KEY_MAP") {
            toUpdate = {
                type: type
            };
        } else if (type === "PAIR") {
            const { left, right } = pair;
            toUpdate = {
                type: "PAIR",
                pair: {},
            }
            if (left.type === "SIMPLE") {
                const leftUpdate = parseSimpleType(left.simpleType, left.value);
                if (!leftUpdate) return;
                toUpdate.pair.left = leftUpdate;
            } else if (left.type === "ARRAY" || left.type === "KEY_MAP") {
                toUpdate.pair.left = { type: left.type };
            }

            if (right.type === "SIMPLE") {
                const rightUpdate = parseSimpleType(right.simpleType, right.value);
                if (!rightUpdate) return;
                toUpdate.pair.right = rightUpdate;
            } else if (right.type === "ARRAY" || right.type === "KEY_MAP") {
                toUpdate.pair.right = { type: right.type };
            }

            if (!toUpdate.pair.left || !toUpdate.pair.right) {
                toast.error("Invalid pair values.");
                return;
            }
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
            <h2 className="editor-title">Key: {keyData}</h2>

            <div className="editor-section">
                <label>Type:</label>
                <select value={type} onChange={handleTypeChange}>
                    <option value="SIMPLE">Simple</option>
                    <option value="ARRAY">Array</option>
                    <option value="PAIR">Pair</option>
                    <option value="KEY_MAP">Key Map</option>
                </select>
            </div>

            {/* SIMPLE Type */}
            {type === "SIMPLE" && (
                <div className="editor-section simple-section">
                    <h3>SIMPLE Type</h3>
                    <div className="simple-row">
                        <label>Simple Type:</label>
                        <select
                            value={simpleType}
                            onChange={(e) => setSimpleType(e.target.value)}
                        >
                            {SimpleTypes.map((simpleType) => (
                                <option key={simpleType} value={simpleType}>
                                    {simpleType}
                                </option>
                            ))}
                        </select>
                    </div>
                    <div className="simple-row">
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
                    </div>
                </div>
            )}

            {/* PAIR Type */}
            {type === "PAIR" && (
                <div className="pair-section">
                    {/* Left Value */}
                    <div className="pair-item">
                        <h3>Left Value</h3>
                        <div className="pair-row">
                            <label>Type:</label>
                            <select
                                value={pair.left.type}
                                onChange={(e) =>
                                    handlePairChange("left", "type", e.target.value)
                                }
                            >
                                {TcpGeneralType.map((tcpType) => (
                                    <option key={tcpType} value={tcpType}>
                                        {tcpType}
                                    </option>
                                ))}
                            </select>
                        </div>

                        {pair.left.type === "SIMPLE" && (
                            <>
                                <div className="pair-row">
                                    <label>Simple Type:</label>
                                    <select
                                        value={pair.left.simpleType}
                                        onChange={(e) =>
                                            handlePairChange("left", "simpleType", e.target.value)
                                        }
                                    >
                                        {SimpleTypes.map((simpleType) => (
                                            <option key={simpleType} value={simpleType}>
                                                {simpleType}
                                            </option>
                                        ))}
                                    </select>
                                </div>
                                <div className="pair-row">
                                    <label>Value:</label>
                                    <input
                                        type="text"
                                        value={pair.left.value}
                                        onChange={(e) =>
                                            handlePairChange("left", "value", e.target.value)
                                        }
                                        placeholder={`Enter ${pair.left.simpleType}`}
                                    />
                                </div>
                            </>
                        )}
                    </div>

                    {/* Right Value */}
                    <div className="pair-item">
                        <h3>Right Value</h3>
                        <div className="pair-row">
                            <label>Type:</label>
                            <select
                                value={pair.right.type}
                                onChange={(e) =>
                                    handlePairChange("right", "type", e.target.value)
                                }
                            >
                                {TcpGeneralType.map((tcpType) => (
                                    <option key={tcpType} value={tcpType}>
                                        {tcpType}
                                    </option>
                                ))}
                            </select>
                        </div>

                        {pair.right.type === "SIMPLE" && (
                            <>
                                <div className="pair-row">
                                    <label>Simple Type:</label>
                                    <select
                                        value={pair.right.simpleType}
                                        onChange={(e) =>
                                            handlePairChange("right", "simpleType", e.target.value)
                                        }
                                    >
                                        {SimpleTypes.map((simpleType) => (
                                            <option key={simpleType} value={simpleType}>
                                                {simpleType}
                                            </option>
                                        ))}
                                    </select>
                                </div>
                                <div className="pair-row">
                                    <label>Value:</label>
                                    <input
                                        type="text"
                                        value={pair.right.value}
                                        onChange={(e) =>
                                            handlePairChange("right", "value", e.target.value)
                                        }
                                        placeholder={`Enter ${pair.right.simpleType}`}
                                    />
                                </div>
                            </>
                        )}
                    </div>
                </div>
            )}

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
