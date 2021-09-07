// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include "test/ic.h"
#include "test/config.h"


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
// uint16_t PERSISTENT register_snapshot[15];
// uint16_t PERSISTENT return_address;

// Snapshots of volatile memory
// uint8_t PERSISTENT bss_snapshot[BSS_SIZE];
// uint8_t PERSISTENT data_snapshot[DATA_SIZE];
// uint8_t PERSISTENT stack_snapshot[STACK_SIZE];
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

// static void adc12_init(void) {
//     // Configure ADC12

//     // 2.0 V reference selected, comment this to use 1.2V
//     // REFCTL0 |= REFVSEL_1;

//     ADC12CTL0 = ADC12SHT0_2 |   // 16 cycles sample and hold time
//                 ADC12ON;        // ADC12 on
//     ADC12CTL1 = ADC12PDIV_1 |   // Predivide by 4, from ~4.8MHz MODOSC
//                 ADC12SHP;       // SAMPCON is from the sampling timer
//     ADC12CTL2 = ADC12RES_2 |    // Default 12-bit conversion results, 14cycles conversion time
//                 ADC12PWRMD_1;   // Low-power mode

//     // Use battery monitor (1/2 AVcc)
//     // ADC12CTL3 = ADC12BATMAP;  // 1/2AVcc channel selected for ADC ch A31
//     // ADC12MCTL0 = ADC12INCH_31 |
//     //              ADC12VRSEL_1;  // VR+ = VREF buffered, VR- = Vss

//     // Use P3.0
//     // P3SEL1 |= BIT0;
//     // P3SEL0 |= BIT0;
//     // ADC12MCTL0 = ADC12INCH_12|      // Select ch A12 at P3.0
//     //              ADC12VRSEL_1;      // VR+ = VREF buffered, VR- = Vss
//     // Use P3.1
//     P3SEL1 |= BIT1;
//     P3SEL0 |= BIT1;
//     ADC12MCTL0 = ADC12INCH_13|      // Select ch A13 at P3.1
//                  ADC12VRSEL_1;      // VR+ = VREF buffered, VR- = Vss
//     while (!(REFCTL0 & REFGENRDY)) {}   // Wait for reference generator to settle
//     // ADC12IER0 = ADC12IE0;  // Enable ADC conv complete interrupt
// }

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
// static uint16_t sample_vcc(void) {
//     P8OUT |= BIT0;
//     ADC12CTL0 |= ADC12ENC | ADC12SC;  // Start sampling & conversion
//     // __bis_SR_register(LPM0_bits | GIE);
//     while (!(ADC12IFGR0 & BIT0)) {}
//     adc_reading = ADC12MEM0;
//     P8OUT &= ~BIT0;
//     return adc_reading;
// }

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
    // comp_init();

    storing_energy = 0;

    // P1OUT |= BIT4;  // Debug
    // __bis_SR_register(LPM4_bits | GIE);  // Enter LPM4 with interrupts enabled
    // __bic_SR_register(OSCOFF);
    // Processor sleeps
    // ...
    // Processor wakes up after interrupt (Hi V threshold hit)

    // Remaining initialization stack for normal execution
    clock_init();
    // adc12_init();
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
