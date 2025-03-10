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
#include <memory>
#include <stdexcept>
#include <future>
#include <array>
#include <iomanip>

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

// Configuration structure for server settings
struct ServerConfig {
    int port = 8080;
    std::string host = "0.0.0.0";
    size_t thread_pool_size = 4;
    size_t max_request_size = 1024 * 1024; // 1MB
    std::string ssl_cert_path;
    std::string ssl_key_path;
    bool enable_cors = false;
    std::string cors_origin = "*";
    std::vector<std::string> allowed_methods = {"GET", "POST", "PUT", "DELETE", "PATCH"};
};

// Error handling class
class HttpError : public std::runtime_error {
public:
    HttpError(int status_code, const std::string& message) 
        : std::runtime_error(message), status_code_(status_code) {}
    int status_code() const { return status_code_; }
private:
    int status_code_;
};

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
 * @brief Enhanced Request class with better parsing and validation
 */
class Request {
public:
    std::map<std::string, std::string> query;      // Query parameters
    std::map<std::string, std::string> params;     // Route parameters
    std::string body;                              // Request body
    std::map<std::string, std::string> headers;    // Request headers
    std::string method;                            // HTTP method
    std::string path;                              // Request path
    std::string version;                           // HTTP version

    bool has_header(const std::string& key) const {
        return headers.find(key) != headers.end();
    }

    std::string get_header(const std::string& key, const std::string& default_value = "") const {
        auto it = headers.find(key);
        return it != headers.end() ? it->second : default_value;
    }

    bool is_secure() const {
        return get_header("X-Forwarded-Proto") == "https" || get_header("X-Forwarded-Ssl") == "on";
    }
};

/**
 * @brief Response class to handle HTTP responses
 */
class Response {
public:
    std::string status;    // HTTP status line
    std::string body;      // Response body
    std::string headers;   // Response headers
    std::string public_dir;  // Public directory path

    Response(const std::string& public_dir = "") : status("200 OK\r\n"), public_dir(public_dir) {}

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
        std::fstream file(public_dir + "/" + path, std::ios::in | std::ios::binary);
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
 * @brief Middleware context for chaining middleware functions
 */
class MiddlewareContext {
public:
    using NextFunction = std::function<void()>;
    
    MiddlewareContext(Request& req, Response& res) : req_(req), res_(res) {}
    
    void next() {
        if (current_middleware_ < middlewares_.size()) {
            auto middleware = middlewares_[current_middleware_++];
            middleware(req_, res_, [this]() { next(); });
        }
    }
    
    void add(std::function<void(Request&, Response&, NextFunction)> middleware) {
        middlewares_.push_back(middleware);
    }
    
private:
    Request& req_;
    Response& res_;
    size_t current_middleware_ = 0;
    std::vector<std::function<void(Request&, Response&, NextFunction)>> middlewares_;
};

// Forward declaration
class http_server;

// Plugin system interface
class Plugin {
public:
    virtual ~Plugin() = default;
    virtual void init(http_server* server) = 0;
    virtual std::string name() const = 0;
    virtual std::string version() const = 0;
};

// WebSocket frame structure
struct WebSocketFrame {
    bool fin;
    bool rsv1, rsv2, rsv3;
    uint8_t opcode;
    bool mask;
    uint64_t payload_length;
    uint8_t masking_key[4];
    std::vector<uint8_t> payload;
};

// Template engine class
class TemplateEngine {
public:
    virtual ~TemplateEngine() = default;
    
    void set_template_dir(const std::string& dir) {
        template_dir_ = dir;
    }
    
    std::string render(const std::string& template_name, 
                      const std::map<std::string, std::string>& vars) {
        std::string template_content = load_template(template_name);
        return render_template(template_content, vars);
    }

protected:
    virtual std::string render_template(const std::string& content,
                                      const std::map<std::string, std::string>& vars) = 0;

private:
    std::string template_dir_;
    
    std::string load_template(const std::string& name) {
        std::ifstream file(template_dir_ + "/" + name);
        if (!file.is_open()) {
            throw HttpError(500, "Template not found: " + name);
        }
        return std::string((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    }
};

// Simple template engine implementation
class SimpleTemplateEngine : public TemplateEngine {
protected:
    std::string render_template(const std::string& content,
                              const std::map<std::string, std::string>& vars) override {
        std::string result = content;
        for (const auto& [key, value] : vars) {
            std::string placeholder = "{{" + key + "}}";
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), value);
                pos += value.length();
            }
        }
        return result;
    }
};

// Simple base64 encoding
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const unsigned char* input, size_t length) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (length--) {
        char_array_3[i++] = *(input++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }
    return ret;
}

// Simple SHA1 implementation
std::string sha1(const std::string& input) {
    std::array<uint32_t, 5> h = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    std::stringstream ss;
    for(size_t i = 0; i < 5; i++) {
        ss << std::hex << std::setw(8) << std::setfill('0') << h[i];
    }
    return ss.str();
}

/**
 * @brief Enhanced HTTP server class with improved architecture
 */
