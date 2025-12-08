#pragma once

#include <array>
#include <cstdint>
#include <string>

#ifdef ARDUINO
#include <FS.h>
#endif

class MD5 {
public:
    MD5();

    void update(const uint8_t* input, size_t length);
    void update(const std::string& input);
    void finalize();
    std::array<uint8_t, 16> digest();
    std::string hexdigest();
    void reset();

    static std::string hash(const std::string& input);
    static std::string hash(const uint8_t* data, size_t length);

#ifdef ARDUINO
    static std::string hashFile(fs::FS& fs, const char* path);
    static std::string hashFile(const char* path);
#else
    static std::string hashFile(const std::string& path);
#endif

private:
    bool finalized_;
    uint32_t state_[4];
    uint64_t bitCount_;
    uint8_t buffer_[64];
    std::array<uint8_t, 16> digest_;

    void transform(const uint8_t block[64]);
    void encode(const uint32_t* input, uint8_t* output, size_t length) const;
    void encode(uint64_t input, uint8_t output[8]) const;
    void decode(const uint8_t* input, uint32_t* output, size_t length) const;
};
