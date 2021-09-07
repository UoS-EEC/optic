// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include "test/ic.h"

void spi_UCA3_init() {
    // P6.3 CS_N
    P6SEL1 &= ~BIT3;    // GPIO function
    P6SEL0 &= ~BIT3;    // GPIO function
    P6DIR |= BIT3;      // Output
    P6OUT |= BIT3;      // Initial output high (inactive)

    // P6.0 MOSI, P6.1 MISO, P6.2 SCLK
    P6SEL1 &= ~(BIT0 | BIT1 | BIT2);    // UCA3 function
    P6SEL0 |= BIT0 | BIT1 | BIT2;       // UCA3 function

    // SPI - UCA3 init
    // 3-pin mode, CS_N active low
    UCA3CTLW0 = UCSWRST;
    UCA3CTLW0 |= UCMST   |      // Master mode
                 UCSYNC  |      // Synchronous mode
                 UCMSB   |      // MSB first
                 UCCKPH;        // Capture on the first CLK edge
    UCA3CTLW0 |= UCSSEL_2;      // SMCLK in master mode
    // f_BitClock = f_BRClock / prescalerValue = 8MHz / 8 = 1MHz
    UCA3BR1 = 0x00;
    UCA3BR0 = 0x08;
    UCA3CTLW0 &= ~UCSWRST;
}

void set_threshold(uint8_t threshold) {
    P6OUT &= ~BIT3;             // CS_N low, enable

    UCA3IFG &= ~UCRXIFG;
    UCA3TXBUF = 0x00;           // High byte
    while (!(UCA3IFG & UCRXIFG)) {}

    UCA3IFG &= ~UCRXIFG;
    UCA3TXBUF = threshold;      // Low byte
    while (!(UCA3IFG & UCRXIFG)) {}

    P6OUT |= BIT3;              // CS_N high, disable
}

int main(void) {
    // Configure P5.5 button S2
    P5OUT = BIT5;
    P5REN = BIT5;
    P5DIR &= ~BIT5;
    P5IES &= ~BIT5;
    P5IFG = 0;
    P5IE = BIT5;

    spi_UCA3_init();

    for (;;) {
        // __delay_cycles(8000000);
        __bis_SR_register(LPM4_bits | GIE);
        set_threshold(0);
        // __delay_cycles(8000000);
        __bis_SR_register(LPM4_bits | GIE);
        set_threshold(64);
        // __delay_cycles(8000000);
        __bis_SR_register(LPM4_bits | GIE);
        set_threshold(128);
    }

    return 0;
}

void __attribute__((interrupt(PORT5_VECTOR))) P5_ISR(void) {
    switch (__even_in_range(P5IV, P5IV__P5IFG7)) {
        case P5IV__NONE:    break;          // Vector  0:  No interrupt
        case P5IV__P5IFG0:  break;          // Vector  2:  P1.0 interrupt flag
        case P5IV__P5IFG1:  break;          // Vector  4:  P1.1 interrupt flag
        case P5IV__P5IFG2:  break;          // Vector  6:  P1.2 interrupt flag
        case P5IV__P5IFG3:  break;          // Vector  8:  P1.3 interrupt flag
        case P5IV__P5IFG4:  break;          // Vector  10:  P1.4 interrupt flag
        case P5IV__P5IFG5:                  // Vector  12:  P1.5 interrupt flag
            __bic_SR_register_on_exit(LPM4_bits);   // Exit LPM4
            break;
        case P5IV__P5IFG6:  break;          // Vector  14:  P1.6 interrupt flag
        case P5IV__P5IFG7:  break;          // Vector  16:  P1.7 interrupt flag
        default: break;
    }
}
