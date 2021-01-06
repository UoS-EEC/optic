// Copyright (c) 2020, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <msp430fr5994.h>
#include "ic.h"

// #define DEBS

#define PERSISTENT __attribute__((section(".persistent")))

typedef struct AtomFuncState_s {
    uint8_t calibrated;  // 0: need to calibrate, 1: no need
    uint8_t check_fail;  // set at function entry, reset at exit; so '1' detected at the entry means failed before
    uint8_t resume_thr;  // resume threshold, represented as the resistor tap setting of the internal comparator
    // uint8_t backup_thr;  // backup threshold, represented as the resistor tap setting of the internal comparator
} AtomFuncState;

AtomFuncState atom_state[ATOM_FUNC_NUM] PERSISTENT;

uint16_t adc_r1;
uint16_t adc_r2;
uint16_t adc_reading;

extern uint8_t __datastart, __dataend, __romdatastart; // , __romdatacopysize;
extern uint8_t __bootstackend;
extern uint8_t __bssstart, __bssend;
extern uint8_t __ramtext_low, __ramtext_high, __ramtext_loadLow;
extern uint8_t __stack, __stackend;

static void gpio_init(void);
static void adc12_init(void);
static void comp_init(void);

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


void __attribute__((interrupt(RESET_VECTOR), naked, used, optimize("O0")))
iclib_boot() {
    WDTCTL = WDTPW | WDTHOLD;              // Stop watchdog timer
    __set_SP_register(&__bootstackend); // Boot stack
    __bic_SR_register(GIE);                // Disable interrupts during startup

    // Boot functions that are mapped to ram (most importantly fastmemcpy)
    uint8_t *dst = &__ramtext_low;
    uint8_t *src = &__ramtext_loadLow;
    size_t len = &__ramtext_high - &__ramtext_low;
    while (len--) {
        *dst++ = *src++;
    }

    // clock_init();
    gpio_init();
    adc12_init();
    comp_init();

    // needRestore = 1;                    // Indicate powerup
    P1OUT |= BIT4; // debug
    __bis_SR_register(LPM4_bits + GIE); // Enter LPM4 with interrupts enabled
    // Processor sleeps
    // ...
    // Processor wakes up after interrupt
    // *!Remaining code in this function is only executed during the first
    // boot!*

    // First time boot: Set SP, load data and initialize LRU

    __set_SP_register(&__stackend); // Runtime stack

    // load .data to RAM
    fastmemcpy(&__datastart, &__romdatastart, &__dataend - &__datastart);

    int main(); // Suppress implicit decl. warning
    main();
}

static void gpio_init(void) {
    // Need to initialize all ports/pins to reduce power consump'n
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

    /* Keep-alive signal to external comparator */
    // P6OUT = BIT0;    // resistor enable
    // P6REN = BIT0;    // Pull-up mode
    // P6DIR = ~(BIT0); // All but 0 to output
    // P6OUT = BIT0;
    // P6DIR = 0xff;

    /* Interrupts from S1 and S2 buttons */
    // P5DIR &= ~(BIT5 | BIT6);        // All but 5,6 to output direction
    // P5OUT |= BIT5 | BIT6;           // Pull-up resistor
    // P5REN |= BIT5 | BIT6;           // Pull-up mode
    // P5IES = BIT5 | BIT6;            // Falling edge

    /* Interrupts from S2 button */
    // P5DIR &= ~BIT5;          // bit 5 input direction
    // P5OUT |= BIT5;           // Pull-up resistor
    // P5REN |= BIT5;           // Pull-up resistor enable
    // P5IES = BIT5;            // Falling edge
    // P5IFG = 0;               // Clear pending interrupts
    // P5IE = BIT5;             // Enable restore irq

    // Disable GPIO power-on default high-impedance mode
    PM5CTL0 &= ~LOCKLPM5;
}

