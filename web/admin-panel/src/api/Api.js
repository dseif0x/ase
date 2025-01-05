import type ConnectionConfigDTO from "../dto/ConnectionConfigDTO";

const API_BASE_URL = "http://localhost:8080/api";

export async function get(key) {
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
        const data = await response.json();
        return data;
    } catch (error) {
        console.error("Error fetching data:", error);
        throw error;
    }
}
