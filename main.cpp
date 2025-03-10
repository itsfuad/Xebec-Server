#include "xebec_server.hpp"
#include <iostream>

int main() {
    xebec::http_server server;
    
    // Set public directory for static files
    server.publicDir("public");
    
    // Add middleware for logging
    server.use([](xebec::Request& req, xebec::Response& res) {
        std::cout << "Request received" << std::endl;
    });
    
    // Add routes
    server.get("/", [](xebec::Request& req, xebec::Response& res) {
        res.html("index.html");
    });
    
    server.get("/about", [](xebec::Request& req, xebec::Response& res) {
        res.status_code(301) << "About page";
    });
    
    server.get("/contact", [](xebec::Request& req, xebec::Response& res) {
        res << "Contact page";
    });
    
    server.get("/echo/:message", [](xebec::Request& req, xebec::Response& res) {
        res << "Echo: " << req.params.at("message");
    });
    
    server.post("/post", [](xebec::Request& req, xebec::Response& res) {
        res << "POST request";
    });
    
    server.post("/post/:id", [](xebec::Request& req, xebec::Response& res) {
        res << "POST request with id: " << req.params.at("id");
    });
    
    server.get("/json", [](xebec::Request& req, xebec::Response& res) {
        res.json("{\"name\": \"John\", \"age\": 30, \"city\": \"New York\"}");
    });
    
    // Start server
    server.start(4119);
    return 0;
}
