// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include "opta/ic.h"


unsigned char input[2048] =
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

void uart_send_str_sz(char* str, unsigned sz) {
    atom_func_start(UART_SEND_STR_SZ);
    UCA0IFG &= ~UCTXIFG;
    while (sz--) {
        UCA0TXBUF = *str++;
        while (!(UCA0IFG & UCTXIFG)) {}
        UCA0IFG &= ~UCTXIFG;
    }
    atom_func_end(UART_SEND_STR_SZ);
}

int main(void) {
    for (;;) {
        // ******* DMA module test ******
        uart_send_str_sz((char*) input, 0x80);     // Send 128 bytes

        __delay_cycles(0xFF);   // a short delay
    }

    return 0;
}
