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


uint16_t adc_r1;
uint16_t adc_r2;
uint16_t adc_reading;

// Function declarations
void hibernate(void);
void restore(void);

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

static void adc12_init(void) {
    // Configure ADC12

    // 2.0 V reference selected, comment this to use 1.2V
    // REFCTL0 |= REFVSEL_1;

    ADC12CTL0 = ADC12SHT0_2 |  // 16 cycles sample and hold time
                ADC12ON;       // ADC12 on
    ADC12CTL1 = ADC12PDIV_1 |  // Predive by 4
                ADC12SHP;      // SAMPCON is from the sampling timer

    // Default 12bit conversion results, 14cycles conversion time
    // ADC12CTL2 = ADC12RES_2;

    // Use battery monitor (1/2 AVcc)
    // ADC12CTL3 = ADC12BATMAP;  // 1/2AVcc channel selected for ADC ch A31
    // ADC12MCTL0 = ADC12INCH_31 |
    //              ADC12VRSEL_1;  // VR+ = VREF buffered, VR- = Vss

    // Use P1.2
    P1SEL1 |= BIT2;  // Configure P1.2 for ADC
    P1SEL0 |= BIT2;
    ADC12MCTL0 = ADC12INCH_2  |  // Select ch A2 at P1.2
                 ADC12VRSEL_1;   // VR+ = VREF buffered, VR- = Vss

    ADC12IER0 = ADC12IE0;  // Enable ADC conv complete interrupt
}

// ADC12 interrupt service routine
void __attribute__((interrupt(ADC12_B_VECTOR))) ADC12_ISR(void) {
    switch (__even_in_range(ADC12IV, ADC12IV__ADC12RDYIFG)) {
        case ADC12IV__ADC12IFG0:            // Vector 12:  ADC12MEM0
            adc_reading = ADC12MEM0;
            __bic_SR_register_on_exit(LPM0_bits);
            break;
        default: break;
    }
}

static void comp_init(void) {
    // P1DIR  |= BIT2;                 // P1.2 COUT output direction
    // P1SEL1 |= BIT2;                 // Select COUT function on P1.2/COUT

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
            CEINT = (CEINT & ~CEIFG & ~CEIIFG & ~CEIE) | CEIIE;
            // CECTL1 |= CEMRVL;
            
            P1OUT &= ~BIT4;  // debug
            __bic_SR_register_on_exit(LPM4_bits);
            break;
        case CEIV_CEIIFG:
            CEINT = (CEINT & ~CEIIFG & ~CEIFG & ~CEIIE) | CEIE;
            // CECTL1 &= ~CEMRVL;

            hibernate();
            if (suspending) {
                P1OUT |= BIT4;  // Debug
                __bis_SR_register_on_exit(LPM4_bits | GIE);
            } else {
                P1OUT &= ~BIT5;  // Debug, come back from restore()
                __bic_SR_register_on_exit(LPM4_bits);
            }
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
    

    P1OUT |= BIT4;  // Debug
    __bis_SR_register(LPM4_bits + GIE);  // Enter LPM4 with interrupts enabled
    // Processor sleeps
    // ...
    // Processor wakes up after interrupt (Hi V threshold hit)

    // Remaining initialization stack for normal execution
    clock_init();
    adc12_init();

    // Boot functions that are mapped to ram (most importantly fastmemcpy)
    uint8_t *dst = &__ramtext_low;
    uint8_t *src = &__ramtext_loadLow;
    size_t len = &__ramtext_high - &__ramtext_low;
    while (len--) {
        *dst++ = *src++;
    }

    if (snapshot_valid) {
        P1OUT |= BIT5;  // Debug, restore() starts
        restore();
        // Does not return here!!!
        // Return to the next line of hibernate()...
    } else {
        // snapshot_valid = 0 when
        // previous snapshot failed or 1st boot (both rare)
        // Normal boot...
    }

    // Load .data to RAM
    fastmemcpy(&__datastart, &__romdatastart, &__dataend - &__datastart);

    __set_SP_register(&__stackend);  // Runtime stack

    int main();  // Suppress implicit decl. warning
    main();
}

