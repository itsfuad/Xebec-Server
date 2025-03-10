#pragma once
#include <string>
#include <map>

namespace xebec {

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

} // namespace xebec 