static void adc12_init(void) {
    // Configure ADC12
    // REFCTL0 |= REFVSEL_1;                   // 2.0 V reference selected, comment this to use 1.2V

    ADC12CTL0 = ADC12SHT0_2 |               // 16 cycles sample and hold time
                ADC12ON     ;               // ADC12 on
    ADC12CTL1 = ADC12PDIV_1 |               // Predive by 4
                ADC12SHP    ;               // SAMPCON is from the sampling timer
    // ADC12CTL2 = ADC12RES_2;                // Default: 12-bit conversion results, 14 cycles conversion time

    // Use battery monitor (1/2 AVcc)    
    // ADC12CTL3 = ADC12BATMAP;                // 1/2AVcc channel selected for ADC ch A31
    // ADC12MCTL0 = ADC12INCH_31 |
    //              ADC12VRSEL_1 ;             // VR+ = VREF buffered, VR- = Vss

    // Use P1.2
    P1SEL1 |= BIT2;                         // Configure P1.2 for ADC
    P1SEL0 |= BIT2;
    ADC12MCTL0 = ADC12INCH_2  |             // Select ch A2 at P1.2
                 ADC12VRSEL_1 ;             // VR+ = VREF buffered, VR- = Vss

    ADC12IER0 = ADC12IE0;                   // Enable ADC conv complete interrupt
}

