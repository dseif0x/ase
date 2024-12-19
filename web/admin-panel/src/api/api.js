const API_BASE_URL = "http://localhost:8080/api";

export async function fetchData() {
    try {
        const response = await fetch(`${API_BASE_URL}/data`);
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
