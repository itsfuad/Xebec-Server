#pragma once
#include <functional>
#include <vector>
#include "request.hpp"
#include "response.hpp"

namespace xebec {

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

} // namespace xebec 