void hibernate(void) {
    // !!! *** DONT TOUCH THE STACK HERE *** !!!

    // copy Core registers to FRAM
    // R0 : PC, save through return_address, not here
    __asm__("mov   sp, &register_snapshot\n"
            "mov   sr, &register_snapshot+2\n"
            "mov   r3, &register_snapshot+4\n"
            "mov   r4, &register_snapshot+6\n"
            "mov   r5, &register_snapshot+8\n"
            "mov   r6, &register_snapshot+10\n"
            "mov   r7, &register_snapshot+12\n"
            "mov   r8, &register_snapshot+14\n"
            "mov   r9, &register_snapshot+16\n"
            "mov   r10, &register_snapshot+18\n"
            "mov   r11, &register_snapshot+20\n"
            "mov   r12, &register_snapshot+22\n"
            "mov   r13, &register_snapshot+24\n"
            "mov   r14, &register_snapshot+26\n"
            "mov   r15, &register_snapshot+28\n");


    // Return to the next line of Hibernate();
    return_address = *(uint16_t *)(__get_SP_register());

    // Save a snapshot of volatile memory
    // allocate to .boot_stack to prevent being overwritten when copying .bss
    static uint8_t *src __attribute__((section(".boot_stack")));
    static uint8_t *dst __attribute__((section(".boot_stack")));
    static size_t len __attribute__((section(".boot_stack")));

    // bss
    src = &__bssstart;
    dst = bss_snapshot;
    len = &__bssend - &__bssstart;
    fastmemcpy(dst, src, len);

    // data
    src = &__datastart;
    dst = data_snapshot;
    len = &__dataend - &__datastart;
    fastmemcpy(dst, src, len);

    // stack
    // stack_low-----[SP-------stack_high]
#ifdef TRACK_STACK
    src = (uint8_t *)register_snapshot[0];  // Saved SP
#else
    src = &__stack;
#endif
    len = &__stackend - src;
    uint16_t offset = (uint16_t)(src - &__stack);
    dst = (uint8_t *)&stack_snapshot[offset];
    fastmemcpy(dst, src, len);

    snapshot_valid = 1;  // For now, don't check snapshot valid
    suspending = 1;
}

void restore(void) {
    // allocate to .boot_stack to prevent being overwritten when copying .bss
    static uint8_t *src __attribute__((section(".boot_stack")));
    static uint8_t *dst __attribute__((section(".boot_stack")));
    static size_t len __attribute__((section(".boot_stack")));

    snapshot_valid = 0;
    /* comment "snapshot_valid = 0"
       for being able to restore from an old snapshot
       if Hibernate() fails to complete
       Here, introducing code re-execution? */

    suspending = 0;

    // data
    dst = &__datastart;
    src = data_snapshot;
    len = &__dataend - &__datastart;
    fastmemcpy(dst, src, len);

    // bss
    dst = &__bssstart;
    src = bss_snapshot;
    len = &__bssend - &__bssstart;
    fastmemcpy(dst, src, len);

    // stack
#ifdef TRACK_STACK
    dst = (uint8_t *)register_snapshot[0];  // Saved stack pointer
#else
    dst = &__stack;  // Save full stack
#endif
    len = &__stackend - dst;
    uint16_t offset = (uint16_t)(dst - &__stack);  // word offset
    src = &stack_snapshot[offset];

    /* Move to separate stack space before restoring original stack. */
    // __set_SP_register((unsigned short)&__cp_stack_high);  // Move to separate stack
    fastmemcpy(dst, src, len);  // Restore default stack
    // Can't use stack here!

    // Restore processor registers
    __asm__("mov   &register_snapshot, sp\n"
            "nop\n"
            "mov   &register_snapshot+2, sr\n"
            "nop\n"
            "mov   &register_snapshot+4, r3\n"
            "mov   &register_snapshot+6, r4\n"
            "mov   &register_snapshot+8, r5\n"
            "mov   &register_snapshot+10, r6\n"
            "mov   &register_snapshot+12, r7\n"
            "mov   &register_snapshot+14, r8\n"
            "mov   &register_snapshot+16, r9\n"
            "mov   &register_snapshot+18, r10\n"
            "mov   &register_snapshot+20, r11\n"
            "mov   &register_snapshot+22, r12\n"
            "mov   &register_snapshot+24, r13\n"
            "mov   &register_snapshot+26, r14\n"
            "mov   &register_snapshot+28, r15\n");

    // Restore return address
    *(uint16_t *)(__get_SP_register()) = return_address;
}

