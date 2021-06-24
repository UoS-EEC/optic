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


// unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
//                       0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
// unsigned char iv[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


// unsigned char input[2048] =
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

// unsigned char __attribute__((section(".persistent"))) output[2048];

void dummy_function(void) {
    atom_func_start(AES_128_ENC);
    // P1OUT |= BIT0;
    uint16_t i = 4000;
    while (i--) {}
    // P1OUT &= ~BIT0;
    // Should see P1.0 be 0->1->0 if this function ends completely
    atom_func_end(AES_128_ENC);
}

int main(void) {
    for (;;) {
        // ******* AESA module test ******
        // aes_128_enc(key, iv, plaintext2, ciphertext2, 2);
        // aes_128_dec(key, iv, ciphertext2, deciphertext2, 2);

        // Encrypt 1KB data
        // aes_128_enc(key, iv, input, output, 64);
        // Encrypt 2KB data
        // aes_128_enc(key, iv, input, output, 128);

        // Decrypt 1KB data
        // aes_128_dec(key, iv, input, input, 64);

        // Decrypt 2KB data
        // aes_128_dec(key, iv, input, input, 128);

        dummy_function();
    }

    return 0;
}
