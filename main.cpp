#include "include/xebec/xebec.hpp"
#include <iostream>

int main() {

    xebec::ServerConfig config;
    config.port = 4119;
    xebec::http_server server(config);
    
    // Set public directory for static files
    server.publicDir("public");
    
    // Add middleware for logging
    server.use([](const xebec::Request& req, const xebec::Response& res, xebec::MiddlewareContext::NextFunction next) {
        std::cout << "Request received" << std::endl;
        next();
    });
    
    // Add routes
    server.get("/", [](const xebec::Request& req, xebec::Response& res) {
        res.html("index.html");
    });
    
    server.get("/about", [](const xebec::Request& req, xebec::Response& res) {
        res.status_code(301) << "About page";
    });
    
    server.get("/contact", [](const xebec::Request& req, xebec::Response& res) {
        res << "Contact page";
    });
    
    server.get("/echo/:message", [](xebec::Request& req, xebec::Response& res) {
        res << "Echo: " << req.params.at("message");
    });
    
    server.post("/post", [](const xebec::Request& req, xebec::Response& res) {
        res << "POST request";
    });
    
    server.post("/post/:id", [](xebec::Request& req, xebec::Response& res) {
        res << "POST request with id: " << req.params.at("id");
    });
    
    server.get("/json", [](const xebec::Request& req, xebec::Response& res) {
        res.json("{\"name\": \"John\", \"age\": 30, \"city\": \"New York\"}");
    });
    
    // Start server
    server.start();
    return 0;
}
