import type ConnectionConfigDTO from "../dto/ConnectionConfigDTO";

const API_BASE_URL = "http://localhost:8080/api";

export async function update_acl(connectionId, username, key, permission) {
  try {
    const url = new URL(`${API_BASE_URL}/db/updateScope`);
    const params = {
      username: username,
      key: key,
      options: permission
    };
    Object.keys(params).forEach((key) =>
      url.searchParams.append(key, params[key])
    );
    const response = await fetch(url, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        "X-Connection-Id": connectionId, // Replace with your connection ID
      },
    });

    if (!response.ok) return await handleError(response);

    const contentType = response.headers.get("Content-Type");
    let data;

    if (contentType && contentType.includes("application/json")) {
      data = await response.json();
    } else {
      data = await response.text();
    }

    return data;
  } catch (error) {
    console.error("Error while updating acl", error);
    throw error;
  }
}

export async function get_acl(connectionId) {
    try {
        const response = await fetch(`${API_BASE_URL}/db/getUserPermissions`, {
          method: "POST",
          headers: {
            "X-Connection-Id": connectionId,
            "Content-Type": "application/json"
          },
          body: JSON.stringify({
            "type": "ARRAY",
            "simpleType": null,
            "value": null,
            "array": [],
            "pair": null,
            "keyMap": null,
          })
        });

        if (!response.ok) {
            return await handleError(response)
        }

        const contentType = response.headers.get("Content-Type");
        let data;

        if (contentType && contentType.includes("application/json")) {
            data = await response.json();
        } else {
            data = await response.text();
        }

        return data;
    } catch (error) {
        console.error("Error fetching data:", error);
        throw error;
    }
}

export async function get(key, connectionId) {
    try {
        const response = await fetch(`${API_BASE_URL}/db/get/` + key, {
            headers: {
                "X-Connection-Id": connectionId,
            },
        });

        if (!response.ok) {
            return await handleError(response)
        }

        const contentType = response.headers.get("Content-Type");
        let data;

        if (contentType && contentType.includes("application/json")) {
            data = await response.json();
        } else {
            data = await response.text();
        }

        return data;
    } catch (error) {
        console.error("Error fetching data:", error);
        throw error;
    }
}

export async function list(key, connectionId) {
    if (!connectionId) {
        console.log("failed to call list, because of undefined connectionId");
        return;
    }
    try {
        const key64 = btoa(key);
        const response = await fetch(`${API_BASE_URL}/db/list/` + key64, {
            headers: {
                "X-Connection-Id": connectionId,
            },
        });

        if (!response.ok) {
            return await handleError(response)
        }

        const contentType = response.headers.get("Content-Type");
        let data;

        if (contentType && contentType.includes("application/json")) {
            data = await response.json();
        } else {
            data = await response.text();
        }

        return data;
    } catch (error) {
        console.error("Error fetching data:", error);
        throw error;
    }
}

function replacer(key, value) {
    if (key === "complex" || key === "elementType") {
        return undefined;
    }
    return value;
}

export async function set(key, value, force, connectionId) {
    try {
        console.log(`set key: ${key} with value: ${JSON.stringify(value)}`);

        const jsonString = JSON.parse(JSON.stringify(value, replacer));

        const response = await fetch(`${API_BASE_URL}/db/set`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "X-Connection-Id": connectionId,
            },
            body: JSON.stringify({
                "key": key,
                "value": jsonString,
                "force": force
            }),
        });

        if (!response.ok) {
            return await handleError(response)
        }

        const data = await response.json();
        console.log("response:", data);

        return data;
    } catch (error) {
        console.error("Error: ", error);
        throw error;
    }
}

export async function connect(config:ConnectionConfigDTO) {
    try {
        const response = await fetch(`${API_BASE_URL}/auth/login`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(config),
        });

        if (!response.ok) {
            return await handleError(response)
        }

        const data = await response.json();
        return data;
    } catch (error) {
        console.log("error: " + error);
        console.error("Error logging in to server ", error);
        throw error;
    }
}

export async function handleError(response) {
    let errorDetails;
    try {
        errorDetails = await response.json();
        if (errorDetails.data?.simpleType === "INT") {
            try {
                errorDetails.data = parseInt(errorDetails.data.value);
            } catch {
                errorDetails.data = 9999;
            }
        }
    } catch {
        const text = await response.text();
        errorDetails = {ok: false, message: text, data: 9999};
    }

    return errorDetails;
}
export async function isLoggedIn(connectionId) {
    try {
        console.log("check login with conn id: " + connectionId);
        const response = await fetch(`${API_BASE_URL}/auth/isLoggedIn`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "X-Connection-Id": connectionId,
            },
        });

        if (!response.ok) {
            return await handleError(response)
        }
        const data = await response.json();
        return data;
    } catch (error) {
        console.log("error: " + error);
        console.error("Error logging in to server ", error);
        throw error;
    }
}

export async function disconnect(connectionId: string) {
    try {

        const response = await fetch(`${API_BASE_URL}/auth/logout`, {
            method: "POST",
            headers: {
                "X-Connection-Id": connectionId,
            },
        });

        if (!response.ok) {
            return await handleError(response)
        }

        return await response.json();
    } catch (error) {
        console.log("error: " + error);
        console.error("Error logging in to server ", error);
        return {ok: false, message: error};
    }
}

export async function deleteKey(key, connectionId) {
    if (!connectionId) {
        console.log("failed to call list, because of undefined connectionId");
        return;
    }
    try {
        console.log("deleting key: ", key);
        const response = await fetch(`${API_BASE_URL}/db/delete/` + key, {
            method: "DELETE",
            headers: {
                "X-Connection-Id": connectionId,
            },
        });

        if (!response.ok) {
            let errorDetails = '';
            try {
                errorDetails = await response.json();
            } catch {
                errorDetails = await response.text();
            }
            throw new Error(`Error: ${response.status} - ${errorDetails}`);
        }

        const contentType = response.headers.get("Content-Type");
        let data;

        if (contentType && contentType.includes("application/json")) {
            data = await response.json();
        } else {
            data = await response.text();
        }

        console.log(data);
        return data;
    } catch (error) {
        console.error("Error fetching data:", error);
        throw error;
    }
}
export async function createUser(username: string, password: string, options: number, connectionId: string) {
    try {
        const url = new URL(`${API_BASE_URL}/db/createUser`);

        const params = {
            username: username,
            password: password,
            options: options
        };
        Object.keys(params).forEach((key) =>
            url.searchParams.append(key, params[key])
        );

        const response = await fetch(url, {
            method: "POST",
            headers: {
                "X-Connection-Id": connectionId,
            },
        });

        if (!response.ok) {
            return await handleError(response)
        }

        return await response.json();
    } catch (error) {
        console.log("error: " + error);
        console.error("Error logging in to server ", error);
        return {ok: false, message: error};
    }
}
