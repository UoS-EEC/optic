#include <msp430fr5994.h>
#include "aesa.h"

// AES accelerator, cipher block chain mode
void aes_128_enc(uint8_t* plaintext, uint8_t* ciphertext, uint8_t* key, uint8_t* iv, uint8_t num_blocks)
// (key, IV, plaintext, ciphertext, num_blocks)
{
    // Reset AES Module (clears internal state memory)
    AESACTL0 = AESSWRST;

    // Configure AES for...
    // AESCMEN: using DMA
    // AESCM__CBC: cipher block chaining (CBC) mode
    // AESKL__128: 128 bit key length
    // AESOP_0: encryption
    AESACTL0 = AESCMEN | AESCM__CBC | AESKL__128 | AESOP_0;

    // Write key into AESAKEY
    uint8_t* pos = key;
    uint8_t i = 16;
    while(i--) 
        AESAKEY = (uint8_t) *(pos++); 
    while (!(AESASTAT & AESKEYWR));

    // Write IV into AESAXIN; // Does not trigger encryption.
                              // Assumes that state is reset (=> XORing with Zeros).
    i = 16;
    while(i--)
        AESAXIN = 0x00;
    while (!(AESASTAT & AESDINWR));


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

#define AESBASE 8
#define PACKET_LEN 16

void loadKey(volatile unsigned int *ac, unsigned char *key) {
    unsigned int i=AESBASE;         //i - length key in words (128, 192, 256 bit)
    unsigned int *ptr=(unsigned int*)key;   //point to key
    while(i--)*ac=*ptr++;
    while(AESASTAT&AESBUSY);
}
/* Crypt/Decrypt AES Cipher Block Chaining (CBC) Mode
 * in - input buffer, out - output buffer, key - security key, iv - Initialization Vector
 * mode=0 - encrypt in -> out
 * mode=1 - decrypt in -> out
 * tested 8MHz enc - 0.0075ms, dec - 0.008ms
 */
void aesCBC(unsigned char mode, unsigned char *in, unsigned char *out, unsigned char *key, unsigned char *iv) {
    AESACTL0=AESSWRST;                          // reset AES, AESCMEN=0, AESOPx=00
    AESACTL0=AESKL__128;                            // set KEY length
    // ------------------------------ AES CBC generate key in decrypt mode -------
    if(mode) {
        AESACTL0|=AESOP1;                           // AESOPx=10
        loadKey(&AESAKEY,key);                          // Load  AES key
        AESACTL0|=AESOP0;                           // AESOPx=11
    }
    AESACTL0|=AESCM0 | AESCMEN;                     // AESCMEN=1, AESCMx=01
    // ------------------------------ AES CBC generate key in crypt mode ---------
    if(mode) {
        AESASTAT|=AESKEYWR;
    } else {
        loadKey(&AESAKEY,key);                          // Load AES key
        loadKey(&AESAXIN,iv);                           // Load IV
    }
    DMACTL0=DMA0TSEL_11 | DMA1TSEL_12;                      // Set DMA 0-1 triggers
    if(mode) DMACTL1=DMA2TSEL_13;                           // Set DMA 2 triggers
    DMA0CTL=DMADT_0 | DMALEVEL;                         // configure DMA 0
    if(mode) {
        DMA0CTL|=DMASRCINCR_3 | DMADSTINCR_0;                       // configure DMA 0
        __data20_write_long((unsigned long)&DMA0SA, (unsigned long)iv);         // Source address
        __data20_write_long((unsigned long)&DMA0DA, (unsigned long)&AESAXIN);       // Destination address
    } else {
        DMA0CTL|=DMASRCINCR_0 | DMADSTINCR_3;                       // configure DMA 0
        __data20_write_long((unsigned long)&DMA0SA, (unsigned long)&AESADOUT);      // Source address
        __data20_write_long((unsigned long)&DMA0DA, (unsigned long)out);        // Destination address
    }
    DMA0SZ=mode?8:PACKET_LEN>>1;                          // Size packet in word or 8 word
    DMA0CTL|= DMAEN;                                // Enable DMA 0
    DMA1CTL=DMADT_0 | DMALEVEL;                         // configure DMA 1
    if(mode) {
        DMA1CTL|=DMASRCINCR_0 | DMADSTINCR_3;                       // configure DMA 1
        __data20_write_long((unsigned long)&DMA1SA, (unsigned long)&AESADOUT);      // Source address
        __data20_write_long((unsigned long)&DMA1DA, (unsigned long)out);        // Destination address
    } else {
        DMA1CTL|=DMASRCINCR_3 | DMADSTINCR_0;                       // configure DMA 1
        __data20_write_long((unsigned long)&DMA1SA, (unsigned long)in);         // Source address
        __data20_write_long((unsigned long)&DMA1DA, (unsigned long)&AESAXDIN);      // Destination address
    }
    DMA1SZ=PACKET_LEN>>1;                             // Size packet in word
    DMA1CTL|= DMAEN;                                // Enable DMA 1
    if(mode) {
        DMA2CTL=DMADT_0 | DMALEVEL | DMASRCINCR_3 | DMADSTINCR_0;           // configure DMA 2 for decrypt
        __data20_write_long((unsigned long)&DMA2SA, (unsigned long)in);         // Source address
        __data20_write_long((unsigned long)&DMA2DA, (unsigned long)&AESADIN);       // Destination address
        DMA2SZ=PACKET_LEN>>1;                             // Size packet in word
        DMA2CTL|= DMAEN;                                // Enable DMA 2
    }
    AESACTL1=PACKET_LEN>>4;                           // start AES set AESBLKCNT size packet/128bit(8 byte)
    if(mode) {
        while(!(DMA0CTL & DMAIFG));                         // wait end of AES decrypt first block
        DMA0CTL=DMADT_0 | DMALEVEL | DMASRCINCR_3 | DMADSTINCR_0;           // configure DMA 0
        __data20_write_long((unsigned long)&DMA0SA, (unsigned long)in);         // Source address
        __data20_write_long((unsigned long)&DMA0DA, (unsigned long)&AESAXIN);       // Destination address
        DMA0SZ=PACKET_LEN/2-8;                              // Size in word - Packet-8 word
        DMA0CTL|=DMAEN;                                 // Enable DMA 0
        while(!(DMA1CTL & DMAIFG));                         // wait end of AES decrypt on DMA 1
    } else {
        while(!(DMA0CTL & DMAIFG));                         // wait end of AES encrypt on DMA 0
    }
}//----- end aesCBC