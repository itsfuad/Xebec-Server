#include "./../include/xebec/xebec.hpp"
#include <iostream>
#include <vector>
#include <mutex>
#include <algorithm>
using namespace xebec;

class ChatServer {
    private:
        std::vector<std::function<void(const WebSocketFrame&)>> clients;
        std::mutex clients_mutex;
    
    public:
        void add_client(std::function<void(const WebSocketFrame&)> send_fn) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(send_fn);
        }
    
        void remove_client(std::function<void(const WebSocketFrame&)> send_fn) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(
                std::remove_if(clients.begin(), clients.end(),
                    [&send_fn](const auto& client) {
                        return &client == &send_fn;  // Compare function pointers directly
                    }),
                clients.end()
            );
        }
    
        void broadcast(const WebSocketFrame& frame) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            for (const auto& client : clients) {
                client(frame);
            }
        }
    };  

int main() {
    ServerConfig config;
    config.port = 8080; // Ensure the port matches the WebSocket URL
    http_server server(config);
    ChatServer chat;

    // Serve static HTML file for the chat interface
    server.publicDir("public");

    // Handle WebSocket connections
    server.ws("/chat", [&chat](WebSocketFrame& frame, auto send_fn) {
        static std::function<void(const WebSocketFrame&)> current_send_fn;
        current_send_fn = send_fn;

        // Declare pong frame outside switch
        WebSocketFrame pong{};

        switch (frame.opcode) {
            case WSOpCode::TEXT: // Text frame
                // Broadcast received message to all clients
                chat.broadcast(frame);
                break;

            case WSOpCode::CLOSE: // Close frame
                chat.remove_client(current_send_fn);
                break;

            case WSOpCode::PING: { // Ping frame
                // Respond with pong
                pong.opcode = WSOpCode::PONG;
                send_fn(pong);
                break;
            }

            case WSOpCode::CONT: // Connection established
                chat.add_client(current_send_fn);
                break;
        }
    });

    std::cout << "WebSocket chat server running on port 8080" << std::endl;
    std::cout << "Open http://localhost:8080/chat.html in multiple browsers to test" << std::endl;
    
    server.start();
    return 0;
}