#ifndef AESA_H
#define AESA_H

#include <stdint.h>

void aes_128_enc(uint8_t key[16], uint8_t *plaintext, uint8_t *ciphertext, uint8_t* iv, uint8_t num_blocks);


// void loadKey(volatile unsigned int *ac, unsigned char *key);
// void aesCBC(unsigned char mode, unsigned char *in, unsigned char *out, unsigned char *key, unsigned char *iv);

#endif // AESA_H