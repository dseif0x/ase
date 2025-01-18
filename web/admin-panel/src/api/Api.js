import type ConnectionConfigDTO from "../dto/ConnectionConfigDTO";

const API_BASE_URL = "http://localhost:8080/api";

export async function get(key, connectionId) {
    try {
        const response = await fetch(`${API_BASE_URL}/db/get/` + key, {
            headers: {
                "X-Connection-Id": connectionId,
            },
        });

        if (!response.ok) {
            throw new Error(`Msg: ${response.status}`);
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

        return data;
    } catch (error) {
        console.error("Error fetching data:", error);
        throw error;
    }
}

export async function set(key, value, force, connectionId) {
    try {
        console.log(`set key: ${key} with value: ${JSON.stringify(value)}`);

        const response = await fetch(`${API_BASE_URL}/db/set`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "X-Connection-Id": connectionId,
            },
            body: JSON.stringify({
                "key": key,
                "value": value,
                "force": force
            }),
        });

        if (!response.ok) {
            throw new Error(`error: ${response.status} ${response.statusText}`);
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
            console.log(response);
            throw new Error(`HTTP error ${response.status}`);
        }

        const data = await response.json();
        return data;
    } catch (error) {
        console.log("error: " + error);
        console.error("Error logging in to server ", error);
        throw error;
    }
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
            console.log(response);
            throw new Error(`HTTP error ${response.status}`);
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
            console.log(response);
            throw new Error(`HTTP error ${response.status}`);
        }

        const data = await response.text();
        return JSON.stringify(data);
    } catch (error) {
        console.log("error: " + error);
        console.error("Error logging in to server ", error);
        return {ok: false, message: error};
    }
}