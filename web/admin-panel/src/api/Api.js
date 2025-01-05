import type ConnectionConfigDTO from "../dto/ConnectionConfigDTO";

const API_BASE_URL = "http://localhost:8080/api";

let loggedIn = false;

export async function get(key) {
    if (!loggedIn) {
        return;
    }
    try {
        const response = fetch(`${API_BASE_URL}/db/get/` + key).then((response) => {
            if (!response.ok) {
                console.log("error getting the value of " + key);
            }
            return response.json();
        })
            .then((data) => {
                console.log('Server response:', data);
            })
            .catch((error) => {
                console.error('Fetch failed:', error);
            });
        console.log(response);
        if (!response.ok) {
            throw new Error(`Msg: ${response.status}`);
        }
        return await response.json();
    } catch (error) {
        console.error("Error fetching data:", error);
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
        return data;
    } catch (error) {
        console.log("error: " + error);
        console.error("Error logging in to server ", error);
        throw error;
    }
}

export async function disconnect(config:ConnectionConfigDTO) {
    if (!loggedIn) {
        return;
    }
    try {
        loggedIn = false;

        console.log("connecting with: " + JSON.stringify(config));
        const response = await fetch(`${API_BASE_URL}/auth/logout`, {
            method: "POST"
        });

        if (!response.ok) {
            console.log(response);
            throw new Error(`HTTP error ${response.status}`);
        }

        const data = await response.text();
        console.log("logout: ", data);
        return data;
    } catch (error) {
        console.log("error: " + error);
        console.error("Error logging in to server ", error);
        throw error;
    }
}

export function isLoggedIn() {
    console.log("logged in: " + loggedIn);
    return loggedIn;
}