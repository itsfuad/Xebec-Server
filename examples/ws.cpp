#include "../include/xebec/xebec.hpp"
#include <iostream>
#include <unordered_map>
#include <mutex>

std::unordered_map<int, SOCKET> clients;
std::mutex clients_mutex;

void broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto& [id, client_socket] : clients) {
        xebec::WebSocketFrame frame;
        frame.fin = true;
        frame.opcode = xebec::WSOpCode::TEXT;
        frame.payload = std::vector<uint8_t>(message.begin(), message.end());
        xebec::http_server::send_websocket_frame(client_socket, frame);
    }
}

int main() {
    xebec::ServerConfig config;
    config.port = 4119;
    xebec::http_server server(config);

    server.publicDir("public");

    server.ws("/chat", [](const xebec::WebSocketFrame& frame, std::function<void(const xebec::WebSocketFrame&)> send) {
        std::string message(frame.payload.begin(), frame.payload.end());
        std::cout << "Received message: " << message << std::endl;
        broadcast(message);
    });

    server.start();
    return 0;
}
