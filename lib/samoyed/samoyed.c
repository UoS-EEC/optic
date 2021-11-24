// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include <stdbool.h>
#include <stddef.h>

#include "lib/ips.h"
#include "lib/samoyed/config.h"

// Header for radio init functions
#ifdef RADIO
#include "radio/hal_spi_rf.h"
#include "radio/msp_nrf24.h"
#endif


#define ENABLE_EXTCOMP_INTERRUPT    P3IE |= BIT0
#define DISABLE_EXTCOMP_INTERRUPT   P3IE &= ~BIT0
#define __low_power_mode_3_on_exit()    __bis_SR_register_on_exit(LPM3_bits | GIE)

extern uint8_t __datastart, __dataend, __romdatastart;  // , __romdatacopysize;
extern uint8_t __bootstackend;
extern uint8_t __bssstart, __bssend;
extern uint8_t __ramtext_low, __ramtext_high, __ramtext_loadLow;
extern uint8_t __stack, __stackend;

void __attribute__((section(".ramtext"), naked))
fastmemcpy(uint8_t *dst, uint8_t *src, size_t len) {
    // r12: dst, r13: src, r14: len
    __asm__("  push    r5\n"        // Save previous r5
            "  tst     r14\n"       // Test for len=0
            "  jz      return\n"
            "  mov     #2, r5\n"    // r5 = word size
            "  xor     r15, r15\n"  // Clear r15
            "  mov     r14, r15\n"  // r15 = len
            "  and     #1, r15\n"   // r15 = len%2
            "  sub     r15, r14\n"  // r14 = len - len%2
            "loopmemcpy:  \n"
            "  mov     @r13+, @r12\n"
            "  add     r5, r12 \n"
            "  sub     #2, r14 \n"  // -2 bytes length
            "  jnz     loopmemcpy \n"
            "  tst     r15\n"
            "  jz      return\n"
            "  mov.b   @r13, @r12\n"  // Move last byte
            "return:      \n"
            "  pop     r5\n"
            "  ret     \n");
}

static void clock_init(void) {
    CSCTL0_H = CSKEY_H;  // Unlock register

    CSCTL1 = DCOFSEL_6;  // DCO = 8 MHz

    // Set ACLK = VLO (~10kHz); SMCLK = DCO; MCLK = DCO;
    CSCTL2 = SELA__VLOCLK + SELS__DCOCLK + SELM__DCOCLK;

    // ACLK: Source/1; SMCLK: Source/1; MCLK: Source/1;
    CSCTL3 = DIVA__1 + DIVS__1 + DIVM__1;      // SMCLK = MCLK = 8 MHz

    CSCTL0_H = 0;  // Lock Register
}

static void gpio_init(void) {
    // Initialize all pins to output low to reduce power consumption
    P1OUT = 0;
    P1DIR = 0xff;
    P2OUT = 0;
    P2DIR = 0xff;
    P3OUT = 0;
    P3DIR = 0xff;
    P4OUT = 0;
    P4DIR = 0xff;
    P5OUT = 0;
    P5DIR = 0xff;
    P6OUT = 0;
    P6DIR = 0xff;
    P7OUT = 0;
    P7DIR = 0xff;
    P8OUT = 0;
    P8DIR = 0xff;
    PJOUT = 0;
    PJDIR = 0xff;
}

// Initialise the external comparator
static void ext_comp_init() {
    // Initialise SPI on UCA3 for the external comparator
    // P6.3 CS_N
    P6SEL1 &= ~BIT3;    // GPIO function
    P6SEL0 &= ~BIT3;    // GPIO function
    P6DIR |= BIT3;      // Output
    P6OUT |= BIT3;      // Initial output high (inactive)

    // P6.0 MOSI, P6.1 MISO, P6.2 SCLK
    // P6SEL1 &= ~(BIT0 | BIT1 | BIT2);    // UCA3 function
    P6SEL0 |= BIT0 | BIT1 | BIT2;       // UCA3 function

    // SPI - UCA3 init
    // 3-pin mode, CS_N active low
    // UCA3CTLW0 = UCSWRST;
    UCA3CTLW0 |= UCSSEL_2;      // SMCLK in master mode
    UCA3CTLW0 |= UCMST   |      // Master mode
                 UCSYNC  |      // Synchronous mode
                 UCMSB   |      // MSB first
                 UCCKPH;        // Capture on the first CLK edge
    // f_BitClock = f_BRClock / prescalerValue = 8MHz / 8 = 1MHz
    UCA3BR1 = 0x00;
    UCA3BR0 = 0x08;
    UCA3CTLW0 &= ~UCSWRST;

    // Initialise P3.0 for the comparator interrupt
    // External pull-up
    P3SEL1 &= ~BIT0;
    P3SEL0 &= ~BIT0;
    P3DIR &= ~BIT0;
    P3REN &= ~BIT0;
    P3IES &= ~BIT0;     // Initial low-to-high transition
    P3IFG &= ~BIT0;
}

