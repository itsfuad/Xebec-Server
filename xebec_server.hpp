#pragma once

#include <string>
#include <map>
#include <functional>
#include <thread>
#include <regex>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <WS2spi.h>
#define SOCKET_CLOSE(sock) closesocket(sock)
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_CLOSE(sock) close(sock)
#endif

namespace xebec {

// Split a string by a delimiter. Used to parse the request path.
// Example: split("/about/us", '/') => ["", "about", "us"]
std::vector<std::string> split_(const std::string& path, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream iss(path); // Convert the string to a stream
    while (std::getline(iss, token, delimiter)) {
        tokens.push_back(token); // Add the token to the vector
    }
    return tokens;
}

/**
 * @brief Request class to handle HTTP requests
 */
class Request {
public:
    std::map<std::string, std::string> query;      // Query parameters
    std::map<std::string, std::string> params;     // Route parameters
    std::string body;                              // Request body
    std::map<std::string, std::string> headers;    // Request headers

    Request() = default;
};

/**
 * @brief Response class to handle HTTP responses
 */
class Response {
public:
    std::string status;    // HTTP status line
    std::string body;      // Response body
    std::string headers;   // Response headers

    Response() : status("200 OK\r\n") {}

    /**
     * @brief Stream operator to append data to response body
     */
    template <typename T>
    Response& operator<<(const T& data) {
        body += data;
        return *this;
    }

    /**
     * @brief Add a header to the response
     */
    Response& header(const std::string& key, const std::string& value) {
        headers += key + ": " + value + "\r\n";
        return *this;
    }

    /**
     * @brief Set the status code of the response
     */
    Response& status_code(int code) {
        status = std::to_string(code) + " OK\r\n";
        return *this;
    }

    /**
     * @brief Send an HTML file as the response
     */
    Response& html(const std::string& path) {
        header("Content-Type", "text/html");
        std::fstream file(path, std::ios::in | std::ios::binary);
        if(file.is_open()) {
            body = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
        } else {
            status_code(404) << "File Not Found";
        }
        return *this;
    }

    /**
     * @brief Send a JSON response
     */
    Response& json(const std::string& data) {
        header("Content-Type", "application/json");
        body = data;
        return *this;
    }
};

/**
 * @brief HTTP server class with support for routing and middleware
 */
class http_server {
public:
    http_server() {
#ifdef _WIN32
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            std::cerr << "WSAStartup failed: " << iResult << std::endl;
        }
#endif
    }

    ~http_server() noexcept {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    /**
     * @brief Set the public directory for static files
     */
    void publicDir(const std::string& dir) {
        publicDirPath = dir;
    }

    /**
     * @brief Start the server on the specified port
     */
    void start(int port) {
        SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket == INVALID_SOCKET) {
            std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
            return;
        }

        sockaddr_in service;
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = INADDR_ANY;
        service.sin_port = htons(port);

        if (bind(listen_socket, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR) {
            std::cerr << "bind() failed." << std::endl;
            SOCKET_CLOSE(listen_socket);
            return;
        }

        if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Error listening on socket." << std::endl;
            SOCKET_CLOSE(listen_socket);
            return;
        }

        std::cout << "Server is listening on port " << port << std::endl;

        while (true) {
            SOCKET client_socket = accept(listen_socket, NULL, NULL);
            if (client_socket == INVALID_SOCKET) {
                std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
                SOCKET_CLOSE(listen_socket);
                return;
            }

            std::thread t(&http_server::handle_client, this, client_socket);
            t.detach();
        }
    }

    /**
     * @brief Add a GET route handler
     */
    void get(const std::string& path, std::function<void(Request&, Response&)> callback) {
        assignHandler("GET", path, callback);
    }

    /**
     * @brief Add a POST route handler
     */
    void post(const std::string& path, std::function<void(Request&, Response&)> callback) {
        assignHandler("POST", path, callback);
    }

    /**
     * @brief Add a PUT route handler
     */
    void put(const std::string& path, std::function<void(Request&, Response&)> callback) {
        assignHandler("PUT", path, callback);
    }

    /**
     * @brief Add a DELETE route handler
     */
    void delete_(const std::string& path, std::function<void(Request&, Response&)> callback) {
        assignHandler("DELETE", path, callback);
    }

    /**
     * @brief Add a PATCH route handler
     */
    void patch(const std::string& path, std::function<void(Request&, Response&)> callback) {
        assignHandler("PATCH", path, callback);
    }

