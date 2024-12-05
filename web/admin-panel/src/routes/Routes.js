import React from "react";
import { Routes, Route } from "react-router-dom";
import Home from "../pages/Home";
import Connections from "../pages/Connections";
import AclEditor from "../pages/AclEditor";

function AppRoutes() {
    return (
        <Routes>
            <Route path="/" element={<Home />} />
            <Route path="/Connections" element={<Connections />} />
            <Route path="/edit/:key" element={<AclEditor />} />
        </Routes>
    );
}

export default AppRoutes;
