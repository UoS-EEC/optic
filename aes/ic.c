// Copyright (c) 2020, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include "ic.h"

// #define DEBS

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

AtomFuncState atom_state[ATOM_FUNC_NUM] PERSISTENT;

uint16_t adc_r1;
uint16_t adc_r2;
uint16_t adc_reading;

extern uint8_t __datastart, __dataend, __romdatastart;  // , __romdatacopysize;
extern uint8_t __bootstackend;
extern uint8_t __bssstart, __bssend;
extern uint8_t __ramtext_low, __ramtext_high, __ramtext_loadLow;
extern uint8_t __stack, __stackend;

void __attribute__((section(".ramtext"), naked))
fastmemcpy(uint8_t *dst, uint8_t *src, size_t len) {
  __asm__(" push r5\n"
          " tst r14\n" // Test for len=0
          " jz return\n"
          " mov #2, r5\n"   // r5 = word size
          " xor r15, r15\n" // Clear r15
          " mov r14, r15\n" // r15=len
          " and #1, r15\n"  // r15 = len%2
          " sub r15, r14\n" // r14 = len - len%2
          "loopmemcpy:  \n"
          " mov.w @r13+, @r12\n"
          " add r5, r12 \n"
          " sub r5, r14 \n"
          " jnz loopmemcpy \n"
          " tst r15\n"
          " jz return\n"
          " mov.b @r13, @r12\n" // move last byte
          "return:\n"
          " pop r5\n"
          " ret\n");
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

static void comp_init(void) {
    // P1DIR  |= BIT2;                 // P1.2 COUT output direction
    // P1SEL1 |= BIT2;                 // Select COUT function on P1.2/COUT

    // Setup Comparator_E
    CECTL0 = CEIPEN | CEIPSEL_12;   // Enable V+, input channel C12
    // CECTL0 = CEIPEN | CEIPSEL_2;    // Enable V+, input channel C2

    // CECTL1 = CEPWRMD_1|             // Normal power mode
    //          CEF      |
    //          CEFDLY_3 ;
    CECTL1 = CEMRVS   |  // CMRVL selects the Vref - default VREF0
             CEPWRMD_2|  // 1 for Normal power mode / 2 for Ultra-low power mode
             CEF      |  // Output filter enabled
             CEFDLY_3;   // Output filter delay 3600 ns
    // CEMRVL = 0 by default, select VREF0

    CECTL2 = CEREFL_1 |  // VREF 1.2 V is selected
             CERS_2   |  // VREF applied to R-ladder
             CERSEL   |  // to -terminal
            //  CEREF0_20|  // 2.625 V
            //  CEREF1_16;  // 2.125 V
             CEREF0_17|  // 2.025 V
             CEREF1_16;  // 1.9125 V

    P3SEL1 |= BIT0;  // P3.0 C12 for Vcompare
    P3SEL0 |= BIT0;
    CECTL3 = CEPD12;  // Input Buffer Disable @P3.0/C12

    // P1SEL1 |= BIT2;  // P1.2 C2 for Vcompare
    // P1SEL0 |= BIT2;
    // CECTL3 = CEPD2;  // C2

    CEINT  = CEIE;  // Interrupt enabled
            //  CEIIE;  // Inverted interrupt enabled
            //  CERDYIE;  // Ready interrupt enabled

    CECTL1 |= CEON;  // Turn On Comparator_E
}

void __attribute__((interrupt(RESET_VECTOR), naked, used, optimize("O0")))
iclib_boot() {
    WDTCTL = WDTPW | WDTHOLD;            // Stop watchdog timer
    __set_SP_register(&__bootstackend);  // Boot stack
    __bic_SR_register(GIE);              // Disable interrupts during startup

    // Boot functions that are mapped to ram (most importantly fastmemcpy)
    uint8_t *dst = &__ramtext_low;
    uint8_t *src = &__ramtext_loadLow;
    size_t len = &__ramtext_high - &__ramtext_low;
    while (len--) {
        *dst++ = *src++;
    }

    clock_init();
    gpio_init();
    adc12_init();
    comp_init();

    // needRestore = 1;                    // Indicate powerup
    P1OUT |= BIT4;  // debug
    __bis_SR_register(LPM4_bits + GIE);  // Enter LPM4 with interrupts enabled
    // Processor sleeps
    // ...
    // Processor wakes up after interrupt
    // *!Remaining code in this function is only executed during the first
    // boot!*

    // First time boot: Set SP, load data and initialize LRU

    __set_SP_register(&__stackend);  // Runtime stack

    // load .data to RAM
    fastmemcpy(&__datastart, &__romdatastart, &__dataend - &__datastart);

    int main();  // Suppress implicit decl. warning
    main();
}

void atom_func_start(uint8_t func_id) {
#ifndef DEBS
    if (atom_state[func_id].check_fail) {
        atom_state[func_id].calibrated = 0;
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
        // not calibrated, need calibration
        // wait for the highest voltage, assumed to be 3.5 V

        // set VREF0 = 28/32 * Vref = 3.5V, Vref = 4V (2V Vref / 1/2 divider)
        // CECTL2 = (CECTL2 & ~CEREF0) | CEREF0_27;

        // set VREF0 = 26/32 * Vref = 3.25 V
        // CECTL2 = (CECTL2 & ~CEREF0) | CEREF0_25;

        // set VREF0 = 24/32 * Vref = 3.0 V
        // CECTL2 = (CECTL2 & ~CEREF0) | CEREF0_23;

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
#else
    if (atom_state[func_id].check_fail) {
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

void hibernate(void) {
    // !!! *** DONT TOUCH THE STACK HERE *** !!!

    // copy Core registers to FRAM
    // R0 : PC, save through return_address, not here
    asm(" MOVA R1,&core_registers");      // R1: Stack Pointer
    asm(" MOVA R2,&core_registers+4");    // R2: Status Register
    asm(" MOVA R3,&core_registers+8");
    asm(" MOVA R4,&core_registers+12");
    asm(" MOVA R5,&core_registers+16");
    asm(" MOVA R6,&core_registers+20");
    asm(" MOVA R7,&core_registers+24");
    asm(" MOVA R8,&core_registers+28");
    asm(" MOVA R9,&core_registers+32");
    asm(" MOVA R10,&core_registers+36");
    asm(" MOVA R11,&core_registers+40");
    asm(" MOVA R12,&core_registers+44");
    asm(" MOVA R13,&core_registers+48");
    asm(" MOVA R14,&core_registers+52");
    asm(" MOVA R15,&core_registers+56");

    // Return to the next line of Hibernate();
    return_address = *(uint16_t *)(__get_SP_register());

    // Save a snapshot of volatile memory
    // allocate to .npbss to prevent being overwritten when copying .bss
    static volatile uint8_t * src __attribute__((section(".npbss")));
    static volatile uint8_t * dst __attribute__((section(".npbss")));
    static volatile size_t len __attribute__((section(".npbss")));

    // bss
    src = &__bss_low;
    dst = (uint8_t *)bss_snapshot;
    len = &__bss_high - src;
    memcpy((void *)dst, (void *)src, len);

    // data
    src = &__data_low;
    dst = (uint8_t *)data_snapshot;
    len = &__data_high - src;
    memcpy((void *)dst, (void *)src, len);

    // stack
    // stack_low-----[SP-------stack_high]
#ifdef TRACK_STACK
    src = (uint8_t *)core_registers[0];  // Saved SP
#else
    src = &__stack_low;
#endif
    len = &__STACK_END - src;
    uint16_t offset = (uint16_t)(src - &__stack_low)/2;  // word offset
    dst = (uint8_t *)&stack_snapshot[offset];
    memcpy((void *)dst, (void *)src, len);

    snapshot_valid = 1;
    from_hiber_or_restore = 1;
}

void restore(void) {
    // allocate to .npbss to prevent being overwritten when copying .bss
    static volatile uint32_t src __attribute__((section(".npbss")));
    static volatile uint32_t dst __attribute__((section(".npbss")));
    static volatile size_t len __attribute__((section(".npbss")));

//    snapshot_valid = 0;
    /* comment "snapshot_valid = 0"
       for being able to restore from an old snapshot
       if Hibernate() fails to complete
       Here, introducing code re-execution? */

    from_hiber_or_restore = 0;

    // data
    dst = (uint32_t) &__data_low;
    src = (uint32_t)data_snapshot;
    len = (uint32_t)&__data_high - dst;
    memcpy((void *)dst, (void *)src, len);

    // bss
    dst = (uint32_t) &__bss_low;
    src = (uint32_t)bss_snapshot;
    len = (uint32_t)&__bss_high - dst;
    memcpy((void *)dst, (void *)src, len);

    // stack
#ifdef TRACK_STACK
        dst = core_registers[0];  // Saved stack pointer
#else
        dst = (uint32_t)&__stack_low;  // Save full stack
#endif
    len = (uint8_t *)&__STACK_END - (uint8_t *)dst;
    uint16_t offset = (uint16_t)((uint16_t *)dst - (uint16_t *)&__stack_low);  // word offset
    src = (uint32_t)&stack_snapshot[offset];

    /* Move to separate stack space before restoring original stack. */
    __set_SP_register((unsigned short)&__cp_stack_high);  // Move to separate stack
    memcpy((uint8_t *)dst, (uint8_t *)src, len);  // Restore default stack
    // Can't use stack here!

    // Restore processor registers
    asm(" MOVA &core_registers,R1");
    __no_operation();
    asm(" MOVA &core_registers+4,R2");
    __no_operation();
    asm(" MOVA &core_registers+8,R3");
    asm(" MOVA &core_registers+12,R4");
    asm(" MOVA &core_registers+16,R5");
    asm(" MOVA &core_registers+20,R6");
    asm(" MOVA &core_registers+24,R7");
    asm(" MOVA &core_registers+28,R8");
    asm(" MOVA &core_registers+32,R9");
    asm(" MOVA &core_registers+36,R10");
    asm(" MOVA &core_registers+40,R11");
    asm(" MOVA &core_registers+44,R12");
    asm(" MOVA &core_registers+48,R13");
    asm(" MOVA &core_registers+52,R14");
    asm(" MOVA &core_registers+56,R15");

    // Restore return address
    *(uint16_t *)(__get_SP_register()) = return_address;
}

// Port 5 interrupt service routine
// void __attribute__ ((__interrupt__(PORT5_VECTOR))) Port5_ISR(void) {
//     if (P5IFG & BIT5) {
//         // if (__get_SR_register() & LPM4_bits) {
//         //     __bic_SR_register_on_exit(LPM4_bits);
//         // } else {
//         //     __bis_SR_register_on_exit(LPM4_bits | GIE);
//         // }
//         for (uint8_t i = 0; i < ATOM_FUNC_NUM; i++)
//             atom_state[i].calibrated = 0;
//         P5IFG &= ~BIT5; // Clear interrupt flags
//     }
// }

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

// Comparator E interrupt service routine
void __attribute__((interrupt(COMP_E_VECTOR))) Comp_ISR(void) {
    switch (__even_in_range(CEIV, CEIV_CERDYIFG)) {
        case CEIV_NONE: break;
        case CEIV_CEIFG:
            CEINT = (CEINT & ~CEIFG & ~CEIE & ~CEIIFG) | CEIIE;
            CECTL1 |= CEMRVL;
            __bic_SR_register_on_exit(LPM4_bits);
            P1OUT &= ~BIT4;  // debug
            break;
        case CEIV_CEIIFG:
            CEINT = (CEINT & ~CEIIFG & ~CEIIE & ~CEIFG) | CEIE;
            CECTL1 &= ~CEMRVL;
            __bis_SR_register_on_exit(LPM4_bits | GIE);
            P1OUT |= BIT4;  // debug
            break;
        case CEIV_CERDYIFG:
            CEINT &= ~CERDYIFG;
            break;
        default: break;
    }
}
