import React from "react";
import { Routes, Route } from "react-router-dom";
import Home from "../pages/Home";
import Connections from "../pages/Connections";
import AclEditor from "../pages/AclEditor";
import CreateUser from "../pages/CreateUser";

function AppRoutes() {
    return (
        <Routes>
            <Route path="/" element={<Home />} />
            <Route path="/Connections" element={<Connections />} />
            <Route path="/edit/:key" element={<AclEditor />} />
            <Route path="/create-user" element={<CreateUser connectionId={''} />}
            />

        </Routes>
    );
}

export default AppRoutes;
