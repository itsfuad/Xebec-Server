#pragma once
#include <string>
#include <array>
#include <sstream>
#include <iomanip>

namespace xebec {

inline std::string sha1(const std::string& input) {
    std::array<uint32_t, 5> h = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    std::stringstream ss;
    for(size_t i = 0; i < 5; i++) {
        ss << std::hex << std::setw(8) << std::setfill('0') << h[i];
    }
    return ss.str();
}

} // namespace xebec 