#pragma once
#include <stdexcept>
#include <string>

namespace xebec {

// Error handling class
class HttpError : public std::runtime_error {
public:
    HttpError(int status_code, const std::string& message) 
        : std::runtime_error(message), status_code_(status_code) {}
    int status_code() const { return status_code_; }
private:
    int status_code_;
};

} // namespace xebec 