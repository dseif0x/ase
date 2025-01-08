import type ConnectionConfigDTO from "../dto/ConnectionConfigDTO";

const API_BASE_URL = "http://localhost:8080/api";

let loggedIn = false;

export async function get(key) {
    if (!loggedIn) {
        return;
    }
    try {
        console.log("get key: " + key);
        const response = await fetch(`${API_BASE_URL}/db/get/` + key);

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

        console.log("response:", data);
        return data;
    } catch (error) {
        console.error("Error fetching data:", error);
        throw error;
    }
}

export async function list(key) {
    if (!loggedIn) {
        return;
    }
    try {
        const key64 = btoa(key);
        const response = await fetch(`${API_BASE_URL}/db/list/` + key64);

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

        console.log("response:", data);
        return data;
    } catch (error) {
        console.error("Error fetching data:", error);
        throw error;
    }
}

// TODO add more types
export async function set(key, value, force) {
    if (!loggedIn) {
        return;
    }
    try {
        console.log(`set key: ${key} with value: ${JSON.stringify(value)}`);

        const response = await fetch(`${API_BASE_URL}/db/set`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify({
                "key": key,
                "value": {
                    "type": "SIMPLE",
                    "value": value,
                    "simpleType": 2
                },
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
    if (loggedIn) {
        return;
    }
    try {
        console.log("connecting with: " + JSON.stringify(config));
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

        loggedIn = true;
        const data = await response.text();
        console.log("response:", data);
        return JSON.parse(data);
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

        loggedIn = false;

        const data = await response.text();
        return JSON.stringify(data);
    } catch (error) {
        console.log("error: " + error);
        console.error("Error logging in to server ", error);
        return {ok: false, message: error};
    }
}

export function isLoggedIn() {
    console.log("logged in: " + loggedIn);
    return loggedIn;
}