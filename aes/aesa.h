#ifndef AESA_H
#define AESA_H

#include <stdint.h>

void aes_128_enc(uint8_t *plaintext, uint8_t* ciphertext, uint8_t* key, uint8_t* iv, uint8_t num_blocks);

void aes_128_dec(uint8_t* ciphertext, uint8_t* plaintext, uint8_t* key, uint8_t* iv, uint8_t num_blocks);

#endif // AESA_H