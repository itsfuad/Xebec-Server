#pragma once
#include <vector>
#include <cstdint>

namespace xebec {

struct WebSocketFrame {
    bool fin;
    bool rsv1, rsv2, rsv3;
    uint8_t opcode;
    bool mask;
    uint64_t payload_length;
    uint8_t masking_key[4];
    std::vector<uint8_t> payload;
};

} // namespace xebec 