// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include "opta/ic.h"
#include "opta/config.h"


#define COMPARATOR_DELAY    __delay_cycles(80)

extern uint8_t __datastart, __dataend, __romdatastart;  // , __romdatacopysize;
extern uint8_t __bootstackend;
extern uint8_t __bssstart, __bssend;
extern uint8_t __ramtext_low, __ramtext_high, __ramtext_loadLow;
extern uint8_t __stack, __stackend;

#define PERSISTENT __attribute__((section(".persistent")))

typedef struct AtomFuncState_s {
    // calibrated:
    // 0: need to calibrate, 1: no need
    // uint8_t calibrated;

    // check_fail:
    // Set at function entry, reset at exit,
    // ..so read as '1' at the entry means the function failed before
    uint8_t check_fail;

    // Adaptive threhold, and profiling variables
    uint8_t  adapt_threshold;   // Init: PROFILING_INIT_THRESHOLD
    uint16_t v_exe_history[V_EXE_HISTORY_SIZE];
    uint8_t  v_exe_hist_index;  // Init: 0
    uint16_t v_exe_mean;        // Init: 0
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
uint8_t PERSISTENT suspending;  // from backup: 1, from restore: 0


// Profiling parameters
volatile uint16_t adc_reading;  // Used by ADC ISR
uint16_t adc_r1;
uint16_t adc_r2;
#ifdef FLOAT_POINT_ARITHMETIC
int16_t d_v_charge;
int16_t d_v_discharge;
int16_t timer_cnt1;
int16_t timer_cnt2;
float v_exe;
#else
uint32_t d_v_charge;
uint32_t d_v_discharge;
uint32_t timer_cnt1;
uint32_t timer_cnt2;
uint16_t v_exe;
#endif

// Comparator E control signal
uint8_t storing_energy;

// uint8_t PERSISTENT adapt_threshold = PROFILING_INIT_THRESHOLD;
// uint16_t PERSISTENT v_exe_history[V_EXE_HISTORY_SIZE];
// uint16_t PERSISTENT v_exe_mean = 0;
// uint8_t PERSISTENT i = 0;

// Function declarations
// static void backup(void);
// static void restore(void);

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

// static void backup(void) {
//     // !!! *** DONT TOUCH THE STACK HERE *** !!!

//     // copy Core registers to FRAM
//     // R0 : PC, save through return_address, not here
//     __asm__("mov   sp, &register_snapshot\n"
//             "mov   sr, &register_snapshot+2\n"
//             "mov   r3, &register_snapshot+4\n"
//             "mov   r4, &register_snapshot+6\n"
//             "mov   r5, &register_snapshot+8\n"
//             "mov   r6, &register_snapshot+10\n"
//             "mov   r7, &register_snapshot+12\n"
//             "mov   r8, &register_snapshot+14\n"
//             "mov   r9, &register_snapshot+16\n"
//             "mov   r10, &register_snapshot+18\n"
//             "mov   r11, &register_snapshot+20\n"
//             "mov   r12, &register_snapshot+22\n"
//             "mov   r13, &register_snapshot+24\n"
//             "mov   r14, &register_snapshot+26\n"
//             "mov   r15, &register_snapshot+28\n");


//     // Return to the next line of backup();
//     return_address = *(uint16_t *)(__get_SP_register());

//     // Save a snapshot of volatile memory
//     // allocate to .boot_stack to prevent being overwritten when copying .bss
//     static uint8_t *src __attribute__((section(".boot_stack")));
//     static uint8_t *dst __attribute__((section(".boot_stack")));
//     static size_t len __attribute__((section(".boot_stack")));

//     // bss
//     src = &__bssstart;
//     dst = bss_snapshot;
//     len = &__bssend - &__bssstart;
//     fastmemcpy(dst, src, len);

//     // data
//     src = &__datastart;
//     dst = data_snapshot;
//     len = &__dataend - &__datastart;
//     fastmemcpy(dst, src, len);

//     // stack
//     // stack_low-----[SP-------stack_high]
// #ifdef TRACK_STACK
//     src = (uint8_t *)register_snapshot[0];  // Saved SP
// #else
//     src = &__stack;
// #endif
//     len = &__stackend - src;
//     uint16_t offset = (uint16_t)(src - &__stack);
//     dst = (uint8_t *)&stack_snapshot[offset];
//     fastmemcpy(dst, src, len);

//     snapshot_valid = 1;  // For now, don't check snapshot valid
//     suspending = 1;
// }

// static void restore(void) {
//     // allocate to .boot_stack to prevent being overwritten when copying .bss
//     static uint8_t *src __attribute__((section(".boot_stack")));
//     static uint8_t *dst __attribute__((section(".boot_stack")));
//     static size_t len __attribute__((section(".boot_stack")));

//     snapshot_valid = 0;
//     /* comment "snapshot_valid = 0"
//        for being able to restore from an old snapshot
//        if backup() fails to complete
//        Here, introducing code re-execution? */

//     suspending = 0;

//     // data
//     dst = &__datastart;
//     src = data_snapshot;
//     len = &__dataend - &__datastart;
//     fastmemcpy(dst, src, len);

//     // bss
//     dst = &__bssstart;
//     src = bss_snapshot;
//     len = &__bssend - &__bssstart;
//     fastmemcpy(dst, src, len);

//     // stack
// #ifdef TRACK_STACK
//     dst = (uint8_t *)register_snapshot[0];  // Saved stack pointer
// #else
//     dst = &__stack;  // Save full stack
// #endif
//     len = &__stackend - dst;
//     uint16_t offset = (uint16_t)(dst - &__stack);  // word offset
//     src = &stack_snapshot[offset];

//     /* Move to separate stack space before restoring original stack. */
//     // __set_SP_register((unsigned short)&__cp_stack_high);  // Move to separate stack
//     fastmemcpy(dst, src, len);  // Restore default stack
//     // Can't use stack here!

//     // Restore processor registers
//     __asm__("mov   &register_snapshot, sp\n"
//             "nop\n"
//             "mov   &register_snapshot+2, sr\n"
//             "nop\n"
//             "mov   &register_snapshot+4, r3\n"
//             "mov   &register_snapshot+6, r4\n"
//             "mov   &register_snapshot+8, r5\n"
//             "mov   &register_snapshot+10, r6\n"
//             "mov   &register_snapshot+12, r7\n"
//             "mov   &register_snapshot+14, r8\n"
//             "mov   &register_snapshot+16, r9\n"
//             "mov   &register_snapshot+18, r10\n"
//             "mov   &register_snapshot+20, r11\n"
//             "mov   &register_snapshot+22, r12\n"
//             "mov   &register_snapshot+24, r13\n"
//             "mov   &register_snapshot+26, r14\n"
//             "mov   &register_snapshot+28, r15\n");

//     // Restore return address
//     *(uint16_t *)(__get_SP_register()) = return_address;
// }

static void clock_init(void) {
    CSCTL0_H = CSKEY_H;  // Unlock register

    CSCTL1 = DCOFSEL_6;  // DCO = 8 MHz

    // Set ACLK = VLO (~10kHz); SMCLK = DCO; MCLK = DCO;
    CSCTL2 = SELA_1 + SELS_3 + SELM_3;

    // ACLK: Source/1; SMCLK: Source/1; MCLK: Source/1;
    CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;      // SMCLK = MCLK = 8 MHz

    // LFXT 32kHz crystal for RTC, not used (due to start-up time & current issue)
    // ..~240ms start-up settling time, and ~100uA current draw
    // PJSEL0 = BIT4 | BIT5;                   // Initialize LFXT pins
    // CSCTL4 &= ~LFXTOFF;                     // Enable LFXT
    // do {
    //   CSCTL5 &= ~LFXTOFFG;                  // Clear LFXT fault flag
    //   SFRIFG1 &= ~OFIFG;
    // } while (SFRIFG1 & OFIFG);              // Test oscillator fault flag

    CSCTL0_H = 0;  // Lock Register
}

// RTC Counter mode setup, directly clocked from 32-kHz crystal
// static void rtc_init(void) {
//     // Setup RTC Timer
//     RTCCTL0_H = RTCKEY_H;                   // Unlock RTC
//     RTCCTL13 = RTCTEV_3 | RTCHOLD;  // Counter Mode, clocked from 32-kHz crystal
//     // ..32-bit ovf (but not used)

//     // RTCCTL13 &= ~(RTCHOLD);                 // Start RTC
// }

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

    ADC12CTL0 = ADC12SHT0_2 |   // 16 cycles sample and hold time
                ADC12ON;        // ADC12 on
    ADC12CTL1 = ADC12PDIV_1 |   // Predivide by 4, from ~4.8MHz MODOSC
                ADC12SHP;       // SAMPCON is from the sampling timer
    ADC12CTL2 = ADC12RES_2 |    // Default 12-bit conversion results, 14cycles conversion time
                ADC12PWRMD_1;   // Low-power mode

    // Use battery monitor (1/2 AVcc)
    // ADC12CTL3 = ADC12BATMAP;  // 1/2AVcc channel selected for ADC ch A31
    // ADC12MCTL0 = ADC12INCH_31 |
    //              ADC12VRSEL_1;  // VR+ = VREF buffered, VR- = Vss

    // Use P3.0
    // P3SEL1 |= BIT0;
    // P3SEL0 |= BIT0;
    // ADC12MCTL0 = ADC12INCH_12|      // Select ch A12 at P3.0
    //              ADC12VRSEL_1;      // VR+ = VREF buffered, VR- = Vss
    // Use P3.1
    P3SEL1 |= BIT1;
    P3SEL0 |= BIT1;
    ADC12MCTL0 = ADC12INCH_13|      // Select ch A13 at P3.1
                 ADC12VRSEL_1;      // VR+ = VREF buffered, VR- = Vss
    while (!(REFCTL0 & REFGENRDY)) {}   // Wait for reference generator to settle
    // ADC12IER0 = ADC12IE0;  // Enable ADC conv complete interrupt
}