// ADC12 interrupt service routine
void __attribute__ ((interrupt(ADC12_B_VECTOR))) ADC12_ISR (void) {
    switch(__even_in_range(ADC12IV, ADC12IV__ADC12RDYIFG))
    {
        case ADC12IV__NONE:        break;   // Vector  0:  No interrupt
        case ADC12IV__ADC12OVIFG:  break;   // Vector  2:  ADC12MEMx Overflow
        case ADC12IV__ADC12TOVIFG: break;   // Vector  4:  Conversion time overflow
        case ADC12IV__ADC12HIIFG:  break;   // Vector  6:  ADC12BHI
        case ADC12IV__ADC12LOIFG:  break;   // Vector  8:  ADC12BLO
        case ADC12IV__ADC12INIFG:  break;   // Vector 10:  ADC12BIN
        case ADC12IV__ADC12IFG0:            // Vector 12:  ADC12MEM0
            adc_reading = ADC12MEM0;
            __bic_SR_register_on_exit(LPM0_bits);
            break;   
        case ADC12IV__ADC12IFG1:   break;   // Vector 14:  ADC12MEM1
        case ADC12IV__ADC12IFG2:   break;   // Vector 16:  ADC12MEM2
        case ADC12IV__ADC12IFG3:   break;   // Vector 18:  ADC12MEM3
        case ADC12IV__ADC12IFG4:   break;   // Vector 20:  ADC12MEM4
        case ADC12IV__ADC12IFG5:   break;   // Vector 22:  ADC12MEM5
        case ADC12IV__ADC12IFG6:   break;   // Vector 24:  ADC12MEM6
        case ADC12IV__ADC12IFG7:   break;   // Vector 26:  ADC12MEM7
        case ADC12IV__ADC12IFG8:   break;   // Vector 28:  ADC12MEM8
        case ADC12IV__ADC12IFG9:   break;   // Vector 30:  ADC12MEM9
        case ADC12IV__ADC12IFG10:  break;   // Vector 32:  ADC12MEM10
        case ADC12IV__ADC12IFG11:  break;   // Vector 34:  ADC12MEM11
        case ADC12IV__ADC12IFG12:  break;   // Vector 36:  ADC12MEM12
        case ADC12IV__ADC12IFG13:  break;   // Vector 38:  ADC12MEM13
        case ADC12IV__ADC12IFG14:  break;   // Vector 40:  ADC12MEM14
        case ADC12IV__ADC12IFG15:  break;   // Vector 42:  ADC12MEM15
        case ADC12IV__ADC12IFG16:  break;   // Vector 44:  ADC12MEM16
        case ADC12IV__ADC12IFG17:  break;   // Vector 46:  ADC12MEM17
        case ADC12IV__ADC12IFG18:  break;   // Vector 48:  ADC12MEM18
        case ADC12IV__ADC12IFG19:  break;   // Vector 50:  ADC12MEM19
        case ADC12IV__ADC12IFG20:  break;   // Vector 52:  ADC12MEM20
        case ADC12IV__ADC12IFG21:  break;   // Vector 54:  ADC12MEM21
        case ADC12IV__ADC12IFG22:  break;   // Vector 56:  ADC12MEM22
        case ADC12IV__ADC12IFG23:  break;   // Vector 58:  ADC12MEM23
        case ADC12IV__ADC12IFG24:  break;   // Vector 60:  ADC12MEM24
        case ADC12IV__ADC12IFG25:  break;   // Vector 62:  ADC12MEM25
        case ADC12IV__ADC12IFG26:  break;   // Vector 64:  ADC12MEM26
        case ADC12IV__ADC12IFG27:  break;   // Vector 66:  ADC12MEM27
        case ADC12IV__ADC12IFG28:  break;   // Vector 68:  ADC12MEM28
        case ADC12IV__ADC12IFG29:  break;   // Vector 70:  ADC12MEM29
        case ADC12IV__ADC12IFG30:  break;   // Vector 72:  ADC12MEM30
        case ADC12IV__ADC12IFG31:  break;   // Vector 74:  ADC12MEM31
        case ADC12IV__ADC12RDYIFG: break;   // Vector 76:  ADC12RDY
        default: break;
    }
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
    CECTL1 = CEMRVS   |             // CMRVL selects the Vref - default VREF0
             CEPWRMD_2|             // 1 for Normal power mode / 2 for Ultra-low power mode
             CEF      |             // Output filter enabled            
             CEFDLY_3 ;             // Output filter delay 3600 ns
    // CEMRVL = 0 by default, select VREF0
                                            
    CECTL2 = CEREFL_1 |             // VREF 1.2 V is selected
             CERS_2   |             // VREF applied to R-ladder
             CERSEL   |             // to -terminal
            //  CEREF0_20|             // 2.625 V
            //  CEREF1_16;             // 2.125 V   
             CEREF0_17|             // 2.025 V
             CEREF1_16;             // 1.9125 V   

    P3SEL1 |= BIT0;                 // P3.0 C12 for Vcompare
    P3SEL0 |= BIT0;
    CECTL3 = CEPD12   ;             // Input Buffer Disable @P3.0/C12

    // P1SEL1 |= BIT2;                 // P1.2 C2 for Vcompare
    // P1SEL0 |= BIT2;
    // CECTL3 = CEPD2;                 // C2

    CEINT  = CEIE     ;             // Interrupt enabled
            //  CEIIE    ;             // Inverted interrupt enabled
    //          CERDYIE  ;             // Ready interrupt enabled

    CECTL1 |= CEON;                 // Turn On Comparator_E
}

