import React from "react";
import { Routes, Route } from "react-router-dom";
import Home from "../pages/Home";
import Connections from "../pages/Connections";
import AclEditor from "../pages/AclEditor";
import ApiTest from "../pages/ApiTest";

function AppRoutes() {
    return (
        <Routes>
            <Route path="/" element={<Home />} />
            <Route path="/Connections" element={<Connections />} />
            <Route path="/edit/:key" element={<AclEditor />} />
            <Route path="/ApiTest" element={<ApiTest />} />
        </Routes>
    );
}

export default AppRoutes;
