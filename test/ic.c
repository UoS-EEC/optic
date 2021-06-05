// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include "test/ic.h"
#include "test/config.h"

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

// Snapshot of core processor registers
uint16_t PERSISTENT register_snapshot[15];
uint16_t PERSISTENT return_address;

// Snapshots of volatile memory
uint8_t PERSISTENT bss_snapshot[BSS_SIZE];
uint8_t PERSISTENT data_snapshot[DATA_SIZE];
uint8_t PERSISTENT stack_snapshot[STACK_SIZE];
// uint8_t PERSISTENT heap_snapshot[HEAP_SIZE];

uint8_t PERSISTENT snapshot_valid = 0;
uint8_t PERSISTENT suspending;  // from hibernate: 1, from restore: 0

volatile uint16_t adc_reading;  // Used by ADC ISR
uint16_t adc_r1;
uint16_t adc_r2;
int16_t d_v_charge;
int16_t d_v_discharge;
int16_t rtc_cnt1;
int16_t rtc_cnt2;
float v_exe;
uint8_t profiling;

volatile uint8_t PERSISTENT adapt_threshold = PROFILING_INIT_THRESHOLD;

uint16_t PERSISTENT adc_th[32] = {127,
 255,
 383,
 511,
 639,
 767,
 895,
1023,
1151,
1279,
1407,
1535,
1663,
1791,
1919,
2047,
2175,
2303,
2431,
2559,
2687,
2815,
2943,
3071,
3199,
3327,
3455,
3583,
3711,
3839,
3967,
4095};

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
    CSCTL1 |= DCOFSEL_6;  // DCO 8MHz

    // Set ACLK = LFXT(if enabled)/VLO; SMCLK = DCO; MCLK = DCO;
    CSCTL2 = SELA_0 + SELS_3 + SELM_3;

    // ACLK: Source/1; SMCLK: Source/1; MCLK: Source/1;
    CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;  // SMCLK = MCLK = 8 MHz

    // LFXT 32kHz crystal for RTC
    PJSEL0 = BIT4 | BIT5;                   // Initialize LFXT pins
    CSCTL4 &= ~LFXTOFF;                     // Enable LFXT
    do {
      CSCTL5 &= ~LFXTOFFG;                  // Clear LFXT fault flag
      SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG);              // Test oscillator fault flag

    CSCTL0_H = 0;  // Lock Register
}

// RTC 1s Timer setup
/*
static void rtc_init(void) {
    // Setup RTC Timer
    RTCCTL0_H = RTCKEY_H;                   // Unlock RTC
    RTCCTL0_L = RTCTEVIE_L;                 // RTC event interrupt enable

    // RTCCTL13 = RTCTEV_1 | RTCHOLD;          // Counter Mode, 32-kHz crystal, 16-bit overflow
    RTCCTL13 = RTCSSEL_2 | RTCTEV_0 | RTCHOLD;  // Counter Mode, Clocked from RTC1PS, 8-bit ovf
    RTCPS0CTL = RT0PSDIV1;                  // ACLK, /8
    RTCPS1CTL = RT1SSEL_2 | RT1PSDIV0 | RT1PSDIV1;  // Clocked from RT0PS, /16
    // RTCCTL13 &= ~(RTCHOLD);                 // Start RTC
}

void __attribute__((interrupt(RTC_C_VECTOR))) RTC_ISR(void) {
    switch (__even_in_range(RTCIV, RTCIV__RT1PSIFG)) {
        case RTCIV__RTCTEVIFG:              // RTCEVIFG
            // P1OUT ^= BIT0;                  // Toggle P1.0 LED
            __bic_SR_register_on_exit(LPM3_bits);
            break;
        default: break;
    }
}
*/


// RTC Counter mode setup, directly clocked from 32-kHz crystal
static void rtc_init(void) {
    // Setup RTC Timer
    RTCCTL0_H = RTCKEY_H;                   // Unlock RTC
    RTCCTL13 = RTCTEV_3 | RTCHOLD;  // Counter Mode, clocked from 32-kHz crystal
    // ..32-bit ovf (but not used)

    // RTCCTL13 &= ~(RTCHOLD);                 // Start RTC
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

    // Disable GPIO power-on default high-impedance mode
    PM5CTL0 &= ~LOCKLPM5;
}