// Comparator E interrupt service routine
void __attribute__ ((interrupt(COMP_E_VECTOR))) Comp_ISR (void) {
    switch(__even_in_range(CEIV, CEIV_CERDYIFG)) {
        case CEIV_NONE: break;
        case CEIV_CEIFG:
            CEINT = (CEINT & ~CEIFG & ~CEIE & ~CEIIFG) | CEIIE;
            CECTL1 |= CEMRVL;
            __bic_SR_register_on_exit(LPM4_bits);
            P1OUT &= ~BIT4; // debug
            break;
        case CEIV_CEIIFG:
            CEINT = (CEINT & ~CEIIFG & ~CEIIE & ~CEIFG) | CEIE;
            CECTL1 &= ~CEMRVL;
            __bis_SR_register_on_exit(LPM4_bits | GIE);
            P1OUT |= BIT4; // debug
            break;
        case CEIV_CERDYIFG:
            CEINT &= ~CERDYIFG;
            break;
        default: break;
    }
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
        CECTL2 = (CECTL2 & ~CEREF0) | (CEREF0 & ((uint16_t) atom_state[func_id].resume_thr));

        // CEINT |= CEIIE; // should have been set, just in case
        CECTL1 &= ~CEMRVL;
        __no_operation(); // sleep here if voltage is not high enough...

        // CECTL1 |= CEMRVL; // turn to VREF1 anyway, if voltage is already high enough
        CEINT &= ~CEIIE;
    } else { 
        // need calibration
        // wait for the highest voltage, assumed to be 3.5 V
        // CECTL2 = (CECTL2 & ~CEREF0) | CEREF0_27; // set VREF0 = 28/32 * Vref = 3.5V, Vref = 4V (2V Vref / 1/2 input divider)
        // CECTL2 = (CECTL2 & ~CEREF0) | CEREF0_25; // set VREF0 = 26/32 * Vref = 3.25 V
        // CECTL2 = (CECTL2 & ~CEREF0) | CEREF0_23; // set VREF0 = 24/32 * Vref = 3.0 V
        CECTL2 = (CECTL2 & ~CEREF0) | CEREF0_26; // set VREF0 = 27/32 * 3.6 = 3.0375 V

        // CEINT |= CEIIE; // should have been set, just in case
        CECTL1 &= ~CEMRVL;
        __no_operation(); // sleep here if voltage is not high enough...

        // CECTL1 |= CEMRVL; // turn to VREF1 anyway, if voltage is already high enough
        CEINT &= ~CEIIE;

        // calibrate
        // adc sampling takes 145 us
        P1OUT |= BIT5; // debug
        ADC12CTL0 |= ADC12ENC | ADC12SC; // start sampling & conversion
        __bis_SR_register(LPM0_bits | GIE);
        adc_r1 = adc_reading;
        P1OUT &= ~BIT5; // debug

        P1OUT |= BIT3; // short-circuit the supply
    }

    atom_state[func_id].check_fail = 1; // this is reset at the end of atomic function; if failed, this is still set on reboots
#else 
    if (atom_state[func_id].check_fail) {
        P1OUT |= BIT0;
        __delay_cycles(0xF);
        P1OUT &= ~BIT0;
    }    
    CECTL2 = (CECTL2 & ~CEREF0) | CEREF0_18;
    CECTL1 &= ~CEMRVL;
    __no_operation(); // sleep here if voltage is not high enough

    CEINT &= ~CEIIE;
    CECTL1 |= CEMRVL;

    atom_state[func_id].check_fail = 1;
#endif
    // disable interrupts
    // __bic_SR_register(GIE);
    // CEINT &= ~CEIIE;
    // CECTL1 |= CEMRVL; // turn to VREF1 anyway, in case 
    P1OUT |= BIT5; // debug, indicate function starts
}

void atom_func_end(uint8_t func_id) {
    P1OUT &= ~BIT5; // debug, indicate function ends
    // enable interrupts again
    // CEINT |= CEIIE;
    
#ifndef DEBS
    if (!atom_state[func_id].calibrated) { 
        // end of a calibration
        // measure end voltage
        P1OUT &= ~BIT3; // reconnect the supply

        P1OUT |= BIT5; // debug
        ADC12CTL0 |= ADC12ENC | ADC12SC;    // Start sampling/conversion
        __bis_SR_register(LPM0_bits);
        adc_r2 = adc_reading;
        P1OUT &= ~BIT5; // debug
    
        
        // calculate the resume threshold, represented as resistor tap setting
        // (may need an overflow check for the latter part)
        // atom_state[func_id].resume_thr = ((int)adc_r1 - (int)adc_r2 < 512) ? 20 : (uint8_t)((double)(adc_r1 - adc_r2) / 4095.0 * 32.0) + 17; 
        if (adc_r1 > adc_r2) {
            atom_state[func_id].resume_thr = (uint8_t)((double)(adc_r1 - adc_r2) / 4095.0 * 32.0 + 15.0);
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