// Set the threshold of the external comparator
static void set_threshold(uint8_t threshold) {
    P6OUT &= ~BIT3;             // CS_N low, enable
    UCA3IFG &= ~UCRXIFG;
    UCA3TXBUF = 0x00;           // Send high byte
    while (!(UCA3IFG & UCRXIFG)) {}
    UCA3IFG &= ~UCRXIFG;
    UCA3TXBUF = threshold;      // Send low byte
    while (!(UCA3IFG & UCRXIFG)) {}
    P6OUT |= BIT3;              // CS_N high, disable
}

void __attribute__((interrupt(PORT3_VECTOR))) Port3_ISR(void) {
#ifdef  DEBUG_GPIO
    P8OUT |= BIT1;
#endif
    switch (__even_in_range(P3IV, P3IV__P3IFG7)) {
        case P3IV__NONE:    break;          // Vector  0:  No interrupt
        case P3IV__P3IFG0:                  // Vector  2:  P3.0 interrupt flag
            // Rising edge, high threshold hit
#ifdef  DEBUG_GPIO
            P1OUT &= ~BIT4;     // Debug
#endif
            __low_power_mode_off_on_exit();
            P3IFG &= ~BIT0;
            break;
        case P3IV__P3IFG1:  break;          // Vector  4:  P3.1 interrupt flag
        case P3IV__P3IFG2:  break;          // Vector  6:  P3.2 interrupt flag
        case P3IV__P3IFG3:  break;          // Vector  8:  P3.3 interrupt flag
        case P3IV__P3IFG4:  break;          // Vector  10:  P3.4 interrupt flag
        case P3IV__P3IFG5:  break;          // Vector  12:  P3.5 interrupt flag
        case P3IV__P3IFG6:  break;          // Vector  14:  P3.6 interrupt flag
        case P3IV__P3IFG7:  break;          // Vector  16:  P3.7 interrupt flag
        default: break;
    }
#ifdef  DEBUG_GPIO
    P8OUT &= ~BIT1;
#endif
}

// Boot function
void __attribute__((interrupt(RESET_VECTOR), naked, used, optimize("O0"))) opta_boot() {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    __set_SP_register(&__bootstackend);     // Boot stack
    __dint();                               // Disable interrupts during startup

    gpio_init();
    clock_init();
    ext_comp_init();

#ifdef RADIO
    // Radio init functions
    nrf24_spi_init();
    nrf24_ce_irq_pins_init();
#endif

    PM5CTL0 &= ~LOCKLPM5;       // Disable GPIO power-on default high-impedance mode

    set_threshold(DEFAULT_HI_THRESHOLD);
    COMPARATOR_DELAY;
    if (!(P3IN & BIT0)) {
        P3IFG &= ~BIT0;         // Prevent fake interrupt
        ENABLE_EXTCOMP_INTERRUPT;
#ifdef  DEBUG_GPIO
        P1OUT |= BIT4;          // Debug
#endif
        __low_power_mode_4();   // Enter LPM4 with interrupts enabled
        // Processor sleeps
        // ...
        // Processor wakes up after interrupt (Hi V threshold hit)
    }

    // Boot functions that are mapped to ram (most importantly fastmemcpy)
    uint8_t *dst = &__ramtext_low;
    uint8_t *src = &__ramtext_loadLow;
    size_t len = &__ramtext_high - &__ramtext_low;
    while (len--) {
        *dst++ = *src++;
    }

    // Load .data to RAM
    fastmemcpy(&__datastart, &__romdatastart, &__dataend - &__datastart);

    __set_SP_register(&__stackend);  // Runtime stack

    int main();  // Suppress implicit decl. warning
    main();
}

// Disconnect supply profiling
void atom_func_start(uint8_t func_id) {
    DISABLE_EXTCOMP_INTERRUPT;
    // Run the atomic function...
#ifdef DEBUG_TASK_INDICATOR
    P1OUT |= BIT0;  // Debug
#endif
}

void atom_func_end(uint8_t func_id) {
    // ...Atomic function ends
#ifdef DEBUG_TASK_INDICATOR
    P1OUT &= ~BIT0;
#endif
#ifdef DEBUG_COMPLETION_INDICATOR
    // Indicate completion
    P7OUT |= BIT1;
    __delay_cycles(0xF);
    P7OUT &= ~BIT1;
#endif
    ENABLE_EXTCOMP_INTERRUPT;
}

void atom_func_start_linear(uint8_t func_id, uint16_t param) {
    atom_func_start(func_id);
}

void atom_func_end_linear(uint8_t func_id, uint16_t param) {
    atom_func_end(func_id);
}
