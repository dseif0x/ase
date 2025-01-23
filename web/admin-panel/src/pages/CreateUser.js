import {createUser} from "../api/Api";
import {toast, ToastContainer} from "react-toastify";
import {useState} from "react";
import {useLocation, useNavigate} from "react-router-dom";
import "../styles/pages/CreateUser.css";
import {IoMdArrowBack} from "react-icons/io";

const CreateUser = () => {
    const navigate = useNavigate();

    const location = useLocation();
    let { connectionId } = location.state || {};

    // Form state
    const [username, setUsername] = useState("");
    const [password, setPassword] = useState("");
    const [isAdmin, setIsAdmin] = useState(false);
    const [isSubmitting, setIsSubmitting] = useState(false);

    const handleSubmit = async (e) => {
        e.preventDefault();

        if (!username.trim()) {
            toast.error("Username is required.");
            return;
        }
        if (!password) {
            toast.error("Password is required.");
            return;
        }

        const options = isAdmin ? 1 : 0;
        setIsSubmitting(true);

        const response = await createUser(username, password, options, connectionId);

        if (response.ok) {
            toast.success("User created successfully!");
            handleBack();
        } else {
            if (response.data) toast.error(response.data + " - " + response.message);
            else toast.error(response.message);
        }
        setIsSubmitting(false);
    }

    const handleBack = () => {
        if (window.history.length > 1) {
            navigate(-1);
        } else {
            navigate("/Connections");
        }
    };

    return (
        <div className="create-user-container">
            <button
                onClick={handleBack}
                aria-label="Go Back"
                title="Go Back"
            >
                <IoMdArrowBack/>
            </button>

            <h2>Create New User</h2>
            <form onSubmit={handleSubmit} className="create-user-form">
                <div className="form-group">
                    <label htmlFor="username">Username:</label>
                    <input
                        type="text"
                        id="username"
                        value={username}
                        onChange={(e) => setUsername(e.target.value)}
                        required
                        disabled={isSubmitting}
                    />
                </div>

                <div className="form-group">
                    <label htmlFor="password">Password:</label>
                    <input
                        type="password"
                        id="password"
                        value={password}
                        onChange={(e) => setPassword(e.target.value)}
                        required
                        disabled={isSubmitting}
                    />
                </div>

                <div className="form-group checkbox-group">
                    <input
                        type="checkbox"
                        id="isAdmin"
                        checked={isAdmin}
                        onChange={(e) => setIsAdmin(e.target.checked)}
                        disabled={isSubmitting}
                    />
                    <label htmlFor="isAdmin">Admin</label>
                </div>

                <div className="form-actions">
                    <button
                        type="button"
                        onClick={() => navigate(-1)}
                        disabled={isSubmitting}
                        className="cancel-button"
                    >
                        Cancel
                    </button>
                    <button type="submit" disabled={isSubmitting}>
                        {isSubmitting ? "Creating..." : "Create User"}
                    </button>
                </div>
            </form>
            <ToastContainer
                position="bottom-center"
                autoClose={3000}
                hideProgressBar={false}
                newestOnTop={false}
                closeOnClick
                rtl={false}
                pauseOnFocusLoss
                draggable
                pauseOnHover
                theme="colored"
            />
        </div>
    );
};

export default CreateUser;