#pragma once
#include <string>
#include <vector>

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

} // namespace xebec 