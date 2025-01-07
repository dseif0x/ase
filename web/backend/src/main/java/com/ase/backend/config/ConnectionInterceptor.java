package com.ase.backend.config;

import com.ase.backend.service.ConnectionService;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.stereotype.Component;
import org.springframework.web.servlet.HandlerInterceptor;

/**
 * An interceptor that runs before every controller request,
 * checking or initializing the connection if needed.
 */
@Component
public class ConnectionInterceptor implements HandlerInterceptor {

    private final ConnectionService connectionService;

    public ConnectionInterceptor(ConnectionService connectionService) {
        this.connectionService = connectionService;
    }

    /**
     * Called before the controller method is invoked.
     *
     * @return true to continue processing the request; false to cancel.
     */
    @Override
    public boolean preHandle(HttpServletRequest request,
                             HttpServletResponse response,
                             Object handler) throws Exception {

        String connectionId = request.getHeader("X-Connection-Id");
        if (connectionId == null) {
            response.sendError(HttpServletResponse.SC_BAD_REQUEST,
                "Missing X-Connection-Id header.");
            return false;
        }

        if (connectionService.getClient(connectionId) == null) {
            response.sendError(HttpServletResponse.SC_UNAUTHORIZED,
                "No DatabaseClient connection. Please log in first.");
            return false;
        }

        return true;
    }
}