import React, {useEffect, useState, useRef} from "react";
import {Subject, zip} from "rxjs";
import {useLocation, useNavigate} from "react-router-dom";
import "../styles/pages/AclEditor.css";
import {get, set, get_acl, update_acl} from "../api/Api";
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

function evaluatePermissionDiff(initial, current) {
  const res = [];
  for (const perm of current) {
    const otherPerm = initial.find(x => x.user === perm.user);
    if (otherPerm) {
      if (otherPerm.update !== perm.update || otherPerm.delete !== perm.delete || otherPerm.read !== perm.read)
        res.push(Object.assign({}, perm));
    } else {
      res.push(Object.assign({}, perm));
    }
  }
  return res;
}

function permToNumber(perm) {
  let res = 0;
  if (perm.read)
    res |= 0b1;
  if (perm.update)
    res |= 0b10;
  if (perm.delete)
    res |= 0b100;
  return res;
}

function evaluatePermissions(permission, key) {
  const splitKey = key.split(".");
  let perm = 0;
  let itObj = permission;
  for (let i = 0; i < splitKey.length; i++) {
    if (!itObj[splitKey[i]]) return perm;
    itObj = itObj[splitKey[i]].keyMap;
  }
  if (itObj["$"]) {
    itObj = itObj["$"];
    if (itObj["simpleType"] === "INT" && typeof(itObj["value"]) === "number")
      perm |= itObj["value"];
  }
  return perm;
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

const TcpGeneralType = ["SIMPLE"];
const SimpleTypes = ["BOOLEAN", "INT", "STRING", "DOUBLE"];

function setElementByPath(rootArray, arrayPath, newElement) {
    const clone = structuredClone(rootArray);
    let current = clone;

    for (let i = 0; i < arrayPath.length - 1; i++) {
        const idx = arrayPath[i];
        current = current.array[idx];
    }
    const finalIndex = arrayPath[arrayPath.length - 1];
    current.array[finalIndex] = newElement;
    return clone;
}

function getElementByPath(tcpArray, arrayPath = []) {
    let current = tcpArray;
    for (let idx of arrayPath) {
        if (!current || current.type !== "ARRAY" || !Array.isArray(current.array)) {
            return null;
        }
        if (idx < 0 || idx >= current.array.length) {
            return null;
        }
        current = current.array[idx];
    }
    return current;
}

const AclEditor = () => {
    const location = useLocation();
    const navigate = useNavigate();

    let { keyData, connectionId, addKey, parentKey, arrayPath, isArray, addArrayKeyPath } = location.state || {};
    if (keyData === "-") keyData = ".";
    const initialPermissions = useRef([]);
    const initialData = useRef({});

    const [newKey, setNewKey] = useState("");
    const [value, setValue] = useState("");
    const [simpleType, setSimpleType] = useState("STRING");
    const [type, setType] = useState("SIMPLE");
    const [permissions, setPermissions] = useState([]);
    const [pair, setPair] = useState({
        left: { type: "SIMPLE", simpleType: "STRING", value: "" },
        right: { type: "SIMPLE", simpleType: "STRING", value: "" },
    });
    const [isEditingArrayElement, setIsEditingArrayElement] = useState(false);
    const [parentArrayData, setParentArrayData] = useState(null);

    useEffect(() => {
        if (arrayPath && parentKey && !addKey) {
            setIsEditingArrayElement(true);
            loadArrayElement(parentKey, arrayPath);
        } else if (keyData && !addKey) {
          queue(async() => {
            const val = await get(keyData, connectionId);
            if (val.ok) {
              const data = val.data || {};
              initialData.current = Object({}, data);
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
              } else if (data.type === "SIMPLE" && data.simpleType === "BOOLEAN") {
                setValue(data.value ? "True" : "False");
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
          queue(async () => {
            console.log("getting acl");
            const val = await get_acl(connectionId);
            if (val.ok) {
              const data = val.data.keyMap;
              const perms = []
              for(const key in data) {
                const perm = evaluatePermissions(data[key].pair.right.keyMap, keyData);
                perms.push({user: key, read: (perm & 0b1) !== 0, update: (perm & 0b10) !== 0, delete: (perm & 0b100) !== 0});
              }
              const permsCopy = [];
              perms.forEach(x => permsCopy.push(Object.assign({}, x)));
              initialPermissions.current = permsCopy;
              setPermissions(perms);
            }
          })
        }
    }, [keyData, connectionId, parentKey, arrayPath, addKey]);

    async function loadArrayElement(key, path) {
        const res = await get(key, connectionId);
        if (!res.ok || !res.data || res.data.type !== "ARRAY") {
            toast.error("Parent key is not an ARRAY or not found");
            return;
        }
        const rootArray = res.data;
        setParentArrayData(rootArray);

        const element = getElementByPath(rootArray, path);
        if (!element) {
            toast.error("Array element not found at path " + JSON.stringify(path));
            return;
        }

        setType(element.type);
        if (element.type === "SIMPLE") {
            setSimpleType(element.simpleType);
            if (element.simpleType === "BOOLEAN") {
                setValue(element.value ? "True" : "False");
            } else {
                setValue(element.value?.toString() ?? "");
            }
        } else if (element.type === "PAIR") {
            setPair({
                left: {
                    type: element.pair?.left?.type || "SIMPLE",
                    simpleType: element.pair?.left?.simpleType || "STRING",
                    value: element.pair?.left?.value || "",
                },
                right: {
                    type: element.pair?.right?.type || "SIMPLE",
                    simpleType: element.pair?.right?.simpleType || "STRING",
                    value: element.pair?.right?.value || "",
                },
            });
        } else {
            console.log("Editing array element of type:", element.type);
        }
    }

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

    function parseSimpleType(simpleType, inputValue) {
        switch (simpleType) {
            case "INT": {
                const parsedInt = parseInt(inputValue, 10);
                if (isNaN(parsedInt)) {
                    toast.error("Invalid integer value");
                    return null;
                }
                return parsedInt;
            }
            case "DOUBLE": {
                const parsedDouble = parseFloat(inputValue);
                if (isNaN(parsedDouble)) {
                    toast.error("Invalid double value");
                    return null;
                }
                return parsedDouble;
            }
            case "BOOLEAN":
                return inputValue === "True";
            case "STRING":
            default:
                return inputValue;
        }
    }

    function buildSimpleTcpGeneral(simpleType, rawValue) {
        const parsed = parseSimpleType(simpleType, rawValue);
        if (parsed === null) return null;
        return {
            type: "SIMPLE",
            value: parsed,
            simpleType: simpleType,
        };
    }

    const addElementToArray = (rootArray, newElement, path) => {
        const clone = structuredClone(rootArray)
        let current = clone;
        for (let i = 0; i < path.length; i++) {
            const index = path[i];
            current = current.array[index];
        }
        if (!current.array) {
            current.array = [];
        }
        current.array[current.array.length] = newElement;
        return clone;
    }

    const handleAclUpdate = async () => {
      const permDiff = evaluatePermissionDiff(initialPermissions.current, permissions);
      for (const permUpdate of permDiff) {
        queue(async() => update_acl(connectionId, permUpdate.user, keyData, permToNumber(permUpdate)));
      }
    }

    const handleUpdate = async () => {
        if (isEditingArrayElement) {
            if (!parentArrayData) return;
            const element = getElementByPath(parentArrayData, arrayPath);
            if (!element) {
                toast.error("Failed to locate element to update");
                return;
            }

            let updatedElement = structuredClone(element);
            if (type === "SIMPLE") {
                const newVal = buildSimpleTcpGeneral(simpleType, value);
                if (!newVal) return;
                updatedElement = newVal;
            } else if (type === "PAIR") {
                let leftObj, rightObj;
                if (pair.left.type === "SIMPLE") {
                    leftObj = buildSimpleTcpGeneral(pair.left.simpleType, pair.left.value);
                    if (!leftObj) return;
                } else {
                    leftObj = {type: pair.left.type};
                }

                if (pair.right.type === "SIMPLE") {
                    rightObj = buildSimpleTcpGeneral(pair.right.simpleType, pair.right.value);
                    if (!rightObj) return;
                } else {
                    rightObj = {type: pair.right.type};
                }

                updatedElement = {
                    type: "PAIR",
                    pair: {left: leftObj, right: rightObj},
                };
            } else {
                if (type === "SIMPLE") {
                    const newVal = buildSimpleTcpGeneral(simpleType, value);
                    if (!newVal) return;
                    updatedElement = newVal;
                } else if (type === "ARRAY" || type === "KEY_MAP") {
                    updatedElement = {type};
                }
            }

            const newRootArray = setElementByPath(parentArrayData, arrayPath, updatedElement);


            queue(async () => set(parentKey, newRootArray, true, connectionId).then((response) => {
                if (!response.ok) {
                    toast.error(response.message);
                    setTimeout(() => {
                        navigate("/Connections");
                    }, 1000);
                } else {
                    toast.success("Array element updated successfully.");
                }
            }));
            return;
        }

        let toUpdate;

        if (type === "SIMPLE") {
            const newVal = buildSimpleTcpGeneral(simpleType, value);
            if (!newVal) return;
            toUpdate = newVal;
        } else if (type === "ARRAY" || type === "KEY_MAP") {
            toUpdate = {type};
        } else if (type === "PAIR") {
            let leftObj, rightObj;
            if (pair.left.type === "SIMPLE") {
                leftObj = buildSimpleTcpGeneral(pair.left.simpleType, pair.left.value);
                if (!leftObj) return;
            } else {
                leftObj = {type: pair.left.type};
            }

            if (pair.right.type === "SIMPLE") {
                rightObj = buildSimpleTcpGeneral(pair.right.simpleType, pair.right.value);
                if (!rightObj) return;
            } else {
                rightObj = {type: pair.right.type};
            }

            toUpdate = {
                type: "PAIR",
                pair: {left: leftObj, right: rightObj},
            };
        }

        if (addKey) {
            if (!isArray) {
                if (!newKey.length > 0 || newKey.indexOf(".") > -1) {
                    toast.error("New key is invalid.");
                    return;
                }
                keyData = keyData + (keyData === '.' ? '' : '.') + newKey;
            } else {
                const res = await get(parentKey, connectionId);
                if (!arrayPath) {
                    arrayPath = [];
                }
                const newElem = addElementToArray(res.data, toUpdate, arrayPath);
                queue(async() => set(parentKey, newElem, true, connectionId).then((response) => {
                    if (!response.ok) {
                        toast.error(response.message);
                        setTimeout(() => {
                            navigate("/Connections");
                        }, 1000);
                    } else {
                        toast.success("Array element updated successfully.");
                    }
                    navigate(-1);
                }));
                return;
            }
        }

      queue(async() => set(keyData, toUpdate, true, connectionId).then((response) => {
        if (!response.ok) {
            setTimeout(() => {
                navigate("/Connections");
            }, 1000);
        } else toast.success("Value updated successfully.");
        if (addKey) {
            navigate(-1);
        }
      }));
    };

    const handleCancel = () => {
        navigate(-1);
    };

    return (
        <div className="acl-editor">
            <h2 className="editor-title">{addKey ? 'Parent ' : ''} Key: {keyData}</h2>

            {(addKey && !isArray) && (
                <div className="editor-section">
                    <h3>Add Key</h3>
                    <div className="add-key-row">
                        <label>Key:</label>
                        <input
                            type="text"
                            value={newKey}
                            onChange={(e) => setNewKey(e.target.value)}
                            placeholder="Enter key"
                        />
                    </div>
                </div>
            )}
            <div className="editor-section">
                <label>Type:</label>
                <select value={type} onChange={handleTypeChange}>
                    <option value="SIMPLE">Simple</option>
                    <option value="ARRAY">Array</option>
                    <option value="PAIR">Pair</option>
                    {type === "KEY_MAP" && <option value="KEY_MAP">Key Map</option>}
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
                                <option value="True">True</option>
                                <option value="False">False</option>
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
                    <th>Add</th>
                    <th>Modify</th>
                    {/*<th>Remove</th>*/}
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
                        {/*<td>
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
                        </td>*/}
                    </tr>
                ))}
                </tbody>
            </table>

            <button
                className="add-btn"
                onClick={handleAclUpdate}>
                Save ACL
            </button>
            <div className="action-buttons">
                <button className="cancel-btn" onClick={handleCancel}>
                    Cancel
                </button>
                <button className="update-btn" onClick={handleUpdate}>
                    {addKey ? 'Add' : 'Update'}
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
