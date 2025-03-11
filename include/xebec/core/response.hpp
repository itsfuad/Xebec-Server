#pragma once
#include <string>
#include <fstream>

namespace xebec {

class Response {
public:
    std::string status;    // HTTP status line
    std::string body;      // Response body
    std::string headers;   // Response headers
    std::string public_dir;  // Public directory path

    explicit Response(const std::string& public_dir = "") : status("200 OK\r\n"), public_dir(public_dir) {}

    template <typename T>
    Response& operator<<(const T& data) {
        body += data;
        return *this;
    }

    Response& header(const std::string& key, const std::string& value) {
        headers += key + ": " + value + "\r\n";
        return *this;
    }

    Response& status_code(int code) {
        status = std::to_string(code) + " OK\r\n";
        return *this;
    }

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

    Response& json(const std::string& data) {
        header("Content-Type", "application/json");
        body = data;
        return *this;
    }
};

} // namespace xebec 