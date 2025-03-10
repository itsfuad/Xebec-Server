# Xebec-Server

A modern, header-only C++ HTTP server library with support for WebSockets, templates, and plugins.

## Features

- Header-only library
- Modern C++ design
- Cross-platform support (Windows and Unix-like systems)
- HTTP/1.1 compliant
- WebSocket support
- Template engine
- Plugin system
- Middleware support
- Static file serving
- JSON responses
- Error handling
- Route parameters
- Query string parsing

## Requirements

- C++17 or later
- CMake 3.10 or later
- Compiler with C++17 support

## Installation

### Using CMake

```cmake
# Add to your CMakeLists.txt
add_subdirectory(xebec-server)
target_link_libraries(your_target PRIVATE xebec::xebec)
```

### Manual Installation

1. Clone the repository
2. Copy the `include/xebec` directory to your project's include path
3. Include the main header in your source files:
```cpp
#include <xebec/xebec.hpp>
```

## Quick Start

```cpp
#include <xebec/xebec.hpp>
#include <iostream>

int main() {
    xebec::ServerConfig config;
    config.port = 8080;
    xebec::http_server server(config);
    
    // Set public directory for static files
    server.publicDir("public");
    
    // Add middleware for logging
    server.use([](xebec::Request& req, xebec::Response& res, xebec::MiddlewareContext::NextFunction next) {
        std::cout << "Request received" << std::endl;
        next();
    });
    
    // Add routes
    server.get("/", [](xebec::Request& req, xebec::Response& res) {
        res.html("index.html");
    });
    
    server.get("/api/hello", [](xebec::Request& req, xebec::Response& res) {
        res.json("{\"message\": \"Hello, World!\"}");
    });
    
    // Start server
    server.start();
    return 0;
}
```

## Features in Detail

### Route Parameters

```cpp
server.get("/users/:id", [](xebec::Request& req, xebec::Response& res) {
    res << "User ID: " << req.params.at("id");
});
```

### WebSocket Support

```cpp
server.ws("/ws", [](xebec::WebSocketFrame& frame, std::function<void(const xebec::WebSocketFrame&)> send) {
    // Echo back the received message
    xebec::WebSocketFrame response;
    response.fin = true;
    response.opcode = 0x1; // Text frame
    response.payload = frame.payload;
    send(response);
});
```

### Template Engine

```cpp
server.set_template_dir("templates");
server.get("/", [](xebec::Request& req, xebec::Response& res) {
    std::map<std::string, std::string> vars = {
        {"title", "Welcome"},
        {"content", "Hello, World!"}
    };
    server.render(res, "index.html", vars);
});
```

### Error Handling

```cpp
server.use_error_handler([](const xebec::HttpError& e, xebec::Request& req, xebec::Response& res) {
    res.status_code(e.status_code())
       .json("{\"error\": \"" + std::string(e.what()) + "\"}");
});
```

## Building from Source

```bash
git clone https://github.com/yourusername/xebec-server.git
cd xebec-server
mkdir build && cd build
cmake ..
cmake --build .
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.