    /**
     * @brief Add middleware to be executed before route handlers
     */
    void use(std::function<void(Request&, Response&)> middleware) {
        middlewares.push_back(middleware);
    }

private:
    std::map<std::string, std::map<std::string, std::pair<std::string, std::function<void(Request&, Response&)>>>> routes;
    std::string publicDirPath;
    std::vector<std::function<void(Request&, Response&)>> middlewares;

    void assignHandler(const std::string& method, const std::string& path, std::function<void(Request&, Response&)> callback) {
        std::string newPath = std::regex_replace(path, std::regex("/:\\w+/?"), "/([^/]+)/?");
        routes[method][newPath] = std::pair<std::string, std::function<void(Request&, Response&)>>(path, callback);
    }

    void handle_client(SOCKET client_socket) {
        std::string request = read_request(client_socket);
        std::string method = request.substr(0, request.find(' '));
        std::string path = request.substr(request.find(' ') + 1, request.find(' ', request.find(' ') + 1) - request.find(' ') - 1);

        Response response;
        Request req;

        // Parse query parameters
        if (path.find('?') != std::string::npos) {
            std::string query_string = path.substr(path.find('?') + 1);
            path.erase(path.find('?'));
            std::istringstream query_iss(query_string);
            std::string query_pair;
            while (std::getline(query_iss, query_pair, '&')) {
                std::string key = query_pair.substr(0, query_pair.find('='));
                std::string value = query_pair.substr(query_pair.find('=') + 1);
                req.query[key] = value;
            }
        }

        // Run middleware
        for (const auto& middleware : middlewares) {
            middleware(req, response);
        }

        // Handle route
        std::smatch match;
        for (const auto& route : routes[method]) {
            std::string route_path = route.first;

            if (std::regex_match(path, match, std::regex(route_path))) {
                std::regex token_regex(":\\w+");
                std::string originalPath = routes[method][route_path].first;

                std::vector<std::string> tokens = split_(originalPath, '/');
                while (std::regex_search(originalPath, match, token_regex)) {
                    const std::string match_token = match.str();
                    int position = 0;
                    for (int i = 0; i < tokens.size(); i++) {
                        if (tokens[i] == match_token) {
                            position = i;
                            break;
                        }
                    }

                    std::vector<std::string> path_tokens = split_(path, '/');
                    req.params[match_token.substr(1)] = path_tokens[position];

                    originalPath = match.suffix();
                }

                routes[method][route_path].second(req, response);
                serve_static_file(path, response);
                send_response(client_socket, response);
                SOCKET_CLOSE(client_socket);
                return;
            }
        }

        // Serve static files if not matched by any route
        serve_static_file(path, response);
        send_response(client_socket, response);
        SOCKET_CLOSE(client_socket);
    }

    void serve_static_file(const std::string& path, Response& response) {
        std::string content_type;
        std::string file_extension = path.substr(path.find_last_of('.') + 1);
        
        if (file_extension == "html") {
            content_type = "text/html";
        } else if (file_extension == "css") {
            content_type = "text/css";
        } else if (file_extension == "js") {
            content_type = "application/javascript";
        } else if (file_extension == "json") {
            content_type = "application/json";
        } else if (file_extension == "jpg" || file_extension == "jpeg") {
            content_type = "image/jpeg";
        } else if (file_extension == "png") {
            content_type = "image/png";
        } else if (file_extension == "gif") {
            content_type = "image/gif";
        } else {
            content_type = "application/octet-stream";
        }

        std::fstream file(publicDirPath + path, std::ios::in | std::ios::binary);
        if (file) {
            response << std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            response.header("Content-Type", content_type);
        }
    }

    std::string read_request(SOCKET client_socket) {
        std::string request;
        char buffer[1024];
        int bytes_received;
        do {
            bytes_received = recv(client_socket, buffer, 1024, 0);
            if (bytes_received > 0) {
                request.append(buffer, bytes_received);
            }
        } while (bytes_received == 1024);
        return request;
    }

    void send_response(SOCKET client_socket, Response response) {
        response.header("Content-Length", std::to_string(response.body.size()));
        response.header("X-Powered-By", "Xebec-Server/0.1.0");
        response.header("Programming-Language", "C++");
        response.headers += "\r\n";
        std::string res = "HTTP/1.1 " + response.status + response.headers + response.body;
        send(client_socket, res.c_str(), res.size(), 0);
    }
};

} // namespace xebec 