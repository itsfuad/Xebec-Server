# Xebec-Server

A lightweight, modern, **header-only** HTTP server implementation in C++ with support for static and dynamic routing, WebSocket, and middleware.

## Features

- 🚀 High-performance HTTP server implementation
- 📦 **Header-only library** - no complex setup required
- 🔄 Support for GET, POST, PUT, DELETE, and PATCH methods
- 🛣️ Dynamic routing with parameter support
- 📦 Static file serving
- 🔒 Built-in security features
- 🌐 Cross-Platform support (Windows and Unix-like systems)
- 📝 JSON response support
- 🎨 Template engine support
- 🔌 WebSocket support
- 🔄 Middleware support

## Requirements

- C++17 or later
- Windows: Winsock API
- Unix-like systems: POSIX sockets

## Installation

1. Clone the repository:
```bash
git clone https://github.com/yourusername/Xebec-Server.git
```

2. Include the header file in your project:
```cpp
#include "xebec_server.hpp"
```

3. Build your application:
```bash
# Windows (using GCC)
g++ -o app.exe your_app.cpp -lws2_32 -std=c++17

# Unix-like systems
g++ -o app your_app.cpp -std=c++17
```

## Quick Start

```cpp
#include "xebec_server.hpp"

int main() {
    xebec::http_server server;
    
    // Set public directory for static files
    server.publicDir("public");
    
    // Add routes
    server.get("/", [](xebec::Request& req, xebec::Response& res) {
        res.html("index.html");
    });
    
    server.get("/api/users/:id", [](xebec::Request& req, xebec::Response& res) {
        res.json("{\"id\": \"" + req.params["id"] + "\"}");
    });
    
    // Start server
    server.start(4119);
    return 0;
}
```

## API Reference

### Server Configuration

```cpp
// Set public directory for static files
server.publicDir("public");

// Start server on specified port
server.start(port);
```

### Route Handlers

```cpp
// GET request handler
server.get(path, callback);

// POST request handler
server.post(path, callback);

// PUT request handler
server.put(path, callback);

// DELETE request handler
server.delete_(path, callback);

// PATCH request handler
server.patch(path, callback);
```

### Request Object

```cpp
class Request {
    std::map<std::string, std::string> query;  // Query parameters
    std::map<std::string, std::string> params; // Route parameters
    std::string body;                          // Request body
    std::map<std::string, std::string> headers; // Request headers
};
```

### Response Object

```cpp
class Response {
    // Set status code
    Response& status_code(int code);
    
    // Add header
    Response& header(const std::string& key, const std::string& value);
    
    // Send HTML response
    Response& html(const std::string& path);
    
    // Send JSON response
    Response& json(const std::string& data);
    
    // Stream response data
    template<typename T>
    Response& operator<<(const T& data);
};
```

## Examples

### Basic Route
```cpp
server.get("/hello", [](Request& req, Response& res) {
    res << "Hello, World!";
});
```

### Dynamic Route with Parameters
```cpp
server.get("/users/:id/posts/:postId", [](Request& req, Response& res) {
    res.json("{\"userId\": \"" + req.params["id"] + 
             "\", \"postId\": \"" + req.params["postId"] + "\"}");
});
```

### Query Parameters
```cpp
server.get("/search", [](Request& req, Response& res) {
    std::string query = req.query["q"];
    res << "Search results for: " << query;
});
```

### File Upload
```cpp
server.post("/upload", [](Request& req, Response& res) {
    // Handle file upload
    res << "File uploaded successfully";
});
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by Express.js and Flask
- Built with modern C++ practices
- Cross-platform compatibility