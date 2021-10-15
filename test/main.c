// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
// #include "opta/ic.h"

#define UART

#ifdef UART
void uart_init(void) {
    P2DIR &= ~(BIT0 | BIT1);
    // P2.0 UCA0TXD
    // P2.1 UCA0RXD
    P2SEL0 &= ~(BIT0 | BIT1);
    P2SEL1 |=  BIT0 | BIT1;

    /* 115200 bps on 8MHz SMCLK */
    UCA0CTL1 |= UCSWRST;                        // Reset State
    UCA0CTL1 |= UCSSEL__SMCLK;                  // SMCLK

    // 115200 baud rate
    // 8M / 115200 = 69.444444...
    // 69.444 = 16 * 4 + 5 + 0.444444....
    UCA0BR0 = 4;
    UCA0BR1 = 0;
    UCA0MCTLW = UCOS16 | UCBRF0 | UCBRF2 | 0x5500;  // UCBRF = 5, UCBRS = 0x55

    UCA0CTL1 &= ~UCSWRST;
}

void uart_send_str(char* str) {
    UCA0IFG &= ~UCTXIFG;
    while (*str != '\0') {
        UCA0TXBUF = *str++;
        while (!(UCA0IFG & UCTXIFG)) {}
        UCA0IFG &= ~UCTXIFG;
    }
}

/* unsigned int to string, decimal format */
char* uitoa_10(uint16_t num, char* const str) {
    // calculate decimal
    char* ptr = str;
    uint16_t modulo;
    do {
        modulo = num % 10;
        num /= 10;
        *ptr++ = '0' + modulo;
    } while (num);

    // reverse string
    *ptr-- = '\0';
    char* ptr1 = str;
    char tmp_char;
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }

    return str;
}
#endif


// uint16_t adc;

uint16_t sample_vcc(void) {
    P8OUT |= BIT0;
    ADC12CTL0 |= ADC12ENC | ADC12SC;  // Start sampling & conversion
    while (!(ADC12IFGR0 & BIT0)) {}
    P8OUT &= ~BIT0;
    return ADC12MEM0;
}

int main(void) {
    // Configure P5.5 button S2
    // P5OUT = BIT5;
    // P5REN = BIT5;
    // P5DIR &= ~BIT5;
    // P5IES &= ~BIT5;
    // P5IFG = 0;
    // P5IE = BIT5;
    uart_init();

    uart_send_str("\n\r");
    uart_send_str("\n\r");
    int i = 10;
    for (;;) {
        char buf[16];
        uart_send_str(uitoa_10(sample_vcc(), buf));
        uart_send_str(" ");
        if (--i == 0) {
            uart_send_str("\n\r");
            i = 10;
        }
        __delay_cycles(50000);
    }

    return 0;
}

// void __attribute__((interrupt(PORT5_VECTOR))) P5_ISR(void) {
//     switch (__even_in_range(P5IV, P5IV__P5IFG7)) {
//         case P5IV__NONE:    break;          // Vector  0:  No interrupt
//         case P5IV__P5IFG0:  break;          // Vector  2:  P1.0 interrupt flag
//         case P5IV__P5IFG1:  break;          // Vector  4:  P1.1 interrupt flag
//         case P5IV__P5IFG2:  break;          // Vector  6:  P1.2 interrupt flag
//         case P5IV__P5IFG3:  break;          // Vector  8:  P1.3 interrupt flag
//         case P5IV__P5IFG4:  break;          // Vector  10:  P1.4 interrupt flag
//         case P5IV__P5IFG5:                  // Vector  12:  P1.5 interrupt flag
//             __bic_SR_register_on_exit(LPM4_bits);   // Exit LPM4
//             break;
//         case P5IV__P5IFG6:  break;          // Vector  14:  P1.6 interrupt flag
//         case P5IV__P5IFG7:  break;          // Vector  16:  P1.7 interrupt flag
//         default: break;
//     }
// }
