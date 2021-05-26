// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include "aes/aesa.h"

#include "lib/ic.h"

// AES accelerator, cipher block chain mode
void aes_128_enc(uint8_t* key, uint8_t* iv, uint8_t* plaintext,
                 uint8_t* ciphertext, uint8_t num_blocks) {
    atom_func_start(AES_128_ENC);

    // Reset AES Module (clears internal state memory)
    AESACTL0 = AESSWRST;

    // Configure AES
    AESACTL0 = AESCMEN    |             // Using DMA
               AESCM__CBC |             // cipher block chaining (CBC) mode
               AESKL__128 |             // 128 bit key length
               AESOP_0    ;             // Encryption

    // Write key into AESAKEY
    // (Load by word. Loading by byte should use AESAKEY0!)
    uint16_t i = 8;  // i = key length in words (128 bit)
    uint16_t* ptr = (uint16_t*) key;
    while (i--) AESAKEY = *ptr++;
    while (AESASTAT & AESBUSY) {}

    // Write IV into AESAXIN
    // (Does not trigger encryption)
    i = 8;
    ptr = (uint16_t*) iv;
    while (i--) AESAXIN = *ptr++;
    while (AESASTAT & AESBUSY) {}

    // DMA Channel selection.
    // AES trigger 0 on channel 0, AES trigger 1 on channel 1
    DMACTL0 = DMA0TSEL_11 | DMA1TSEL_12;

    // Configure DMA0
    DMA0CTL = DMADT_0      |            // Single transfer mode
              DMADSTINCR_3 |            // Increment destination address
              DMASRCINCR_0 |            // Unchanged source address
              DMALEVEL     ;            // AESA uses level sensitive triggers
    DMA0SA = (uint16_t) &AESADOUT;      // Source address
    DMA0DA = (uint16_t) ciphertext;     // Destination address
    DMA0SZ = num_blocks << 3;           // Size in words
    DMA0CTL |= DMAEN;                   // Enable DMA 0

    // Configure DMA1
    DMA1CTL = DMADT_0      |            // Single transfer mode
              DMADSTINCR_0 |            // Unchanged destination address
              DMASRCINCR_3 |            // Increment source address
              DMALEVEL     ;            // AESA uses level sensitive triggers
    DMA1SA = (uint16_t) plaintext;      // Source address
    DMA1DA = (uint16_t) &AESAXDIN;      // Destination address
    DMA1SZ = num_blocks << 3;           // Size in words
    DMA1CTL |= DMAEN;                   // Enable DMA 1

    // Start encryption
    AESACTL1 = num_blocks;

    // End of encryption
    while (!(DMA0CTL & DMAIFG)) {}
    DMA0CTL &= ~DMAIFG;

    atom_func_end(AES_128_ENC);
}

void aes_128_dec(uint8_t* key, uint8_t* iv, uint8_t* ciphertext,
                 uint8_t* plaintext, uint8_t num_blocks) {
    atom_func_start(AES_128_DEC);

    // Reset AES Module (clears internal state memory)
    AESACTL0 = AESSWRST;

    // Generate the last round key for decryption
    AESACTL0 |= AESOP_2   |             // key generation mode
                AESKL__128;             // 128 bit key length
    // loadKey(&AESAKEY, key);         // Load  AES key
    // Write key into AESAKEY
    // (Load by word. Loading by byte should use AESAKEY0!)
    uint16_t i = 8;  // i = key length in words (128 bit)
    uint16_t* ptr = (uint16_t*) key;
    while (i--) AESAKEY = *ptr++;
    while (AESASTAT & AESBUSY) {}

    // Configure AES
    AESACTL0 |= AESCMEN    |            // Using DMA
                AESCM__CBC |            // cipher block chaining (CBC) mode
                AESOP_3    ;            // Decryption
    AESASTAT |= AESKEYWR;               // Use previously generated key

    // DMA Channel selection.
    // AES trigger 0 on channel 0, AES trigger 1 on channel 1
    DMACTL0 = DMA0TSEL_11 | DMA1TSEL_12;
    // AES trigger 2 on channel 2
    DMACTL1 = DMA2TSEL_13;

    // Configure DMA 0, to XOR output
    DMA0CTL = DMADT_0      |
              DMADSTINCR_0 |
              DMASRCINCR_3 |
              DMALEVEL     ;
    DMA0SA = (uint16_t) iv;             // Source address
    DMA0DA = (uint16_t) &AESAXIN;       // Destination address
    DMA0SZ = 8;                         // Size in words
    DMA0CTL |= DMAEN;                   // Enable DMA 0

    // Configure DMA 1
    DMA1CTL = DMADT_0      |
              DMADSTINCR_3 |
              DMASRCINCR_0 |
              DMALEVEL     ;
    DMA1SA = (uint16_t) &AESADOUT;      // Source address
    DMA1DA = (uint16_t) plaintext;      // Destination address
    DMA1SZ = num_blocks << 3;           // Size in words
    DMA1CTL |= DMAEN;                   // Enable DMA 1

    // Configure DMA 2
    DMA2CTL = DMADT_0      |
              DMASRCINCR_3 |
              DMADSTINCR_0 |
              DMALEVEL     ;
    DMA2SA = (uint16_t) ciphertext;     // Source address
    DMA2DA = (uint16_t) &AESADIN;       // Destination address
    DMA2SZ = num_blocks << 3;           // Size in words
    DMA2CTL |= DMAEN;                   // Enable DMA 2

    AESACTL1 = num_blocks;              // Start decryption

    while (!(DMA0CTL & DMAIFG)) {}      // Wait until first block is decrypted
    DMA0CTL &= ~DMAIFG;

    // Configure DMA 0, to XOR output
    DMA0SA = (uint16_t) ciphertext;     // Source address
    DMA0SZ = (num_blocks << 3) - 8;     // Size in words - 8
    DMA0CTL |= DMAEN;                   // Enable DMA 0

    while (!(DMA1CTL & DMAIFG)) {}      // Wait until end of decryption on DMA 1
    DMA0CTL &= ~DMAIFG;

    atom_func_end(AES_128_DEC);
}
