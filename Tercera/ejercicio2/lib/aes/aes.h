/* tiny-AES-c (subset): AES-128 block encryption.
 * Simplified and embedded here for educational purposes.
 * Public domain / MIT-like usage.
 */
#ifndef AES_H
#define AES_H

#include <stdint.h>
#include <stddef.h>

struct AES_ctx {
    uint8_t RoundKey[176];
};

void AES_init_ctx(struct AES_ctx* ctx, const uint8_t* key);
void AES_ECB_encrypt(struct AES_ctx* ctx, uint8_t* buf);

#endif // AES_H
