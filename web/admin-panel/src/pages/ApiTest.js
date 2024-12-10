import React, { useEffect, useState } from "react";
import { fetchData } from "../api/api";

function App() {
    const [data, setData] = useState(null);
    const [error, setError] = useState(null);

    useEffect(() => {
        const getData = async () => {
            try {
                const result = await fetchData();
                setData(result);
            } catch (err) {
                setError(err.message);
            }
        };

        getData();
    }, []);

    return (
        <div>
            <h1>Test Data</h1>
            {error ? (
                <p style={{ color: "red" }}>Error: {error}</p>
            ) : data ? (
                <div>
                    <p>Message: {data.message}</p>
                    <p>Status: {data.status}</p>
                    <p>Timestamp: {new Date(data.timestamp).toString()}</p>
                </div>
            ) : (
                <p>Loading...</p>
            )}
        </div>
    );
}

export default App;
