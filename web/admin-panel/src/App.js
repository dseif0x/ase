import React from "react";
import { BrowserRouter as Router } from "react-router-dom";
import Navbar from "./components/Navbar";
import AppRoutes from "./routes/Routes";

function App() {
  return (
      <Router>
          <div>
              <Navbar />
              <AppRoutes />
          </div>
      </Router>
  );
}

export default App;
