#include <msp430fr5994.h>
#include "aesa.h"

// AES accelerator, cipher block chain mode
void aes_128_enc(uint8_t* key, uint8_t* plaintext, uint8_t* ciphertext, uint8_t* iv, uint8_t num_blocks) {
    // Reset AES Module (clears internal state memory)
    AESACTL0 = AESSWRST;

    // Configure AES for...
    // AESCMEN: using DMA
    // AESCM__CBC: cipher block chaining (CBC) mode
    // AESKL__128: 128 bit key length
    // AESOP_0: encryption
    AESACTL0 = AESCMEN | AESCM__CBC | AESKL__128 | AESOP_0;

    // Write key into AESAKEY
    // Load by word. Loading by byte should use AESAKEY0!
    unsigned int i = 8; // i - length key in words (128, 192, 256 bit)
    unsigned int *ptr = (unsigned int*) key;
    while(i--) AESAKEY = *ptr++;

    // Write IV into AESAXI
    // Does not trigger encryption.
    i = 8;
    ptr = (unsigned int*) iv;
    while(i--) AESAXIN = *ptr++;

    // Channel selection. 
    // AES trigger 0 on channel 0, AES trigger 1 on channel 1 
    DMACTL0 = DMA0TSEL_11 | DMA1TSEL_12;

    // DMADT_0: Single transfer mode
    // DMADSTINCR_3: Increment destination address
    // DMASRCINCR_0: Unchanged source address
    // DMADSTBYTE, DMASRCBYTE: Byte transfer
    // DMALEVEL: The AES accelerator makes use of level sensitive triggers
    DMA0CTL = DMADT_0      |
              DMADSTINCR_3 |
              DMASRCINCR_0 |
              DMADSTBYTE   |
              DMASRCBYTE   |
              DMALEVEL     ;
    // Source address
    DMA0SA = (uint16_t) &AESADOUT;
    // Destination address
    DMA0DA = (uint16_t) ciphertext;
    // Size in bytes
    DMA0SZ = num_blocks * 16;
    // Enable
    DMA0CTL |= DMAEN;

    // DMADT_0: Single transfer mode
    // DMADSTINCR_0: Unchanged destination address
    // DMASRCINCR_3: Increment source address
    // DMADSTBYTE, DMASRCBYTE: Byte transfer
    // DMALEVEL: The AES accelerator makes use of level sensitive triggers
    DMA1CTL = DMADT_0      |
              DMADSTINCR_0 |
              DMASRCINCR_3 |
              DMADSTBYTE   |
              DMASRCBYTE   |
              DMALEVEL     ;
    // Source address
    DMA1SA = (uint16_t) plaintext;
    // Destination address
    DMA1DA = (uint16_t) &AESAXDIN;
    // Size in words
    DMA1SZ = num_blocks * 16;
    // Enable
    DMA1CTL |= DMAEN;

    // Start encryption 
    AESACTL1 = num_blocks;

    // End of encryption
    while (!(DMA0CTL & DMAIFG));
    DMA0CTL &= ~DMAIFG;
}
