# Xebec-Server

# Simple HTTP Server in C++

This is a simple HTTP server implementation written in C++ using the Winsock API for Windows.

## Features

- Supports basic HTTP GET and POST requests.
- Handles static routes and dynamic routes with parameters.
- Parses query parameters from the URL.
- Sends HTML responses with appropriate headers.
- Provides basic error handling and graceful shutdown.

## Dependencies

- Windows: Winsock API
- Unix-like systems: None (standard POSIX sockets are used)

## Usage

1. Clone the repository:


2. Compile the code:

- For Windows, you may need to link against the Winsock library.
- For Unix-like systems, standard POSIX sockets are used, so no additional linking is required.

3. Run the compiled executable:
`g++ -o http.exe http.cpp -lws2_32 -std=c++17` I used GCC here.


By default, the server listens on port 4119. You can change the port by modifying the `start()` function call in the `main()` function.

## Adding Routes

Routes can be added using the `get()` and `post()` methods of the `http_server` class. For example:

```cpp
int main() {

    http_server server;

    server.publicDir("public");

    server.get("/", [](Request& req, Response& res) {
        //send the html file
        res.html("index.html");
    });

    server.get("/about", [](Request& req, Response& res) {
        res.status_code(301) << "About page";
    });

    server.get("/contact", [](Request& req, Response& res) {
        res << "Contact page";
    });

    server.post("/post", [](Request& req, Response& res) {
        res << "POST request";
    });

    server.post("/post/:id", [](Request& req, Response& res) {
        res << "POST request with id: " << req.params["id"];
    });

    server.get("/json", [](Request& req, Response& res) {
        res.json("{\"name\": \"John\", \"age\": 30, \"city\": \"New York\"}");
    });

    server.start(4119);
    return 0;
}
```
