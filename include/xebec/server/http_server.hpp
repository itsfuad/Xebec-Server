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

#include "../core/config.hpp"
#include "../core/error.hpp"
#include "../core/request.hpp"
#include "../core/response.hpp"
#include "../core/middleware.hpp"
#include "../features/plugin.hpp"
#include "../features/websocket.hpp"
#include "../features/template.hpp"
#include "../utils/base64.hpp"
#include "../utils/sha1.hpp"
#include "../utils/string_utils.hpp"

namespace xebec {

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

    void publicDir(const std::string& dir) {
        std::string sanitized_path = std::regex_replace(dir, std::regex("\\/\\/"), "/");
        publicDirPath = sanitized_path;
    }

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

    void get(const std::string& path, std::function<void(Request&, Response&)> callback) {
        assignHandler("GET", path, callback);
    }

    void post(const std::string& path, std::function<void(Request&, Response&)> callback) {
        assignHandler("POST", path, callback);
    }

    void put(const std::string& path, std::function<void(Request&, Response&)> callback) {
        assignHandler("PUT", path, callback);
    }

    void delete_(const std::string& path, std::function<void(Request&, Response&)> callback) {
        assignHandler("DELETE", path, callback);
    }

    void patch(const std::string& path, std::function<void(Request&, Response&)> callback) {
        assignHandler("PATCH", path, callback);
    }

    void use(std::function<void(Request&, Response&, MiddlewareContext::NextFunction)> middleware) {
        middlewares_.push_back(middleware);
    }

    void use_error_handler(std::function<void(const HttpError&, Request&, Response&)> handler) {
        error_handler_ = handler;
    }

    void register_plugin(std::unique_ptr<Plugin> plugin) {
        plugin->init(this);
        plugins_[plugin->name()] = std::move(plugin);
    }

    void ws(const std::string& path,
            std::function<void(WebSocketFrame&, std::function<void(const WebSocketFrame&)>)> handler) {
        ws_handlers_[path] = handler;
    }

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
            std::cout << "Raw request:\n" << request << std::endl;
            
            Response res(publicDirPath);
            parse_request(request, req);
            
            std::cout << "Parsed request - Method: " << req.method 
                      << ", Path: " << req.path << std::endl;
            
            if (req.get_header("Upgrade") == "websocket") {
                handle_websocket(req, client_socket);
                return;
            }

            MiddlewareContext ctx(req, res);
            for (const auto& middleware : middlewares_) {
                ctx.add(middleware);
            }
            ctx.next();
            
            handle_route(req, res);
            
            std::cout << "Response body length: " << res.body.length() << std::endl;
            
            send_response(client_socket, res);
        }
        catch (const HttpError& e) {
            std::cerr << "HTTP Error: " << e.what() << std::endl;
            Response error_response(publicDirPath);
            if (error_handler_) {
                error_handler_(e, req, error_response);
            } else {
                default_error_handler(e, error_response);
            }
            send_response(client_socket, error_response);
        }
        catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
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

        if (std::getline(request_stream, line)) {
            std::istringstream line_stream(line);
            line_stream >> req.method >> req.path >> req.version;
            
            size_t query_pos = req.path.find('?');
            if (query_pos != std::string::npos) {
                std::string query_string = req.path.substr(query_pos + 1);
                req.path = req.path.substr(0, query_pos);
                
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

        while (std::getline(request_stream, line) && line != "\r") {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                req.headers[key] = value;
            }
        }

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
                return;
            }
        }

        serve_static_file(path, res);
    }

    void serve_static_file(const std::string& path, Response& response) {
        std::string content_type;
        std::string file_extension = path.substr(path.find_last_of('.') + 1);
        
        if (file_extension == "html") content_type = "text/html";
        else if (file_extension == "css") content_type = "text/css";
        else if (file_extension == "js") content_type = "application/javascript";
        else if (file_extension == "json") content_type = "application/json";
        else if (file_extension == "jpg" || file_extension == "jpeg") content_type = "image/jpeg";
        else if (file_extension == "png") content_type = "image/png";
        else if (file_extension == "gif") content_type = "image/gif";
        else content_type = "application/octet-stream";

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

    std::string generate_websocket_accept(const std::string& key) {
        const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    
        // Compute SHA-1 hash
        SHA1 sha1;
        sha1.update(key + magic);
        std::vector<uint8_t> hash_bytes = sha1.final_bytes();
    
        // Encode the hash in base64
        return base64_encode(hash_bytes.data(), hash_bytes.size());
    }
    

    void handle_websocket(const Request& req, SOCKET client_socket) {
        std::string key = req.get_header("Sec-WebSocket-Key");
        if (key.empty()) {
            throw HttpError(400, "Invalid WebSocket request");
        }

        std::string accept_key = generate_websocket_accept(key);
        Response res(publicDirPath);
        res.status_code(101)
           .header("Upgrade", "websocket")
           .header("Connection", "Upgrade") // Ensure 'Upgrade' is capitalized
           .header("Sec-WebSocket-Accept", accept_key);
        send_response(client_socket, res);

        while (true) {
            try {
                WebSocketFrame frame = read_websocket_frame(client_socket);
                
                switch (frame.opcode) {
                    case WSOpCode::CLOSE:
                        return;
                    case WSOpCode::PING:
                        frame.opcode = WSOpCode::PONG;
                        send_websocket_frame(client_socket, frame);
                        break;
                    default:
                        auto it = ws_handlers_.find(req.path);
                        if (it != ws_handlers_.end()) {
                            it->second(frame, [client_socket](const WebSocketFrame& response_frame) {
                                send_websocket_frame(client_socket, response_frame);
                            });
                        }
                }
                
            } catch (const std::exception& e) {
                break;
            }
        }
    }

    WebSocketFrame read_websocket_frame(SOCKET socket) {
        WebSocketFrame frame{};
        unsigned char header[2];
        recv(socket, reinterpret_cast<char*>(header), 2, 0);
        
        frame.fin = (header[0] & 0x80) != 0;
        frame.rsv1 = (header[0] & 0x40) != 0;
        frame.rsv2 = (header[0] & 0x20) != 0;
        frame.rsv3 = (header[0] & 0x10) != 0;
        frame.opcode = static_cast<WSOpCode>(header[0] & 0x0F);
        frame.mask = (header[1] & 0x80) != 0;
        frame.payload_length = header[1] & 0x7F;

        if (frame.payload_length == 126) {
            uint16_t length;
            recv(socket, reinterpret_cast<char*>(&length), 2, 0);
            frame.payload_length = ntohs(length);
        } else if (frame.payload_length == 127) {
            uint64_t length;
            recv(socket, reinterpret_cast<char*>(&length), 8, 0);
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

    public:
    static void send_websocket_frame(SOCKET socket, const WebSocketFrame& frame) {
        char header[2] = {
            static_cast<char>((frame.fin << 7) | static_cast<uint8_t>(frame.opcode)),
            static_cast<char>(frame.payload.size() & 0x7F)
        };
        send(socket, header, 2, 0);
        if (!frame.payload.empty()) {
            send(socket, reinterpret_cast<const char*>(frame.payload.data()), frame.payload.size(), 0);
        }
    }
};

} // namespace xebec