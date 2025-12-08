#ifndef CBCMAC_H
#define CBCMAC_H

#include <stdint.h>
#include <string>
#include <array>

class CBCMAC {
public:
    // key: must be 16 bytes (AES-128)
    CBCMAC(const uint8_t key[16]);
    // Compute MAC of a file; returns MAC as 16-byte array
    std::array<uint8_t, 16> computeMAC(const std::string& filepath);
    // Save a MAC (in hex) to a filename. Returns true if successful.
    bool saveMACHex(const std::array<uint8_t, 16>& mac, const std::string& outPath);
private:
    uint8_t key_[16];
};

#endif // CBCMAC_H
