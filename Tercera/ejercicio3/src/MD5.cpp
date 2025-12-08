#include "MD5.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>

#ifndef ARDUINO
#include <fstream>
#else
#include <Arduino.h>
#include <FS.h>
#if defined(__has_include)
#if __has_include(<SPIFFS.h>)
#include <SPIFFS.h>
#endif
#else
#include <SPIFFS.h>
#endif
#endif

namespace {
constexpr std::array<uint8_t, 64> PADDING = [] {
    std::array<uint8_t, 64> pad{};
    pad[0] = 0x80;
    return pad;
}();

inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) | (~x & z);
}

inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) {
    return (x & z) | (y & ~z);
}

inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) {
    return x ^ y ^ z;
}

inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) {
    return y ^ (x | ~z);
}

inline uint32_t rotateLeft(uint32_t value, uint32_t bits) {
    return (value << bits) | (value >> (32U - bits));
}

inline void FF(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a += F(b, c, d) + x + ac;
    a = rotateLeft(a, s);
    a += b;
}

inline void GG(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a += G(b, c, d) + x + ac;
    a = rotateLeft(a, s);
    a += b;
}

inline void HH(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a += H(b, c, d) + x + ac;
    a = rotateLeft(a, s);
    a += b;
}

inline void II(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a += I(b, c, d) + x + ac;
    a = rotateLeft(a, s);
    a += b;
}
}

MD5::MD5() {
    reset();
}

void MD5::reset() {
    finalized_ = false;
    bitCount_ = 0;
    state_[0] = 0x67452301; // A
    state_[1] = 0xefcdab89; // B
    state_[2] = 0x98badcfe; // C
    state_[3] = 0x10325476; // D
    std::fill(std::begin(buffer_), std::end(buffer_), 0);
    digest_.fill(0);
}

void MD5::update(const uint8_t* input, size_t length) {
    if (length == 0 || input == nullptr) {
        return;
    }

    if (finalized_) {
        // Permitir reutilizar la instancia reiniciando el estado.
        reset();
    }

    size_t index = static_cast<size_t>((bitCount_ / 8) % 64);
    bitCount_ += static_cast<uint64_t>(length) * 8ULL;

    size_t partLen = 64 - index;
    size_t i = 0;

    if (length >= partLen) {
        std::memcpy(&buffer_[index], input, partLen);
        transform(buffer_);

        for (i = partLen; i + 63 < length; i += 64) {
            transform(&input[i]);
        }

        index = 0;
    }

    if (i < length) {
        std::memcpy(&buffer_[index], &input[i], length - i);
    }
}

void MD5::update(const std::string& input) {
    update(reinterpret_cast<const uint8_t*>(input.data()), input.size());
}

void MD5::finalize() {
    if (finalized_) {
        return;
    }

    uint8_t bits[8];
    encode(bitCount_, bits);

    size_t index = static_cast<size_t>((bitCount_ / 8) % 64);
    size_t padLen = (index < 56) ? (56 - index) : (120 - index);

    update(PADDING.data(), padLen);
    update(bits, 8);

    encode(state_, digest_.data(), 16);

    finalized_ = true;
}

std::array<uint8_t, 16> MD5::digest() {
    if (!finalized_) {
        finalize();
    }
    return digest_;
}

std::string MD5::hexdigest() {
    auto bytes = digest();
    static const char* digits = "0123456789abcdef";
    std::string output;
    output.reserve(32);

    for (uint8_t byte : bytes) {
        output.push_back(digits[(byte >> 4) & 0x0F]);
        output.push_back(digits[byte & 0x0F]);
    }

    return output;
}

std::string MD5::hash(const std::string& input) {
    MD5 md5;
    md5.update(input);
    return md5.hexdigest();
}

std::string MD5::hash(const uint8_t* data, size_t length) {
    MD5 md5;
    md5.update(data, length);
    return md5.hexdigest();
}

#ifdef ARDUINO
std::string MD5::hashFile(fs::FS& fs, const char* path) {
    if (path == nullptr) {
        return {};
    }

    File file = fs.open(path, "r");
    if (!file) {
        return {};
    }

    MD5 md5;
    std::array<uint8_t, 512> block{};
    while (file.available()) {
        size_t readBytes = file.read(block.data(), block.size());
        if (readBytes == 0) {
            break;
        }
        md5.update(block.data(), readBytes);
    }
    file.close();
    return md5.hexdigest();
}

std::string MD5::hashFile(const char* path) {
#if defined(SPIFFS)
    return hashFile(SPIFFS, path);
#else
    return {};
#endif
}
#else
std::string MD5::hashFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }

    MD5 md5;
    std::array<char, 1024> block{};
    while (file.good()) {
        file.read(block.data(), block.size());
        std::streamsize count = file.gcount();
        if (count > 0) {
            md5.update(reinterpret_cast<const uint8_t*>(block.data()), static_cast<size_t>(count));
        }
    }

    return md5.hexdigest();
}
#endif

