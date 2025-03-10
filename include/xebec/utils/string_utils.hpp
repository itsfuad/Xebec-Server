#pragma once
#include <string>
#include <vector>
#include <sstream>

namespace xebec {

inline std::vector<std::string> split_(const std::string& path, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream iss(path);
    while (std::getline(iss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

} // namespace xebec 