class http_server {
public:
    explicit http_server(const ServerConfig& config = ServerConfig())
        : config_(config), template_engine_(std::make_unique<SimpleTemplateEngine>()) {
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
        //sanitize the path
        std::string sanitized_path = std::regex_replace(dir, std::regex("\\/\\/"), "/"); // remove duplicate slashes
        publicDirPath = sanitized_path;
    }

    /**
     * @brief Start the server on the specified port
     */
    void start() {
        SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket == INVALID_SOCKET) {
            std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
            return;
        }

        sockaddr_in service;
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = INADDR_ANY;
        service.sin_port = htons(config_.port);

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

        std::cout << "Server is listening on port " << config_.port << std::endl;

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
     * @brief Add middleware with next() support
     */
    void use(std::function<void(Request&, Response&, MiddlewareContext::NextFunction)> middleware) {
        middlewares_.push_back(middleware);
    }

    /**
     * @brief Add error handling middleware
     */
    void use_error_handler(std::function<void(const HttpError&, Request&, Response&)> handler) {
        error_handler_ = handler;
    }

    // Plugin management
    void register_plugin(std::unique_ptr<Plugin> plugin) {
        plugin->init(this);
        plugins_[plugin->name()] = std::move(plugin);
    }

    // WebSocket support
    void ws(const std::string& path,
            std::function<void(WebSocketFrame&, std::function<void(const WebSocketFrame&)>)> handler) {
        ws_handlers_[path] = handler;
    }

    // Template engine methods
    void set_template_dir(const std::string& dir) {
        template_engine_->set_template_dir(dir);
    }

    void set_template_engine(std::unique_ptr<TemplateEngine> engine) {
        template_engine_ = std::move(engine);
    }

    Response& render(Response& res, const std::string& template_name,
                    const std::map<std::string, std::string>& vars) {
        std::string content = template_engine_->render(template_name, vars);
        res.header("Content-Type", "text/html");
        res.body = content;
        return res;
    }

private:
    ServerConfig config_;
    std::map<std::string, std::map<std::string, std::pair<std::string, std::function<void(Request&, Response&)>>>> routes;
    std::string publicDirPath;
    std::vector<std::function<void(Request&, Response&, MiddlewareContext::NextFunction)>> middlewares_;
    std::function<void(const HttpError&, Request&, Response&)> error_handler_;
    std::map<std::string, std::unique_ptr<Plugin>> plugins_;
    std::map<std::string, std::function<void(WebSocketFrame&,
                          std::function<void(const WebSocketFrame&)>)>> ws_handlers_;
    std::unique_ptr<TemplateEngine> template_engine_;

    void assignHandler(const std::string& method, const std::string& path, std::function<void(Request&, Response&)> callback) {
        std::string newPath = std::regex_replace(path, std::regex("/:\\w+/?"), "/([^/]+)/?");
        routes[method][newPath] = std::pair<std::string, std::function<void(Request&, Response&)>>(path, callback);
    }

    void handle_client(SOCKET client_socket) {
        Request req;
        try {
            auto request = read_request(client_socket);
            std::cout << "Raw request:\n" << request << std::endl;  // Debug log
            
            Response res(publicDirPath);
            parse_request(request, req);
            
            std::cout << "Parsed request - Method: " << req.method 
                      << ", Path: " << req.path << std::endl;  // Debug log
            
            // Run middleware chain
            MiddlewareContext ctx(req, res);
            for (const auto& middleware : middlewares_) {
                ctx.add(middleware);
            }
            ctx.next();
            
            // Handle routes
            handle_route(req, res);
            
            std::cout << "Response body length: " << res.body.length() << std::endl;  // Debug log
            
            send_response(client_socket, res);
        }
        catch (const HttpError& e) {
            std::cerr << "HTTP Error: " << e.what() << std::endl;  // Debug log
            Response error_response(publicDirPath);
            if (error_handler_) {
                error_handler_(e, req, error_response);
            } else {
                default_error_handler(e, error_response);
            }
            send_response(client_socket, error_response);
        }
        catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;  // Debug log
            Response error_response(publicDirPath);
            default_error_handler(HttpError(500, e.what()), error_response);
            send_response(client_socket, error_response);
        }
        
