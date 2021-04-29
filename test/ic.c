// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include "ic.h"

// #define DEBS
#define TRACK_STACK

extern uint8_t __datastart, __dataend, __romdatastart;  // , __romdatacopysize;
extern uint8_t __bootstackend;
extern uint8_t __bssstart, __bssend;
extern uint8_t __ramtext_low, __ramtext_high, __ramtext_loadLow;
extern uint8_t __stack, __stackend;

#define PERSISTENT __attribute__((section(".persistent")))

typedef struct AtomFuncState_s {
    // calibrated:
    // 0: need to calibrate, 1: no need
    uint8_t calibrated;

    // check_fail:
    // set at function entry, reset at exit;
    // so '1' detected at the entry means failed before
    uint8_t check_fail;

    // resume_thr:
    // resume threshold, represented as
    // the resistor tap setting of the internal comparator
    uint8_t resume_thr;

    // backup_thr:
    // backup threshold, represented as
    // the resistor tap setting of the internal comparator
    // uint8_t backup_thr;
} AtomFuncState;

AtomFuncState PERSISTENT atom_state[ATOM_FUNC_NUM];


// indicating the maximum size of each FRAM snapshot
#define STACK_SIZE 0x0100  // 256 bytes stack, should >= __stack_size in .ld
// #define HEAP_SIZE 0x00A0  // 160 bytes heap, if there is heap
#define DATA_SIZE 0x0800  // at most 2KB
#define BSS_SIZE 0x0800  // at most 2KB


// Snapshot of core processor registers
uint16_t PERSISTENT register_snapshot[15];
uint16_t PERSISTENT return_address;

// Snapshots of volatile memory
uint8_t PERSISTENT bss_snapshot[BSS_SIZE];
uint8_t PERSISTENT data_snapshot[DATA_SIZE];
uint8_t PERSISTENT stack_snapshot[STACK_SIZE];
// uint8_t PERSISTENT heap_snapshot[HEAP_SIZE];

uint8_t PERSISTENT snapshot_valid = 0;
uint8_t PERSISTENT suspending;  // from hibernate = 1, from restore = 0


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
    CSCTL0_H = 0xA5;  // Unlock register
    CSCTL1 |= DCOFSEL_6;  // DCO 8MHz

    // Set ACLK = VLO; SMCLK = DCO; MCLK = DCO;
    CSCTL2 = SELA_0 + SELS_3 + SELM_3;

    // ACLK: Source/1; SMCLK: Source/1; MCLK: Source/1;
    CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;  // SMCLK = MCLK = 8 MHz

    CSCTL0_H = 0x01;  // Lock Register
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
    P7OUT = 0;
    P7DIR = 0xff;
    P8OUT = 0;
    P8DIR = 0xff;
    PJOUT = 0;
    PJDIR = 0xff;

    // Disable GPIO power-on default high-impedance mode
    PM5CTL0 &= ~LOCKLPM5;
}

static void comp_init(void) {
    P1DIR  |= BIT2;                 // P1.2 COUT output direction
    P1SEL1 |= BIT2;                 // Select COUT function on P1.2/COUT

    // Setup Comparator_E
    
    // CECTL1 = CEPWRMD_1|             // Normal power mode
    //          CEF      |
    //          CEFDLY_3 ;
    CECTL1 = // CEMRVS   |  // CMRVL selects the Vref - default VREF0
             CEPWRMD_2|  // 1 for Normal power mode / 2 for Ultra-low power mode
             CEF      |  // Output filter enabled
             CEFDLY_3;   // Output filter delay 3600 ns
            // CEMRVL = 0 by default, select VREF0

    CECTL2 = CEREFL_1 |  // VREF 1.2 V is selected
             CERS_2   |  // VREF applied to R-ladder
             CERSEL   |  // to -terminal
             CEREF04 | CEREF02 | CEREF01 | CEREF00|  // Hi V_th, 23(10111)
             CEREF14 | CEREF11 | CEREF10;  // Lo V_th, 19(10011)
            // CEREF_n : V threshold (Volt)
            // 0 : 0.1125
            // 1 : 0.2250
            // 2 : 0.3375
            // 3 : 0.4500
            // 4 : 0.5625
            // 5 : 0.6750
            // 6 : 0.7875
            // 7 : 0.9000
            // 8 : 1.0125
            // 9 : 1.1250
            // 10 : 1.2375
            // 11 : 1.3500
            // 12 : 1.4625
            // 13 : 1.5750
            // 14 : 1.6875
            // 15 : 1.8000
            // 16 : 1.9125
            // 17 : 2.0250
            // 18 : 2.1375
            // 19 : 2.2500
            // 20 : 2.3625
            // 21 : 2.4750
            // 22 : 2.5875
            // 23 : 2.7000
            // 24 : 2.8125
            // 25 : 2.9250
            // 26 : 3.0375
            // 27 : 3.1500
            // 28 : 3.2625
            // 29 : 3.3750
            // 30 : 3.4875
            // 31 : 3.6000

    // while (REFCTL0 & REFGENBUSY);
    // REFCTL0 |= REFVSEL_0 | REFON;  // Select internal Vref (VR+) = 1.2V  Internal Reference ON
    // while (!(REFCTL0 & REFBGRDY));  // Wait for reference generator to settle

    // Channel selection
    P3SEL1 |= BIT0;  // P3.0/C12 for Vcompare
    P3SEL0 |= BIT0;
    CECTL0 = CEIPEN | CEIPSEL_12;   // Enable V+, input channel C12/P3.0
    CECTL3 |= CEPD12;  // Input Buffer Disable @P3.0/C12 (also set by the last line)

    // P1SEL1 |= BIT2;  // P1.2/C2 for Vcompare
    // P1SEL0 |= BIT2;
    // CECTL0 = CEIPEN | CEIPSEL_2;    // Enable V+, input channel C2/P1.2
    // CECTL3 |= CEPD2;  // Input Buffer Disable @P1.2/C2 (also set by the last line)

    CEINT  = CEIE;  // Interrupt enabled
            //  CEIIE;  // Inverted interrupt enabled
            //  CERDYIE;  // Ready interrupt enabled

    CECTL1 |= CEON;  // Turn On Comparator_E
}