static void adc12_init(void) {
    // Configure ADC12

    // 2.0 V reference selected, comment this to use 1.2V
    // REFCTL0 |= REFVSEL_1;

    ADC12CTL0 = ADC12SHT0_2 |  // 16 cycles sample and hold time
                ADC12ON;       // ADC12 on
    ADC12CTL1 = ADC12PDIV_0 |  // Predivide by 1, from ~4.8MHz MODOSC
                ADC12SHP;      // SAMPCON is from the sampling timer
    // ADC12CTL2 = 0;  // 8-bit conversion result resolution, 10 cycles
    // ADC12CTL2 = ADC12RES_2;  // Default 12-bit conversion results, 14cycles conversion time

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
            // Result is stored in ADC12MEM0
            adc_reading = ADC12MEM0;
            __bic_SR_register_on_exit(LPM0_bits);
            break;
        default: break;
    }
}

// Take 60~70us
uint16_t sample_vcc(void) {
    ADC12CTL0 |= ADC12ENC | ADC12SC;  // Start sampling & conversion
    __bis_SR_register(LPM0_bits | GIE);
    return adc_reading;
}

static void comp_init(void) {
    // P1DIR  |= BIT2;                 // P1.2 COUT output direction
    // P1SEL1 |= BIT2;                 // Select COUT function on P1.2/COUT

    // Setup Comparator_E

    // CECTL1 = CEPWRMD_1|             // Normal power mode
    //          CEF      |
    //          CEFDLY_3 ;
    CECTL1 = CEPWRMD_2|  // 1 for Normal power mode / 2 for Ultra-low power mode
             CEF      |  // Output filter enabled
             CEFDLY_3;   // Output filter delay 3600 ns

    CECTL2 = CEREFACC |  // Enable (low-power low-accuracy) clocked mode
                         // ..(can be overwritten by ADC static mode)
             CEREFL_1 |  // VREF 1.2 V is selected
             CERS_2   |  // VREF applied to R-ladder
             CERSEL   |  // to -terminal
             CEREF04 | CEREF02 | CEREF01 | CEREF00 |  // Hi V_th, 23(10111)
             CEREF14 | CEREF11 | CEREF10;  // Lo V_th, 19(10011)
            // CEREF_n : V threshold (Volt)
            //  0 : 0.1125
            //  1 : 0.2250
            //  2 : 0.3375
            //  3 : 0.4500
            //  4 : 0.5625
            //  5 : 0.6750
            //  6 : 0.7875
            //  7 : 0.9000
            //  8 : 1.0125
            //  9 : 1.1250
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

    // Channel selection
    P3SEL1 |= BIT0;  // P3.0/C12 for Vcompare
    P3SEL0 |= BIT0;
    CECTL0 = CEIPEN | CEIPSEL_12;   // Enable V+, input channel C12/P3.0
    CECTL3 |= CEPD12;  // Input Buffer Disable @P3.0/C12 (also set by the last line)

    CEINT  = CEIE;  // Interrupt enabled (Rising Edge)
            //  CEIIE;  // Inverted interrupt enabled (Falling Edge)
            //  CERDYIE;  // Ready interrupt enabled

    CECTL1 |= CEON;  // Turn On Comparator_E
}

// resistor_tap should be from 0 to 31
void set_CEREF0(uint8_t resistor_tap) {
    CECTL2 = (CECTL2 & ~CEREF0) |
        (CEREF0 & (uint16_t) resistor_tap);
}

// resistor_tap should be from 0 to 31
void set_CEREF1(uint8_t resistor_tap) {
    CECTL2 = (CECTL2 & ~CEREF1) |
        (CEREF1 & (uint16_t) resistor_tap);
}

