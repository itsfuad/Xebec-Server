#pragma once
#include <string>
#include <memory>

namespace xebec {

// Forward declaration
class http_server;

class Plugin {
public:
    virtual ~Plugin() = default;
    virtual void init(http_server* server) = 0;
    virtual std::string name() const = 0;
    virtual std::string version() const = 0;
};

} // namespace xebec 