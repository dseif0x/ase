import React, {useEffect, useState} from "react";
import {Subject, zip} from "rxjs";
import {useLocation, useNavigate} from "react-router-dom";
import "../styles/pages/AclEditor.css";
import {get, set, get_acl} from "../api/Api";
import { toast, ToastContainer } from "react-toastify";
import "react-toastify/dist/ReactToastify.css";

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
    const [valueType, setValueType] = useState("STRING");
    const [permissions, setPermissions] = useState(initialPermissions);


    useEffect(() => {
        if (keyData) {
          queue(async () => {
            const val = await get(keyData, connectionId);
            if (val.ok && val.data?.type === "SIMPLE") {
              setValue(val.data?.value ?? "");
              setValueType(val.data?.simpleType || "STRING");
            } else {
              toast.error(val.message);
              console.error()
            }
          });

          queue(async () => {
            console.log("gettting acl...");
            const val = await get_acl("admin", connectionId);
            console.log("got acl");
            console.log(val);
          })
        }
    }, [keyData]);

    const togglePermission = (index, permissionType) => {
        const updatedPermissions = [...permissions];
        updatedPermissions[index][permissionType] = !updatedPermissions[index][permissionType];
        setPermissions(updatedPermissions);
    };

    const handleUpdate = () => {
        let toUpdate;

        switch (valueType) {
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
                <label>Value:</label>

                {valueType === "BOOLEAN" ? (
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
                        placeholder={`Enter a ${SIMPLE_TYPE_MAP[valueType]}`}
                    />
                )}

                <label>Type:</label>
                <select
                    value={valueType}
                    onChange={(e) => setValueType(e.target.value)}
                >
                    <option value="INT">Integer</option>
                    <option value="STRING">String</option>
                    <option value="BOOLEAN">Boolean</option>
                    <option value="DOUBLE">Double</option>
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
