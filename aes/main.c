// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include "aes/aesa.h"
#include "opta/ic.h"

// unsigned char plaintext[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
//                             0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
// unsigned char ciphertext[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// Reference answer
// unsigned char ciphertext[] = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
//                             0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};


// Another set of data
// unsigned char plaintext2[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
//                               0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
//                               0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88,
//                               0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};
// unsigned char ciphertext2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// unsigned char deciphertext2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
unsigned char iv[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


// unsigned char __attribute__((section(".persistent"))) input[2048] =
//     "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo "
//     "ligula eget dolor. Aenean massa. Cum sociis natoque penatibus et magnis "
//     "dis parturient montes, nascetur ridiculus mus. Donec quam felis, "
//     "ultricies nec, pellentesque eu, pretium quis, sem. Nulla consequat massa "
//     "quis enim. Donec pede justo, fringilla vel, aliquet nec, vulputate eget, "
//     "arcu. In enim justo, rhoncus ut, imperdiet a, venenatis vitae, justo. "
//     "Nullam dictum felis eu pede mollis pretium. Integer tincidunt. Cras "
//     "dapibus. Vivamus elementum semper nisi. Aenean vulputate eleifend tellus. "
//     "Aenean leo ligula, porttitor eu, consequat vitae, eleifend ac, enim. "
//     "Aliquam lorem ante, dapibus in, viverra quis, feugiat a, tellus. "
//     "Phasellus viverra nulla ut metus varius laoreet. Quisque rutrum. Aenean "
//     "imperdiet. Etiam ultricies nisi vel augue. Curabitur ullamcorper "
//     "ultricies nisi. Nam eget dui. Lorem ipsum dolor sit amet, consectetuer "
//     "adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa. Cum "
//     "sociis natoque penatibus et magnis dis parturient montes, nascetur "
//     "ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium "
//     "quis, sem. Nulla consequat massa quis enim. Donec pede justo, fringilla "
//     "vel, aliquet nec, vulputate eget, arcu. In enim justo, rhoncus ut, "
//     "imperdiet a, venenatis vitae, justo. Nullam dictum felis eu pede mollis "
//     "pretium. Integer tincidunt. Cras dapibus. Vivamus elementum semper nisi. "
//     "Aenean vulputate eleifend tellus. Aenean leo ligula, porttitor eu, "
//     "consequat vitae, eleifend ac, enim. Aliquam lorem ante, dapibus in, "
//     "viverra quis, feugiat a, tellus. Phasellus viverra nulla ut metus varius "
//     "laoreet. Quisque rutrum. Aenean imperdiet. Etiam ultricies nisi vel "
//     "augue. Curabitur ullamcorper ultricies nisi. Nam eget dui. Lorem ipsum "
//     "dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget "
//     "dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis "
//     "parturient montes, nascetur ridiculus mus. Donec quam felis, ultricies "
//     "nec, pellentesque eu, pretium quis, sem. Nulla consequat massa quis enim. "
//     "Donec pede justo";

unsigned char __attribute__((section(".persistent"))) input[4096] =
    "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo "
    "ligula eget dolor. Aenean massa. Cum sociis natoque penatibus et magnis "
    "dis parturient montes, nascetur ridiculus mus. Donec quam felis, "
    "ultricies nec, pellentesque eu, pretium quis, sem. Nulla consequat massa "
    "quis enim. Donec pede justo, fringilla vel, aliquet nec, vulputate eget, "
    "arcu. In enim justo, rhoncus ut, imperdiet a, venenatis vitae, justo. "
    "Nullam dictum felis eu pede mollis pretium. Integer tincidunt. Cras "
    "dapibus. Vivamus elementum semper nisi. Aenean vulputate eleifend tellus. "
    "Aenean leo ligula, porttitor eu, consequat vitae, eleifend ac, enim. "
    "Aliquam lorem ante, dapibus in, viverra quis, feugiat a, tellus. "
    "Phasellus viverra nulla ut metus varius laoreet. Quisque rutrum. Aenean "
    "imperdiet. Etiam ultricies nisi vel augue. Curabitur ullamcorper "
    "ultricies nisi. Nam eget dui. Lorem ipsum dolor sit amet, consectetuer "
    "adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa. Cum "
    "sociis natoque penatibus et magnis dis parturient montes, nascetur "
    "ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium "
    "quis, sem. Nulla consequat massa quis enim. Donec pede justo, fringilla "
    "vel, aliquet nec, vulputate eget, arcu. In enim justo, rhoncus ut, "
    "imperdiet a, venenatis vitae, justo. Nullam dictum felis eu pede mollis "
    "pretium. Integer tincidunt. Cras dapibus. Vivamus elementum semper nisi. "
    "Aenean vulputate eleifend tellus. Aenean leo ligula, porttitor eu, "
    "consequat vitae, eleifend ac, enim. Aliquam lorem ante, dapibus in, "
    "viverra quis, feugiat a, tellus. Phasellus viverra nulla ut metus varius "
    "laoreet. Quisque rutrum. Aenean imperdiet. Etiam ultricies nisi vel "
    "augue. Curabitur ullamcorper ultricies nisi. Nam eget dui. Lorem ipsum "
    "dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget "
    "dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis "
    "parturient montes, nascetur ridiculus mus. Donec quam felis, ultricies "
    "nec, pellentesque eu, pretium quis, sem. Nulla consequat massa quis enim. "
    "Donec pede justo"
    "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo "
    "ligula eget dolor. Aenean massa. Cum sociis natoque penatibus et magnis "
    "dis parturient montes, nascetur ridiculus mus. Donec quam felis, "
    "ultricies nec, pellentesque eu, pretium quis, sem. Nulla consequat massa "
    "quis enim. Donec pede justo, fringilla vel, aliquet nec, vulputate eget, "
    "arcu. In enim justo, rhoncus ut, imperdiet a, venenatis vitae, justo. "
    "Nullam dictum felis eu pede mollis pretium. Integer tincidunt. Cras "
    "dapibus. Vivamus elementum semper nisi. Aenean vulputate eleifend tellus. "
    "Aenean leo ligula, porttitor eu, consequat vitae, eleifend ac, enim. "
    "Aliquam lorem ante, dapibus in, viverra quis, feugiat a, tellus. "
    "Phasellus viverra nulla ut metus varius laoreet. Quisque rutrum. Aenean "
    "imperdiet. Etiam ultricies nisi vel augue. Curabitur ullamcorper "
    "ultricies nisi. Nam eget dui. Lorem ipsum dolor sit amet, consectetuer "
    "adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa. Cum "
    "sociis natoque penatibus et magnis dis parturient montes, nascetur "
    "ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium "
    "quis, sem. Nulla consequat massa quis enim. Donec pede justo, fringilla "
    "vel, aliquet nec, vulputate eget, arcu. In enim justo, rhoncus ut, "
    "imperdiet a, venenatis vitae, justo. Nullam dictum felis eu pede mollis "
    "pretium. Integer tincidunt. Cras dapibus. Vivamus elementum semper nisi. "
    "Aenean vulputate eleifend tellus. Aenean leo ligula, porttitor eu, "
    "consequat vitae, eleifend ac, enim. Aliquam lorem ante, dapibus in, "
    "viverra quis, feugiat a, tellus. Phasellus viverra nulla ut metus varius "
    "laoreet. Quisque rutrum. Aenean imperdiet. Etiam ultricies nisi vel "
    "augue. Curabitur ullamcorper ultricies nisi. Nam eget dui. Lorem ipsum "
    "dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget "
    "dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis "
    "parturient montes, nascetur ridiculus mus. Donec quam felis, ultricies "
    "nec, pellentesque eu, pretium quis, sem. Nulla consequat massa quis enim. "
    "Donec pede justo";

unsigned char __attribute__((section(".persistent"))) output[4096];
// unsigned char __attribute__((section(".persistent"))) output2[4096];

// void dummy_function(uint16_t cnt) {
//     atom_func_start_linear(0, cnt);
//     uint16_t i = cnt * 1000;
//     while (i--) {}
//     atom_func_end_linear(0, cnt);
// }

// void uart_init(void) {
//     // P2.0 UCA0TXD
//     // P2.1 UCA0RXD
//     P2SEL0 &= ~(BIT0 | BIT1);
//     P2SEL1 |=   BIT0 | BIT1;

//     /* 115200 bps on 8MHz SMCLK */
//     UCA0CTL1 |= UCSWRST;                        // Reset State
//     UCA0CTL1 |= UCSSEL__SMCLK;                  // SMCLK

//     // 115200 baud rate
//     // 8M / 115200 = 69.444444...
//     // 69.444 = 16 * 4 + 5 + 0.444444....
//     UCA0BR0 = 4;
//     UCA0BR1 = 0;
//     UCA0MCTLW = UCOS16 | UCBRF0 | UCBRF2 | 0x5500;  // UCBRF = 5, UCBRS = 0x55

//     UCA0CTL1 &= ~UCSWRST;
// }

// void uart_send_str_sz(char* str, unsigned sz) {
//     UCA0IFG &= ~UCTXIFG;
//     while (sz--) {
//         UCA0TXBUF = *str++;
//         while (!(UCA0IFG & UCTXIFG)) {}
//         UCA0IFG &= ~UCTXIFG;
//     }
// }

int main(void) {
    // uart_init();    // Init UART for printing
    // uint16_t j = 1;  // For dummy function test
    for (;;) {
        // ******* Dummy function test *******
        // dummy_function(j);
        // if (++j == 10) {
        //     j = 1;
        // }

        // ******* AESA module test ******
        // aes_128_enc(key, iv, plaintext2, ciphertext2, 2);
        // aes_128_dec(key, iv, ciphertext2, deciphertext2, 2);

        // aes_128_enc(key, iv, input, output, 32);        // Encrypt 512B data
        // aes_128_enc(key, iv, input, output, 64);        // Encrypt 1KB data
        // aes_128_enc(key, iv, input, output, 128);       // Encrypt 2KB data
        // aes_128_enc(key, iv, input, output, 192);       // Encrypt 3KB data
        // aes_128_enc(key, iv, input, output, 255);       // Encrypt ~4KB data

        // aes_128_enc(key, iv, input, output, 16);        // Encrypt 256B data
        // aes_128_enc(key, iv, input, output, 96);        // Encrypt 1.5KB data
        // aes_128_enc(key, iv, input, output, 160);       // Encrypt 2.5KB data
        // aes_128_enc(key, iv, input, output, 224);       // Encrypt 3.5KB data

        // aes_128_dec(key, iv, input, input, 64);         // Decrypt 1KB data
        // aes_128_dec(key, iv, output, output2, 128);     // Decrypt 2KB data
        // aes_128_dec(key, iv, output, output2, 255);     // Decrypt ~4KB data

        // aes_256_enc(key, iv, input, output, 192);       // Encrypt 3KB data
        // aes_256_dec(key, iv, output, output2, 255);     // Decrypt 2KB data

        // aes_256_enc(key, iv, input, output, 64);       // Encrypt 1KB data
        // aes_256_enc(key, iv, input, output, 128);       // Encrypt 2KB data
        // aes_256_enc(key, iv, input, output, 192);       // Encrypt 3KB data
        // aes_256_enc(key, iv, input, output, 255);       // Encrypt 4KB data

        // uart_send_str_sz((char*) input, 64);        // Print plaintext
        // uart_send_str_sz("\n\r", 2);                // Print newline
        // aes_256_enc(key, iv, input, output, 4);     // Encrypt data
        // uart_send_str_sz((char*) output, 64);       // Print ciphertext
        // uart_send_str_sz("\n\r", 2);                // Print newline
        // aes_256_dec(key, iv, output, output2, 4);   // Decrypt data
        // uart_send_str_sz((char*) output2, 64);      // Print deciphertext
        // uart_send_str_sz("\n\r", 2);                // Print newline

        // __delay_cycles(1000000);     // Dummy delay

        // ******* Completion *******
        // P1OUT |= BIT0;
        // __delay_cycles(0xFF);
        // P1OUT &= ~BIT0;

        // __bis_SR_register(LPM3_bits | GIE);
    }

    return 0;
}