void MD5::transform(const uint8_t block[64]) {
    uint32_t a = state_[0];
    uint32_t b = state_[1];
    uint32_t c = state_[2];
    uint32_t d = state_[3];
    uint32_t x[16];

    decode(block, x, 64);

    // Ronda 1
    FF(a, b, c, d, x[0], 7, 0xd76aa478);
    FF(d, a, b, c, x[1], 12, 0xe8c7b756);
    FF(c, d, a, b, x[2], 17, 0x242070db);
    FF(b, c, d, a, x[3], 22, 0xc1bdceee);
    FF(a, b, c, d, x[4], 7, 0xf57c0faf);
    FF(d, a, b, c, x[5], 12, 0x4787c62a);
    FF(c, d, a, b, x[6], 17, 0xa8304613);
    FF(b, c, d, a, x[7], 22, 0xfd469501);
    FF(a, b, c, d, x[8], 7, 0x698098d8);
    FF(d, a, b, c, x[9], 12, 0x8b44f7af);
    FF(c, d, a, b, x[10], 17, 0xffff5bb1);
    FF(b, c, d, a, x[11], 22, 0x895cd7be);
    FF(a, b, c, d, x[12], 7, 0x6b901122);
    FF(d, a, b, c, x[13], 12, 0xfd987193);
    FF(c, d, a, b, x[14], 17, 0xa679438e);
    FF(b, c, d, a, x[15], 22, 0x49b40821);

    // Ronda 2
    GG(a, b, c, d, x[1], 5, 0xf61e2562);
    GG(d, a, b, c, x[6], 9, 0xc040b340);
    GG(c, d, a, b, x[11], 14, 0x265e5a51);
    GG(b, c, d, a, x[0], 20, 0xe9b6c7aa);
    GG(a, b, c, d, x[5], 5, 0xd62f105d);
    GG(d, a, b, c, x[10], 9, 0x02441453);
    GG(c, d, a, b, x[15], 14, 0xd8a1e681);
    GG(b, c, d, a, x[4], 20, 0xe7d3fbc8);
    GG(a, b, c, d, x[9], 5, 0x21e1cde6);
    GG(d, a, b, c, x[14], 9, 0xc33707d6);
    GG(c, d, a, b, x[3], 14, 0xf4d50d87);
    GG(b, c, d, a, x[8], 20, 0x455a14ed);
    GG(a, b, c, d, x[13], 5, 0xa9e3e905);
    GG(d, a, b, c, x[2], 9, 0xfcefa3f8);
    GG(c, d, a, b, x[7], 14, 0x676f02d9);
    GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

    // Ronda 3
    HH(a, b, c, d, x[5], 4, 0xfffa3942);
    HH(d, a, b, c, x[8], 11, 0x8771f681);
    HH(c, d, a, b, x[11], 16, 0x6d9d6122);
    HH(b, c, d, a, x[14], 23, 0xfde5380c);
    HH(a, b, c, d, x[1], 4, 0xa4beea44);
    HH(d, a, b, c, x[4], 11, 0x4bdecfa9);
    HH(c, d, a, b, x[7], 16, 0xf6bb4b60);
    HH(b, c, d, a, x[10], 23, 0xbebfbc70);
    HH(a, b, c, d, x[13], 4, 0x289b7ec6);
    HH(d, a, b, c, x[0], 11, 0xeaa127fa);
    HH(c, d, a, b, x[3], 16, 0xd4ef3085);
    HH(b, c, d, a, x[6], 23, 0x04881d05);
    HH(a, b, c, d, x[9], 4, 0xd9d4d039);
    HH(d, a, b, c, x[12], 11, 0xe6db99e5);
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8);
    HH(b, c, d, a, x[2], 23, 0xc4ac5665);

    // Ronda 4
    II(a, b, c, d, x[0], 6, 0xf4292244);
    II(d, a, b, c, x[7], 10, 0x432aff97);
    II(c, d, a, b, x[14], 15, 0xab9423a7);
    II(b, c, d, a, x[5], 21, 0xfc93a039);
    II(a, b, c, d, x[12], 6, 0x655b59c3);
    II(d, a, b, c, x[3], 10, 0x8f0ccc92);
    II(c, d, a, b, x[10], 15, 0xffeff47d);
    II(b, c, d, a, x[1], 21, 0x85845dd1);
    II(a, b, c, d, x[8], 6, 0x6fa87e4f);
    II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    II(c, d, a, b, x[6], 15, 0xa3014314);
    II(b, c, d, a, x[13], 21, 0x4e0811a1);
    II(a, b, c, d, x[4], 6, 0xf7537e82);
    II(d, a, b, c, x[11], 10, 0xbd3af235);
    II(c, d, a, b, x[2], 15, 0x2ad7d2bb);
    II(b, c, d, a, x[9], 21, 0xeb86d391);

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;

    std::fill(std::begin(x), std::end(x), 0);
}

void MD5::encode(const uint32_t* input, uint8_t* output, size_t length) const {
    for (size_t i = 0, j = 0; j < length; ++i, j += 4) {
        output[j] = static_cast<uint8_t>(input[i] & 0xff);
        output[j + 1] = static_cast<uint8_t>((input[i] >> 8) & 0xff);
        output[j + 2] = static_cast<uint8_t>((input[i] >> 16) & 0xff);
        output[j + 3] = static_cast<uint8_t>((input[i] >> 24) & 0xff);
    }
}

void MD5::encode(uint64_t input, uint8_t output[8]) const {
    for (size_t i = 0; i < 8; ++i) {
        output[i] = static_cast<uint8_t>((input >> (8 * i)) & 0xff);
    }
}

void MD5::decode(const uint8_t* input, uint32_t* output, size_t length) const {
    for (size_t i = 0, j = 0; j < length; ++i, j += 4) {
        output[i] = static_cast<uint32_t>(input[j]) |
                    (static_cast<uint32_t>(input[j + 1]) << 8) |
                    (static_cast<uint32_t>(input[j + 2]) << 16) |
                    (static_cast<uint32_t>(input[j + 3]) << 24);
    }
}
