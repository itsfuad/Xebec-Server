#include <iostream>
#include <string>
#include <vector>
#include "../include/xebec/utils/sha1.hpp"
#include "../include/xebec/utils/base64.hpp"

void test_base64() {
    std::string input = "hello World";
    std::string expected_base64 = "aGVsbG8gV29ybGQ=";
    std::string expected_input = "hello World";

    // Encode input to Base64
    std::string base64_result = xebec::base64_encode((unsigned char*)input.c_str(), input.size());
    std::cout << "Base64 Result: " << base64_result << std::endl;
    std::cout << "Expected Base64: " << expected_base64 << std::endl;

    // Decode Base64 result
    std::vector<unsigned char> decoded_result = xebec::base64_decode(base64_result);
    std::string result(decoded_result.begin(), decoded_result.end()); // Convert bytes to string
    std::cout << "Decoded Result: " << result << std::endl;

    // Verify results
    if (base64_result == expected_base64) {
        std::cout << "Base64 Test Passed" << std::endl;
    } else {
        std::cout << "Base64 Test Failed" << std::endl;
    }
}

void test_sha1() {
    std::string input = "hello World. This is a sample string to test SHA1 hashing";

    // Create SHA1 object
    SHA1 sha1;
    sha1.update(input);
    std::string result = sha1.final();

    // Expected SHA1 hash
    std::string expected = "678fbf18cf5a62d8adc863c523bf705771344aee";

    std::cout << "SHA1 Result: " << result << std::endl;
    std::cout << "Expected SHA1: " << expected << std::endl;

    // Verify results
    if (result == expected) {
        std::cout << "SHA1 Test Passed" << std::endl;
    } else {
        std::cout << "SHA1 Test Failed" << std::endl;
    }
}

void test_websocket_accept() {
    std::string key = "dGhlIHNhbXBsZSBub25jZQ=="; // "the sample nonce" in base64
    std::string expected_accept = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";

    // Generate WebSocket accept key
    const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = key + magic;
    SHA1 sha1;
    sha1.update(combined);
    std::vector<uint8_t> hash_bytes = sha1.final_bytes();
    std::string accept_key = xebec::base64_encode(hash_bytes.data(), hash_bytes.size());

    std::cout << "WebSocket Accept Key: " << accept_key << std::endl;
    std::cout << "Expected Accept Key: " << expected_accept << std::endl;

    // Verify results
    if (accept_key == expected_accept) {
        std::cout << "WebSocket Accept Test Passed" << std::endl;
    } else {
        std::cout << "WebSocket Accept Test Failed" << std::endl;
    }
}

int main() {
    test_base64();
    test_sha1();
    test_websocket_accept();
    return 0;
}
