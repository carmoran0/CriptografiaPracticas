#include "CBCMAC.h"
#include "aes.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>

CBCMAC::CBCMAC(const uint8_t key[16]){
    memcpy(key_, key, 16);
}

static void xor_block(uint8_t out[16], const uint8_t a[16], const uint8_t b[16]){
    for (int i = 0; i < 16; ++i) out[i] = a[i] ^ b[i];
}

std::array<uint8_t, 16> CBCMAC::computeMAC(const std::string& filepath){
    std::array<uint8_t, 16> mac{};
    uint8_t iv[16] = {0};
    uint8_t block[16];
    uint8_t xorbuf[16];
    AES_ctx ctx;
    AES_init_ctx(&ctx, key_);

    FILE* f = fopen(filepath.c_str(), "rb");
    if (!f) {
        // Return MAC all-zero if failed to open file
        return mac;
    }

    while (true) {
        size_t n = fread(block, 1, 16, f);
        if (n == 0) break; // EOF
        if (n < 16) {
            // PKCS#7 padding
            uint8_t pad = 16 - n;
            for (size_t i = n; i < 16; ++i) block[i] = pad;
        }
        xor_block(xorbuf, block, iv);
        AES_ECB_encrypt(&ctx, xorbuf);
        memcpy(iv, xorbuf, 16);
        if (n < 16) break; // this was last block
    }
    fclose(f);
    memcpy(mac.data(), iv, 16);
    return mac;
}

bool CBCMAC::saveMACHex(const std::array<uint8_t, 16>& mac, const std::string& outPath){
    FILE* f = fopen(outPath.c_str(), "wb");
    if (!f) return false;
    for (int i = 0; i < 16; ++i) {
        fprintf(f, "%02x", mac[i]);
    }
    fprintf(f, "\n");
    fclose(f);
    return true;
}