// ADC12 interrupt service routine
// void __attribute__((interrupt(ADC12_B_VECTOR))) ADC12_ISR(void) {
//     switch (__even_in_range(ADC12IV, ADC12IV__ADC12RDYIFG)) {
//         case ADC12IV__ADC12IFG0:            // Vector 12:  ADC12MEM0
//             // Result is stored in ADC12MEM0
//             adc_reading = ADC12MEM0;
//             __bic_SR_register_on_exit(LPM0_bits);
//             break;
//         default: break;
//     }
// }

// Take ~60us
static uint16_t sample_vcc(void) {
    P8OUT |= BIT0;
    ADC12CTL0 |= ADC12ENC | ADC12SC;  // Start sampling & conversion
    // __bis_SR_register(LPM0_bits | GIE);
    while (!(ADC12IFGR0 & BIT0)) {}
    adc_reading = ADC12MEM0;
    P8OUT &= ~BIT0;
    return adc_reading;
}

static void comp_init(void) {
    // Enable CEOUT for stability concerns
    P1DIR  |= BIT2;                 // P1.2 COUT output direction
    P1SEL1 |= BIT2;                 // Select COUT function on P1.2/COUT

    // Setup Comparator_E
    CECTL1 = CEPWRMD_2|  // 0 for High-speed mode
                         // 1 for Normal power mode
                         // 2 for Ultra-low power mode
             CEF      |  // Output filter enabled
             CEFDLY_3;   // Output filter delay 3600 ns

    // Using CEREFACC here can cause inaccurate comparator function
    CECTL2 = CEREFL_1 |  // VREF 1.2 V is selected
             CERS_2   |  // VREF applied to R-ladder
             CERSEL   |  // to -terminal
             COMPE_DEFAULT_HI_THRESHOLD|    // Hi V_th
             COMPE_DEFAULT_LO_THRESHOLD;    // Lo V_th
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

    // Set up internal Vref (No use sometimes)
    while (REFCTL0 & REFGENBUSY) {}
    REFCTL0 |= REFVSEL_0 | REFON;       // Select internal Vref (VR+) = 1.2V  Internal Reference ON
    while (!(REFCTL0 & REFBGRDY)) {}    // Wait for reference generator to settle

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

// Set CEREF0, resistor_tap should be from 0 to 31
void set_CEREF0(uint8_t resistor_tap) {
    CECTL2 = (CECTL2 & ~CEREF0) |
        (CEREF0 & (uint16_t) resistor_tap);
}

// Set CEREF1, resistor_tap should be from 0 to 31
void set_CEREF1(uint8_t resistor_tap) {
    CECTL2 = (CECTL2 & ~CEREF1) |
        (CEREF1 & ((uint16_t) resistor_tap << 8));
}

// Comparator E interrupt service routine, Hibernus
void __attribute__((interrupt(COMP_E_VECTOR))) Comp_ISR(void) {
    switch (__even_in_range(CEIV, CEIV_CERDYIFG)) {
        case CEIV_NONE: break;
        case CEIV_CEIFG:
            CEINT = (CEINT & (~CEIFG) & (~CEIIFG) & (~CEIE)) | CEIIE;
            P1OUT &= ~BIT4;  // Debug
            __bic_SR_register_on_exit(LPM3_bits);
            break;
        case CEIV_CEIIFG:
            if (storing_energy) {
                // set_CEREF0(adapt_threshold);
                set_CEREF0(CECTL2_H & CEREF1_H);
#ifdef DEBUG_UART
                set_CEREF1(TARGET_END_THRESHOLD - 1);
#else
                set_CEREF1(TARGET_END_THRESHOLD);
#endif
            }
            CEINT = (CEINT & (~CEIIFG) & (~CEIFG) & (~CEIIE)) | CEIE;

            // backup();
            // if (suspending) {
                P1OUT |= BIT4;  // Debug
                __bis_SR_register_on_exit(LPM3_bits | GIE);
            // } else {
            //     P1OUT &= ~BIT5;  // Debug, come back from restore()
            //     __bic_SR_register_on_exit(LPM3_bits);
            // }
            break;
        // case CEIV_CERDYIFG:
        //     CEINT &= ~CERDYIFG;
        //     break;
        default: break;
    }
}

#ifdef DEBUG_UART
void uart_init(void) {
    // P2.0 UCA0TXD
    // P2.1 UCA0RXD
    P2SEL0 &= ~(BIT0 | BIT1);
    P2SEL1 |=   BIT0 | BIT1;

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
char* uitoa_10(unsigned num, char* const str) {
    // calculate decimal
    char* ptr = str;
    unsigned modulo;
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

// Boot function
void __attribute__((interrupt(RESET_VECTOR), naked, used, optimize("O0")))
iclib_boot() {
    WDTCTL = WDTPW | WDTHOLD;            // Stop watchdog timer
    __set_SP_register(&__bootstackend);  // Boot stack
    __bic_SR_register(GIE);              // Disable interrupts during startup

    // Minimized initialization stack for wakeup
    // ..to avoid being stuck in boot & fail
    gpio_init();
    comp_init();

    storing_energy = 0;

    P1OUT |= BIT4;  // Debug
    __bis_SR_register(LPM4_bits | GIE);  // Enter LPM4 with interrupts enabled
    __bic_SR_register(OSCOFF);
    // Processor sleeps
    // ...
    // Processor wakes up after interrupt (Hi V threshold hit)

    // Remaining initialization stack for normal execution
    clock_init();
    adc12_init();
#ifdef DEBUG_UART
    uart_init();
#endif

    // Boot functions that are mapped to ram (most importantly fastmemcpy)
    uint8_t *dst = &__ramtext_low;
    uint8_t *src = &__ramtext_loadLow;
    size_t len = &__ramtext_high - &__ramtext_low;
    while (len--) {
        *dst++ = *src++;
    }

    if (snapshot_valid) {
        // P1OUT |= BIT5;  // Debug, restore() starts
        // restore();
        // Does not return here!!!
        // Return to the next line of backup()...
    } else {
        // snapshot_valid = 0 when
        // ..previous snapshot failed or 1st boot (both rare)
        // Normal boot...

        // Init atom_state
        for (uint8_t i = 0; i < ATOM_FUNC_NUM; i++) {
            atom_state[i].adapt_threshold = PROFILING_INIT_THRESHOLD;
            atom_state[i].v_exe_hist_index = 0;
            atom_state[i].v_exe_mean = 0;
        }
        snapshot_valid = 1;  // Temporary line, make sure atom_state init only once
    }

    // Load .data to RAM
    fastmemcpy(&__datastart, &__romdatastart, &__dataend - &__datastart);

    __set_SP_register(&__stackend);  // Runtime stack

    int main();  // Suppress implicit decl. warning
    main();
}

#ifndef DISCONNECT_SUPPLY_PROFILING
// Connect supply profiling

void atom_func_start(uint8_t func_id) {
    // If the task failed before, reset the profiling
    if (atom_state[func_id].check_fail) {
        // atom_state[func_id].adapt_threshold = PROFILING_INIT_THRESHOLD;
        atom_state[func_id].adapt_threshold++;   // Increment threshold
        atom_state[func_id].v_exe_hist_index = 0;
    }

    // *** Charging cycle starts ***
    CEINT &= ~CEIIE;
    P7OUT |= BIT0;      // Indicate overhead

    // Take a Vcc reading
    adc_r1 = sample_vcc();
    // Clear and start Timer A0
    TA0CTL = TASSEL__ACLK | MC__CONTINUOUS | TACLR;

    P7OUT &= ~BIT0;     // Indicate overhead
    CEINT |= CEIIE;

    // Sleep here if (Vcc < adapt_threshold) until adapt_threshold is hit
    // ..or continue directly if (Vcc > adapt_threshold)
    // Sleep and wait for energy recharged
    storing_energy = 1;
    set_CEREF1(atom_state[func_id].adapt_threshold);    // Adaptive threshold
    // set_CEREF1(FIXED_PROFILING_THRESHOLD);              // Fixed profiling threshold
    COMPARATOR_DELAY;   // May sleep here until Hi Vth is reached
#ifdef DEBUG_UART
    set_CEREF1(TARGET_END_THRESHOLD - 1);
#else
    set_CEREF1(TARGET_END_THRESHOLD);
#endif
    storing_energy = 0;

    // *** Charging cycle ends, discharging cycle starts ***
    CEINT &= ~CEIIE;
    P7OUT |= BIT0;      // Indicate overhead
    // Take a time reading, get T_charge
    timer_cnt1 = TA0R;  // T_charge

    if (timer_cnt1 > MIN_PROFILING_TIMER_CNT) {  // Don't profile if charging time is too short
        // Take a Vcc reading, get Delta V_charge
        adc_r2 = sample_vcc();
        d_v_charge = (int16_t) adc_r2 - (int16_t) adc_r1;
    }
    atom_state[func_id].check_fail = 1;
    P7OUT &= ~BIT0;     // Indicate overhead

    P1OUT |= BIT0;      // Debug
    // Run the atomic function...
}

void atom_func_end(uint8_t func_id) {
    // ...Atomic function ends
    P1OUT &= ~BIT0;

    // Indicate completion
    P7OUT |= BIT1;
    __delay_cycles(0xF);
    P7OUT &= ~BIT1;

    // *** Discharging cycle ends ***
    P7OUT |= BIT0;      // Indicate overhead

    atom_state[func_id].check_fail = 0;     // Succefully complete
    if (!(CECTL1 & CEOUT)) {     // If target end threshold is not met
        atom_state[func_id].adapt_threshold++;
        atom_state[func_id].v_exe_hist_index = 0;
    }
    TA0CTL &= ~MC;  // Stop Timer A0

    if (timer_cnt1 > MIN_PROFILING_TIMER_CNT) {    // Don't profile if charging time is too short
        // Take a Vcc reading, get Delta V_exe
        adc_r1 = sample_vcc();
        d_v_discharge = (int16_t) adc_r2 - (int16_t) adc_r1;

        // Take a time reading, get T_exe
        timer_cnt2 = TA0R - timer_cnt1;  // T_exe

        // Calculate the actual voltage drop
#ifdef FLOAT_POINT_ARITHMETIC
        // Float-point method
        v_exe = (float) timer_cnt2 / timer_cnt1 * d_v_charge + d_v_discharge;
#else
        // Integer method (preferred)
        v_exe = (uint16_t) ((((timer_cnt2 * 0x100) / timer_cnt1) * d_v_charge +
                            (d_v_discharge * 0x100)) / 0x100);
#endif

        if (v_exe < 4096) {  // Discard illegal results over 4096
            // atom_state[func_id].v_exe_mean -= atom_state[func_id].v_exe_history[atom_state[func_id].v_exe_hist_index] / V_EXE_HISTORY_SIZE;
            atom_state[func_id].v_exe_history[atom_state[func_id].v_exe_hist_index] = v_exe;
            // atom_state[func_id].v_exe_mean += atom_state[func_id].v_exe_history[atom_state[func_id].v_exe_hist_index] / V_EXE_HISTORY_SIZE;

#ifdef DEBUG_UART
            // UART debug info
            char str_buffer[20];
            // uart_send_str(uitoa_10(v_exe, str_buffer));
            uart_send_str(uitoa_10(timer_cnt1, str_buffer));
            uart_send_str(" ");
            uart_send_str(uitoa_10(timer_cnt2, str_buffer));
            uart_send_str("\n\r");
#endif

            if (++atom_state[func_id].v_exe_hist_index == V_EXE_HISTORY_SIZE) {
                atom_state[func_id].v_exe_hist_index = 0;   // Reset index

                // Calculate v_exe_mean
                atom_state[func_id].v_exe_mean = 0;
                for (int i = 0; i < V_EXE_HISTORY_SIZE; i++) {
                    atom_state[func_id].v_exe_mean += atom_state[func_id].v_exe_history[i];
                }
                atom_state[func_id].v_exe_mean /= V_EXE_HISTORY_SIZE;

                // Adapt the next threshold
                atom_state[func_id].adapt_threshold = (uint8_t) (atom_state[func_id].v_exe_mean / UNIT_COMPE_ADC) + 1 + TARGET_END_THRESHOLD;
                if (atom_state[func_id].adapt_threshold > 31) {     // Prevent illegal values
                    atom_state[func_id].adapt_threshold = 31;
                }
#ifdef DEBUG_UART
                // UART debug info
                // char str_buffer[20];
                uart_send_str("mean: ");
                uart_send_str(uitoa_10(atom_state[func_id].v_exe_mean, str_buffer));
                // uart_send_str(uitoa_10(atom_state[func_id].adapt_threshold, str_buffer));
                uart_send_str("\n\r");
#endif
            }
        }
    }

    P7OUT &= ~BIT0;     // Indicate overhead
    CEINT |= CEIIE;
}

#else
// Disconnect supply profiling

void atom_func_start(uint8_t func_id) {
        // *** Charging cycle starts ***

    // Sleep here if (Vcc < adapt_threshold) until adapt_threshold is hit
    // ..or continue directly if (Vcc > adapt_threshold)
    // Sleep and wait for energy recharged
    storing_energy = 1;
    // set_CEREF1(atom_state[func_id].adapt_threshold);    // Adaptive threshold
    set_CEREF1(FIXED_PROFILING_THRESHOLD);              // Fixed profiling threshold
    COMPARATOR_DELAY;  // May sleep here until Hi Vth is reached
    set_CEREF1(TARGET_END_THRESHOLD);
    storing_energy = 0;

    // *** Charging cycle ends, discharging cycle starts ***
    // Disconnect supply
    CEINT &= ~CEIIE;
    P1OUT |= BIT5;
    // Take a Vcc reading, get Delta V_charge
    // adc_r2 = sample_vcc();

    // Run the atomic function...
    P1OUT |= BIT0;  // Debug
}

void atom_func_end(uint8_t func_id) {
    // ...Atomic function ends
    P1OUT &= ~BIT0;

    // Indicate completion
    P7OUT |= BIT1;
    __delay_cycles(0xF);
    P7OUT &= ~BIT1;

    // *** Discharging cycle ends ***
    // Reconnect supply
    P1OUT &= ~BIT5;
    // Take a Vcc reading, get Delta V_exe
    // adc_r1 = sample_vcc();
    // d_v_discharge = (int16_t) adc_r2 - (int16_t) adc_r1;

    // atom_state[func_id].v_exe_mean -= atom_state[func_id].v_exe_history[atom_state[func_id].v_exe_hist_index] / V_EXE_HISTORY_SIZE;
    // atom_state[func_id].v_exe_history[atom_state[func_id].v_exe_hist_index] = (uint16_t) d_v_discharge;
    // atom_state[func_id].v_exe_mean += atom_state[func_id].v_exe_history[atom_state[func_id].v_exe_hist_index] / V_EXE_HISTORY_SIZE;
    // if (++atom_state[func_id].v_exe_hist_index == V_EXE_HISTORY_SIZE) {
    //     atom_state[func_id].v_exe_hist_index = 0;
    // }

#ifdef DEBUG_UART
    // UART debug info
    char str_buffer[20];
    // P1OUT |= BIT0;      // Debug
    uart_send_str(uitoa_10(atom_state[func_id].v_exe_mean, str_buffer));
    uart_send_str("\n\r");
    // P1OUT &= ~BIT0;     // Debug
#endif
    CEINT |= CEIIE;
}

#endif


// Linear adaptation utility

// The following parameters should be dedicated to each function later
uint16_t PERSISTENT param_min = 0xFFFF;
uint16_t PERSISTENT param_max = 0;
uint16_t PERSISTENT y_min = 0xFFFF;
uint16_t PERSISTENT y_max = 0;
// uint16_t PERSISTENT slope = 0;
uint16_t PERSISTENT offset = UNIT_COMPE_ADC * 8;
uint16_t PERSISTENT y_curr;

void atom_func_start_linear(uint8_t func_id, uint16_t param) {
    // If the task failed before, reset the profiling
    if (atom_state[func_id].check_fail) {
        offset += UNIT_COMPE_ADC;
    }

    if (param_max <= param_min) {
        y_curr = offset;
    } else {
        y_curr = (y_max - y_min) * param / (param_max - param_min) + offset;
    }

    // *** Charging cycle starts ***
    CEINT &= ~CEIIE;
    P7OUT |= BIT0;      // Indicate overhead

    // Take a Vcc reading
    adc_r1 = sample_vcc();
    // Clear and start Timer A0
    TA0CTL = TASSEL__ACLK | MC__CONTINUOUS | TACLR;

    P7OUT &= ~BIT0;     // Indicate overhead
    CEINT |= CEIIE;

    // Sleep here if (Vcc < adapt_threshold) until adapt_threshold is hit
    // ..or continue directly if (Vcc > adapt_threshold)
    // Sleep and wait for energy recharged
    storing_energy = 1;
    uint8_t temp = (uint8_t) (y_curr / UNIT_COMPE_ADC + 1 + TARGET_END_THRESHOLD);
    if (temp > 31) temp = 31;
    set_CEREF1(temp);
    COMPARATOR_DELAY;  // May sleep here until Hi Vth is reached
    set_CEREF1(TARGET_END_THRESHOLD);
    storing_energy = 0;

    // *** Charging cycle ends, discharging cycle starts ***
    CEINT &= ~CEIIE;
    P7OUT |= BIT0;      // Indicate overhead
    // Take a time reading, get T_charge
    timer_cnt1 = TA0R;  // T_charge

    if (timer_cnt1 > MIN_PROFILING_TIMER_CNT) {  // Don't profile if charging time is too short
        // Take a Vcc reading, get Delta V_charge
        adc_r2 = sample_vcc();
        d_v_charge = (int16_t) adc_r2 - (int16_t) adc_r1;
    }
    atom_state[func_id].check_fail = 1;
    P7OUT &= ~BIT0;     // Indicate overhead

    P1OUT |= BIT0;  // Debug
    // Run the atomic function...
}

void atom_func_end_linear(uint8_t func_id, uint8_t param) {
    // ...Atomic function ends
    P1OUT &= ~BIT0;

    // Indicate completion
    P7OUT |= BIT1;
    __delay_cycles(0xF);
    P7OUT &= ~BIT1;

    // *** Discharging cycle ends ***
    P7OUT |= BIT0;      // Indicate overhead

    atom_state[func_id].check_fail = 0;     // Succefully complete
    if (!(CECTL1 & CEOUT)) {     // If target end threshold is not met
        offset += UNIT_COMPE_ADC;
    }
    TA0CTL &= ~MC;  // Stop Timer A0

    if (timer_cnt1 > MIN_PROFILING_TIMER_CNT) {    // Don't profile if charging time is too short
        // Take a Vcc reading, get Delta V_exe
        adc_r1 = sample_vcc();
        d_v_discharge = (int16_t) adc_r2 - (int16_t) adc_r1;

        // Take a time reading, get T_exe
        timer_cnt2 = TA0R - timer_cnt1;  // T_exe

        // Calculate the actual voltage drop
#ifdef FLOAT_POINT_ARITHMETIC
        // Float-point method
        v_exe = (float) timer_cnt2 / timer_cnt1 * d_v_charge + d_v_discharge;
#else
        // Integer method
        v_exe = (uint16_t) ((((timer_cnt2 * 0x100) / timer_cnt1) * d_v_charge +
                            (d_v_discharge * 0x100)) / 0x100);
#endif

        if (v_exe < 4096) {  // Discard illegal results over 4096
            if (param <= param_min) {
                param_min = param;
                y_min = v_exe;
            }
            if (param >= param_max) {
                param_max = param;
                y_max = v_exe;
            }

            // offset = offset - y_curr / LINEAR_ADAPT_COEFFICIENT + v_exe / LINEAR_ADAPT_COEFFICIENT;
            offset += (v_exe - y_curr) / LINEAR_ADAPT_COEFFICIENT;
#ifdef DEBUG_UART
            // UART debug info
            char str_buffer[20];
            uart_send_str(uitoa_10((uint16_t) v_exe, str_buffer));
            uart_send_str(" ");
            uart_send_str(uitoa_10((uint16_t) y_curr, str_buffer));
            uart_send_str(" ");
            uart_send_str(uitoa_10((uint16_t) offset, str_buffer));
            uart_send_str("\n\r");
#endif
        }
    }
    P7OUT &= ~BIT0;     // Indicate overhead
    CEINT |= CEIIE;
}


// Old code
/*
void atom_func_start(uint8_t func_id) {
#ifndef DEBS  // Our approach
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
            (CEREF0 & ((uint16_t) atom_state[func_id].start_thr));

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

#ifndef DEBS  // Our own approach
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
        // atom_state[func_id].start_thr =
            // ((int)adc_r1 - (int)adc_r2 < 512) ?
            // 20 :(uint8_t)((double)(adc_r1 - adc_r2) / 4095.0 * 32.0) + 17;
        if (adc_r1 > adc_r2) {
            atom_state[func_id].start_thr =
                (uint8_t)((double)(adc_r1 - adc_r2) / 4095.0 * 32.0 + 15.0);
        } else {
            atom_state[func_id].start_thr = (uint8_t) 17;
        }
        // atom_state[func_id].start_thr = (adc_r1 > adc_r2) ?
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
*/
