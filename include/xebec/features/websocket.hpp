#pragma once
#include <vector>
#include <cstdint>

namespace xebec {

enum class WSOpCode : uint8_t {
    CONT = 0x0,
    TEXT = 0x1,
    BIN = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
};

struct WebSocketFrame {
    bool fin;
    bool rsv1, rsv2, rsv3;
    WSOpCode opcode;
    bool mask;
    uint64_t payload_length;
    uint8_t masking_key[4];
    std::vector<uint8_t> payload;
};

} // namespace xebec