        SOCKET_CLOSE(client_socket);
    }
    
    void default_error_handler(const HttpError& e, Response& res) {
        res.status_code(e.status_code())
           .header("Content-Type", "application/json")
           .json("{\"error\": \"" + std::string(e.what()) + "\"}");
    }
    
    void parse_request(const std::string& request_str, Request& req) {
        std::istringstream request_stream(request_str);
        std::string line;

        // Parse request line
        if (std::getline(request_stream, line)) {
            std::istringstream line_stream(line);
            line_stream >> req.method >> req.path >> req.version;
            
            // Remove query string from path if present
            size_t query_pos = req.path.find('?');
            if (query_pos != std::string::npos) {
                std::string query_string = req.path.substr(query_pos + 1);
                req.path = req.path.substr(0, query_pos);
                
                // Parse query parameters
                std::istringstream query_stream(query_string);
                std::string param;
                while (std::getline(query_stream, param, '&')) {
                    size_t eq_pos = param.find('=');
                    if (eq_pos != std::string::npos) {
                        req.query[param.substr(0, eq_pos)] = param.substr(eq_pos + 1);
                    }
                }
            }
        }

        // Parse headers
        while (std::getline(request_stream, line) && line != "\r") {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                // Trim leading/trailing whitespace
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                req.headers[key] = value;
            }
        }

        // Read body if present
        if (req.has_header("Content-Length")) {
            size_t content_length = std::stoi(req.get_header("Content-Length"));
            std::vector<char> body_buffer(content_length);
            request_stream.read(body_buffer.data(), content_length);
            req.body = std::string(body_buffer.data(), content_length);
        }
    }

    void handle_route(Request& req, Response& res) {
        std::string method = req.method;
        std::string path = req.path;

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

                routes[method][route_path].second(req, res);
                return; // Return after handling the route
            }
        }

        // Only serve static files if no route matched
        serve_static_file(path, res);
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

        std::fstream file(publicDirPath + "/" + path, std::ios::in | std::ios::binary);
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

    void handle_websocket(const Request& req, SOCKET client_socket) {
        // Implement WebSocket handshake and frame handling
        std::string key = req.get_header("Sec-WebSocket-Key");
        if (key.empty()) {
            throw HttpError(400, "Invalid WebSocket request");
        }

        // Send WebSocket handshake response
        std::string accept_key = generate_websocket_accept(key);
        Response res(publicDirPath);
        res.status_code(101)
           .header("Upgrade", "websocket")
           .header("Connection", "Upgrade")
           .header("Sec-WebSocket-Accept", accept_key);
        send_response(client_socket, res);

        // Handle WebSocket frames
        while (true) {
            try {
                WebSocketFrame frame = read_websocket_frame(client_socket);
                if (frame.opcode == 0x8) { // Close frame
                    break;
                }
                
                auto it = ws_handlers_.find(req.path);
                if (it != ws_handlers_.end()) {
                    it->second(frame, [client_socket](const WebSocketFrame& response_frame) {
                        send_websocket_frame(client_socket, response_frame);
                    });
                }
            } catch (const std::exception& e) {
                break;
            }
        }
    }

    std::string generate_websocket_accept(const std::string& key) {
        const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        std::string combined = key + magic;
        std::string hash = sha1(combined);
        return base64_encode(reinterpret_cast<const unsigned char*>(hash.c_str()), hash.length());
    }

    WebSocketFrame read_websocket_frame(SOCKET socket) {
        WebSocketFrame frame{};
        unsigned char header[2];
        recv(socket, reinterpret_cast<char*>(header), 2, 0);
        
        frame.fin = (header[0] & 0x80) != 0;
        frame.rsv1 = (header[0] & 0x40) != 0;
        frame.rsv2 = (header[0] & 0x20) != 0;
        frame.rsv3 = (header[0] & 0x10) != 0;
        frame.opcode = header[0] & 0x0F;
        frame.mask = (header[1] & 0x80) != 0;
        frame.payload_length = header[1] & 0x7F;

        if (frame.payload_length == 126) {
            uint16_t length;
            recv(socket, reinterpret_cast<char*>(&length), 2, 0);
            frame.payload_length = ntohs(length);
        } else if (frame.payload_length == 127) {
            uint64_t length;
            recv(socket, reinterpret_cast<char*>(&length), 8, 0);
            // Convert network byte order (big-endian) to host byte order
            length = ((length & 0xFF00000000000000ull) >> 56) |
                    ((length & 0x00FF000000000000ull) >> 40) |
                    ((length & 0x0000FF0000000000ull) >> 24) |
                    ((length & 0x000000FF00000000ull) >> 8) |
                    ((length & 0x00000000FF000000ull) << 8) |
                    ((length & 0x0000000000FF0000ull) << 24) |
                    ((length & 0x000000000000FF00ull) << 40) |
                    ((length & 0x00000000000000FFull) << 56);
            frame.payload_length = length;
        }

        if (frame.mask) {
            recv(socket, reinterpret_cast<char*>(frame.masking_key), 4, 0);
        }

        frame.payload.resize(frame.payload_length);
        if (frame.payload_length > 0) {
            recv(socket, reinterpret_cast<char*>(frame.payload.data()), frame.payload_length, 0);
            if (frame.mask) {
                for (size_t i = 0; i < frame.payload_length; ++i) {
                    frame.payload[i] ^= frame.masking_key[i % 4];
                }
            }
        }

        return frame;
    }

    static void send_websocket_frame(SOCKET socket, const WebSocketFrame& frame) {
        // Basic implementation
        char header[2] = {
            static_cast<char>((frame.fin << 7) | frame.opcode),
            static_cast<char>((frame.mask << 7) | (frame.payload.size() & 0x7F))
        };
        send(socket, header, 2, 0);
        if (!frame.payload.empty()) {
            send(socket, reinterpret_cast<const char*>(frame.payload.data()), frame.payload.size(), 0);
        }
    }
};

} // namespace xebec 