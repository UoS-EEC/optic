// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AES_AESA_H_
#define AES_AESA_H_

#include <stdint.h>

void aes_128_enc(uint8_t* key, uint8_t* iv, uint8_t* plaintext,
                 uint8_t* ciphertext, uint8_t num_blocks);
void aes_128_dec(uint8_t* key, uint8_t* iv, uint8_t* ciphertext,
                 uint8_t* plaintext, uint8_t num_blocks);
void aes_256_enc(uint8_t* key, uint8_t* iv, uint8_t* plaintext,
                 uint8_t* ciphertext, uint8_t num_blocks);
void aes_256_dec(uint8_t* key, uint8_t* iv, uint8_t* ciphertext,
                 uint8_t* plaintext, uint8_t num_blocks);

#endif  // AES_AESA_H_