// Comparator E interrupt service routine, Hibernus
void __attribute__((interrupt(COMP_E_VECTOR))) Comp_ISR(void) {
    switch (__even_in_range(CEIV, CEIV_CERDYIFG)) {
        case CEIV_NONE: break;
        case CEIV_CEIFG:
            CEINT = (CEINT & ~CEIFG & ~CEIE & ~CEIIFG) | CEIIE;
            // CECTL1 |= CEMRVL;
            
            P1OUT &= ~BIT4;  // debug
            __bic_SR_register_on_exit(LPM4_bits);
            break;
        case CEIV_CEIIFG:
            CEINT = (CEINT & ~CEIIFG & ~CEIIE & ~CEIFG) | CEIE;
            // CECTL1 &= ~CEMRVL;

            // hibernate();
            // if (suspending) {
                P1OUT |= BIT4;  // Debug
                __bis_SR_register_on_exit(LPM4_bits | GIE);
            // } else {
            //     P1OUT &= ~BIT5;  // Debug, come back from restore()
            //     __bic_SR_register_on_exit(LPM4_bits);
            // }
            break;
        // case CEIV_CERDYIFG:
        //     CEINT &= ~CERDYIFG;
        //     break;
        default: break;
    }
}

void __attribute__((interrupt(RESET_VECTOR), naked, used, optimize("O0")))
iclib_boot() {
    WDTCTL = WDTPW | WDTHOLD;            // Stop watchdog timer
    __set_SP_register(&__bootstackend);  // Boot stack
    __bic_SR_register(GIE);              // Disable interrupts during startup

    // Essential initialization stack for wakeup
    // (Avoid being stuck in boot & fail)
    gpio_init();
    comp_init();
    clock_init();

    P1OUT |= BIT4;  // Debug
    __bis_SR_register(LPM4_bits | GIE);  // Enter LPM4 with interrupts enabled
    // Processor sleeps
    // ...
    // Processor wakes up after interrupt (Hi V threshold hit)
    // P1OUT &= ~BIT4;  // Debug

    // Remaining initialization stack for normal execution

    // Boot functions that are mapped to ram (most importantly fastmemcpy)
    uint8_t *dst = &__ramtext_low;
    uint8_t *src = &__ramtext_loadLow;
    size_t len = &__ramtext_high - &__ramtext_low;
    while (len--) {
        *dst++ = *src++;
    }

    // if (snapshot_valid) {
    //     P1OUT |= BIT5;  // Debug, restore() starts
    //     restore();
    //     // Does not return here!!!
    //     // Return to the next line of hibernate()...
    // } else {
    //     // snapshot_valid = 0 when
    //     // previous snapshot failed or 1st boot (both rare)
    //     // Normal boot...
    // }

    // Load .data to RAM
    fastmemcpy(&__datastart, &__romdatastart, &__dataend - &__datastart);

    __set_SP_register(&__stackend);  // Runtime stack

    int main();  // Suppress implicit decl. warning
    main();
}
