#pragma once
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cstring>

class SHA1 {
public:
    SHA1() { 
        reset();
        //init buffer
        memset(buffer, 0, 64);
    }

    void update(const std::string& data) {
        update(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
    }

    std::vector<uint8_t> final_bytes() {
        uint8_t digest[20];
        finalize(digest);
        return std::vector<uint8_t>(digest, digest + 20);
    }

    static std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
        std::ostringstream result;
        for (uint8_t byte : bytes) {
            result << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }
        return result.str();
    }

    std::string final() {
        uint8_t digest[20];
        finalize(digest);
        std::ostringstream result;
        for (int i = 0; i < 20; i++)
            result << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
        return result.str();
    }

private:
    uint32_t h0, h1, h2, h3, h4;
    uint64_t totalLength;
    uint8_t buffer[64];
    size_t bufferSize;

    void reset() {
        h0 = 0x67452301;
        h1 = 0xEFCDAB89;
        h2 = 0x98BADCFE;
        h3 = 0x10325476;
        h4 = 0xC3D2E1F0;
        totalLength = 0;
        bufferSize = 0;
    }

    void update(const uint8_t* data, size_t length) {
        totalLength += length;
        while (length > 0) {
            size_t reducedBufferSize = 64 - bufferSize;
            size_t copySize = (length < reducedBufferSize) ? length : reducedBufferSize;
            std::memcpy(buffer + bufferSize, data, copySize);
            bufferSize += copySize;
            data += copySize;
            length -= copySize;
            if (bufferSize == 64) {
                processChunk(buffer);
                bufferSize = 0;
            }
        }
    }

    void finalize(uint8_t digest[20]) {
        buffer[bufferSize++] = 0x80;
        if (bufferSize > 56) {
            while (bufferSize < 64)
                buffer[bufferSize++] = 0;
            processChunk(buffer);
            bufferSize = 0;
        }
        while (bufferSize < 56)
            buffer[bufferSize++] = 0;

        uint64_t bitLength = totalLength * 8;
        for (int i = 7; i >= 0; i--)
            buffer[bufferSize++] = (bitLength >> (i * 8)) & 0xFF;

        processChunk(buffer);

        const uint32_t hashValues[] = { h0, h1, h2, h3, h4 };
        for (int i = 0; i < 5; i++) {
            for (int j = 3; j >= 0; j--) {
                digest[i * 4 + (3 - j)] = (hashValues[i] >> (j * 8)) & 0xFF;
            }
        }
    }

    void processChunk(const uint8_t* chunk) {
        uint32_t w[80];
        for (int i = 0; i < 16; i++) {
            w[i] = (chunk[i * 4] << 24) | (chunk[i * 4 + 1] << 16) |
                   (chunk[i * 4 + 2] << 8) | (chunk[i * 4 + 3]);
        }
        for (int i = 16; i < 80; i++) {
            w[i] = rotateLeft(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t temp = rotateLeft(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotateLeft(b, 30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    static uint32_t rotateLeft(uint32_t value, uint32_t bits) {
        return (value << bits) | (value >> (32 - bits));
    }
};