// Comparator E interrupt service routine, Hibernus
void __attribute__((interrupt(COMP_E_VECTOR))) Comp_ISR(void) {
    switch (__even_in_range(CEIV, CEIV_CERDYIFG)) {
        case CEIV_NONE: break;
        case CEIV_CEIFG:
            // RTCCTL13 &= ~(RTCHOLD);  // Start RTC
            CECTL1 &= ~CEMRVS;  // Should be before changing CEINT, otherwise UNSTABLE!!!
            CEINT = (CEINT & ~CEIFG & ~CEIIFG & ~CEIE) | CEIIE;

            P1OUT &= ~BIT4;  // debug
            __bic_SR_register_on_exit(LPM3_bits);
            break;
        case CEIV_CEIIFG:
            // RTCCTL13 |= RTCHOLD;  // Hold RTC

            CEINT = (CEINT & ~CEIIFG & ~CEIFG & ~CEIIE) | CEIE;

            // hibernate();
            // if (suspending) {
            P1OUT |= BIT4;  // Debug
            __bis_SR_register_on_exit(LPM3_bits | GIE);
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

    // Minimized initialization stack for wakeup
    // to avoid being stuck in boot & fail)
    gpio_init();
    adc12_init();
    comp_init();

    P1OUT |= BIT4;  // Debug
    __bis_SR_register(LPM3_bits | GIE);  // Enter LPM4 with interrupts enabled
    // Processor sleeps
    // ...
    // Processor wakes up after interrupt (Hi V threshold hit)
    // P1OUT &= ~BIT4;  // Debug

    clock_init();
    rtc_init();

    // __bic_SR_register(GIE);              // Disable interrupts during startup
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
    // __bis_SR_register(GIE);

    int main();  // Suppress implicit decl. warning
    main();
}

void profiling_start(uint8_t func_id) {
    // Take a Vcc reading
    adc_r1 = sample_vcc();

    // Sleep here if (Vcc < adapt_threshold) until adapt_threshold is hit...
    // Continue directly if (Vcc > adapt_threshold)...
    if (adc_r1 > adc_th[adapt_threshold]) {
        // Vcc > Vth, Continue
        profiling = 0;
    } else {
        // Vcc < Vth, Sleep and store energy
        profiling = 1;
        __bic_SR_register(GIE);  // Disable global interrupt

        // *** Charging cycle starts ***
        // Clear and start RTC
        RTCCNT12 = 0;
        RTCCTL13 &= ~(RTCHOLD);  // Start RTC

        // Sleep and wait for energy recharged

        set_CEREF0(adapt_threshold);  // Resume threshold
        CEINT = (CEINT & ~CEIIFG & ~CEIFG & ~CEIIE) | CEIE;
        CECTL1 |= CEMRVS;
        P1OUT |= BIT4;  // Debug
        __bis_SR_register(LPM3_bits | GIE);
        // Sleep until Vth is reached...

        // *** Charging cycle ends, discharging cycle starts ***
        // Take a Vcc reading, get Delta V_charge
        adc_r2 = sample_vcc();
        d_v_charge = (int16_t) adc_r2 - (int16_t) adc_r1;
        // Take a time reading, get T_charge
        rtc_cnt1 = RTCCNT12;  // T_charge
    }

    __bic_SR_register(GIE);  // Disable global interrupt
    // Run the atomic function...
}

void profiling_end(uint8_t func_id) {
    // *** Discharging cycle ends ***
    __bis_SR_register(GIE);  // Enable global interrupt

    if (profiling) {
        // Take a Vcc reading, get Delta V_exe
        adc_r1 = sample_vcc();
        d_v_discharge = (int16_t) adc_r1 - (int16_t) adc_r2;

        // Take a time reading, get T_exe
        rtc_cnt2 = RTCCNT12 - rtc_cnt1;  // T_exe

        // Stop RTC
        RTCCTL13 |= RTCHOLD;

        // Adapt the next threshold
        v_exe = ((float) rtc_cnt2 / rtc_cnt1 * d_v_charge - d_v_discharge);
        adapt_threshold = (uint8_t) (v_exe / MAX_ADC_READING * MAX_COMPE_RTAP) + TARGET_END_THRESHOLD;
        if (adapt_threshold > 31)
            adapt_threshold = 31;
    }
}
