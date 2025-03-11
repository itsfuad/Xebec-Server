#pragma once
#include <string>

namespace xebec {

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

inline std::vector<unsigned char> base64_decode(const std::string& input) {
    size_t length = input.size();
    if (length % 4 != 0) {
        throw std::invalid_argument("Invalid base64 input length");
    }

    std::vector<unsigned char> ret;
    int i = 0;
    unsigned char char_array_4[4], char_array_3[3];

    for (size_t pos = 0; pos < length; pos++) {
        if (input[pos] == '=') break; // Padding reached
        if (!is_base64(input[pos])) {
            throw std::invalid_argument("Invalid character in base64 string");
        }

        char_array_4[i++] = input[pos];
        if (i == 4) {
            for (int j = 0; j < 4; j++) {
                char_array_4[j] = base64_chars.find(char_array_4[j]);
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0x0F) << 4) + ((char_array_4[2] & 0x3C) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x03) << 6) + char_array_4[3];

            for (int j = 0; j < 3; j++) {
                ret.push_back(char_array_3[j]);
            }
            i = 0;
        }
    }

    if (i > 1) {
        for (int j = i; j < 4; j++) {
            char_array_4[j] = 0;
        }

        for (int j = 0; j < 4; j++) {
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0x0F) << 4) + ((char_array_4[2] & 0x3C) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x03) << 6) + char_array_4[3];

        for (int j = 0; j < i - 1; j++) {
            ret.push_back(char_array_3[j]);
        }
    }

    return ret;
}

inline std::string base64_encode(const unsigned char* input, size_t length) {
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (length--) {
        char_array_3[i++] = *(input++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        // Fill remaining bytes with '\0'
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }
    return ret;
}

} // namespace xebec