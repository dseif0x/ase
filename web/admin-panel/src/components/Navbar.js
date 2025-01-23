import React from "react";
import { Link } from "react-router-dom";
import "../styles/components/Navbar.css"

function Navbar() {
    return (
        <nav className="navbar">
            <ul>
                <li><Link to="/">Home</Link></li>
                <li><Link to="/Connections">Connections</Link></li>
                <li><Link to="/create-user">Create User</Link></li>
            </ul>
        </nav>
    );
}

export default Navbar;