void atom_func_start(uint8_t func_id) {
#ifndef DEBS
    if (atom_state[func_id].check_fail) {
        atom_state[func_id].calibrated = 0;
        // Debug, indicate failed
        P1OUT |= BIT0;
        __delay_cycles(0xF);
        P1OUT &= ~BIT0;
    }

    if (atom_state[func_id].calibrated) {
        // don't need calibration

        // set comparator to the previously calibrated threshold
        CECTL2 = (CECTL2 & ~CEREF0) |
            (CEREF0 & ((uint16_t) atom_state[func_id].resume_thr));

        // CEINT |= CEIIE; // should have been set, just in case
        CECTL1 &= ~CEMRVL;
        __no_operation();  // sleep here if voltage is not high enough...

        // CECTL1 |= CEMRVL;  // turn VREF1 anyway if volt. already high enough
        CEINT &= ~CEIIE;
    } else {
        // calibration

        // set VREF0 = 27/32 * 3.6 = 3.0375 V
        CECTL2 = (CECTL2 & ~CEREF0) | CEREF0_26;

        // CEINT |= CEIIE; // should have been set, just in case
        CECTL1 &= ~CEMRVL;
        __no_operation();  // sleep here if voltage is not high enough...

        // CECTL1 |= CEMRVL;  // turn VREF1 anyway if volt. already high enough
        CEINT &= ~CEIIE;

        // calibrate
        // adc sampling takes 145 us
        P1OUT |= BIT5;  // debug
        ADC12CTL0 |= ADC12ENC | ADC12SC;  // start sampling & conversion
        __bis_SR_register(LPM0_bits | GIE);
        adc_r1 = adc_reading;
        P1OUT &= ~BIT5;  // debug

        P1OUT |= BIT3;  // short-circuit the supply
    }

    // check_fail will be reset in  atom_func_end();
    // if failed, this will remain set on reboots
    atom_state[func_id].check_fail = 1;
#else  // DEBS
    if (atom_state[func_id].check_fail) {
        // Debug, indicate failed
        P1OUT |= BIT0;
        __delay_cycles(0xF);
        P1OUT &= ~BIT0;
    }
    CECTL2 = (CECTL2 & ~CEREF0) | CEREF0_18;
    CECTL1 &= ~CEMRVL;
    __no_operation();  // sleep here if voltage is not high enough

    CEINT &= ~CEIIE;
    CECTL1 |= CEMRVL;

    atom_state[func_id].check_fail = 1;
#endif
    // disable interrupts
    // __bic_SR_register(GIE);
    // CEINT &= ~CEIIE;
    // CECTL1 |= CEMRVL;  // turn to VREF1 anyway, in case
    P1OUT |= BIT5;  // debug, indicate function starts
}

void atom_func_end(uint8_t func_id) {
    P1OUT &= ~BIT5;  // debug, indicate function ends
    // enable interrupts again
    // CEINT |= CEIIE;

#ifndef DEBS
    if (!atom_state[func_id].calibrated) {
        // end of a calibration
        // measure end voltage
        P1OUT &= ~BIT3;  // reconnect the supply

        P1OUT |= BIT5;  // debug
        ADC12CTL0 |= ADC12ENC | ADC12SC;    // Start sampling/conversion
        __bis_SR_register(LPM0_bits);
        adc_r2 = adc_reading;
        P1OUT &= ~BIT5;  // debug

        // calculate the resume threshold, represented as resistor tap setting
        // (may need an overflow check for the latter part)
        // atom_state[func_id].resume_thr =
            // ((int)adc_r1 - (int)adc_r2 < 512) ?
            // 20 :(uint8_t)((double)(adc_r1 - adc_r2) / 4095.0 * 32.0) + 17;
        if (adc_r1 > adc_r2) {
            atom_state[func_id].resume_thr =
                (uint8_t)((double)(adc_r1 - adc_r2) / 4095.0 * 32.0 + 15.0);
        } else {
            atom_state[func_id].resume_thr = (uint8_t) 17;
        }
        // atom_state[func_id].resume_thr = (adc_r1 > adc_r2) ?
        //     (uint8_t)((double)(adc_r1 - adc_r2) / 4095.0 * 32.0 + 14.0) :
        //     (uint8_t) 17;

        atom_state[func_id].calibrated = 1;
    }
    // else {
    //     if (!check_fail)
    // }

    atom_state[func_id].check_fail = 0;
#else
    atom_state[func_id].check_fail = 0;
#endif

    // CECTL2 = (CECTL2 & ~CEREF0) | CEREF0_17;
    CEINT |= CEIIE;
}
