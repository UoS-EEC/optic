// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include "opta/ic.h"

#include <msp430fr5994.h>
#include <stdbool.h>
#include <stddef.h>

#include "opta/config.h"

#ifdef RADIO
// Header for radio init functions
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

#define PERSISTENT __attribute__((section(".persistent")))

typedef struct AtomFuncState_s {
    // Check_fail:
    // Set at function entry, reset at exit,
    // ..so read as '1' at the entry means the function failed before
    bool check_fail;

    // Adaptive threshold
    uint8_t  adapt_threshold;   // Init: PROFILING_THRESHOLD
    // Profiling variables
#ifdef V_EXE_HISTORY_SIZE
    uint16_t v_exe_history[V_EXE_HISTORY_SIZE];
    uint8_t  v_exe_hist_index;  // Init: 0
#endif
#ifdef DELAY_COUNTER
    uint8_t delay_cnt;
#endif
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

// Control signals
bool PERSISTENT snapshot_valid = false;
bool PERSISTENT suspending = false;          // From backup: 1, from restore: 0
bool profiling;     // A profiling state indicator
                    // ..redundant but makes the code clearer

// Profiling parameters
uint16_t adc_r1;
uint16_t adc_r2;
uint32_t d_v_charge;
uint32_t d_v_discharge;
uint32_t timer_cnt1;
uint32_t timer_cnt2;
uint16_t v_exe;

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

//     snapshot_valid = true;  // For now, don't check snapshot valid
//     suspending = true;
// }

// static void restore(void) {
//     // allocate to .boot_stack to prevent being overwritten when copying .bss
//     static uint8_t *src __attribute__((section(".boot_stack")));
//     static uint8_t *dst __attribute__((section(".boot_stack")));
//     static size_t len __attribute__((section(".boot_stack")));

//     snapshot_valid = false;
//     /* comment "snapshot_valid = false"
//        for being able to restore from an old snapshot
//        if backup() fails to complete
//        Here, introducing code re-execution? */

//     suspending = false;

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

static void adc12_init(void) {
    ADC12CTL0 &= ~ADC12ENC;     // Stop conversion
    ADC12CTL0 &= ~ADC12ON;      // Turn off

    // Turning on REF makes ADC reading ~50us faster
    // ..but draw ~20uA more quiescent current
    // while (REFCTL0 & REFGENBUSY) {}
    // REFCTL0 = REFVSEL_1 |
    //           REFON_1;          // REF ON
    REFCTL0 = REFVSEL_1;        // 2.0V reference

    // Configure ADC12
    ADC12CTL0 = ADC12SHT0_0;
    ADC12CTL1 = ADC12PDIV_1 |   // Predivide by 4, from ~4.8MHz MODOSC
                ADC12SHP;       // SAMPCON is from the sampling timer
    ADC12CTL2 = ADC12RES_2  |   // Default 12-bit conversion results
                                // ..and 14 cycles conversion time
                ADC12PWRMD_1;   // Low-power mode

    // Use internal 1/2AVcc channel
    ADC12CTL3 = ADC12BATMAP;
    ADC12MCTL0 = ADC12INCH_31|          // Select ch A31
                 ADC12VRSEL_1;          // VR+ = VREF buffered, VR- = Vss

    // Use P3.1
    // P3SEL1 |= BIT1;
    // P3SEL0 |= BIT1;
    // ADC12MCTL0 = ADC12INCH_13|          // Select ch A13 at P3.1
    //              ADC12VRSEL_1;          // VR+ = VREF buffered, VR- = Vss

    // while (!(REFCTL0 & REFGENRDY)) {}    // Wait for reference generator to settle
    ADC12CTL0 |= ADC12ON;
}

// Take ~70us
static uint16_t sample_vcc(void) {
#ifdef DEBUG_ADC_INDICATOR
    P8OUT |= BIT0;
#endif
    ADC12CTL0 |= ADC12ENC | ADC12SC;    // Start sampling & conversion
    while (!(ADC12IFGR0 & BIT0)) {}
#ifdef DEBUG_ADC_INDICATOR
    P8OUT &= ~BIT0;
#endif
    return ADC12MEM0;
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
            if (P3IES & BIT0) {     // Falling edge, low threshold hit
                set_threshold(DEFAULT_HI_THRESHOLD);
                P3IES &= ~BIT0;     // Detect rising edge next
#ifdef  DEBUG_GPIO
                P1OUT |= BIT4;      // Debug
#endif
                __low_power_mode_3_on_exit();
            } else {                // Rising edge, high threshold hit
                set_threshold(DEFAULT_LO_THRESHOLD);
                P3IES |= BIT0;      // Detect falling edge next
#ifdef  DEBUG_GPIO
                P1OUT &= ~BIT4;     // Debug
#endif
                __low_power_mode_off_on_exit();
            }
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
void __attribute__((interrupt(RESET_VECTOR), naked, used, optimize("O0"))) opta_boot() {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    __set_SP_register(&__bootstackend);     // Boot stack
    __dint();                               // Disable interrupts during startup

    gpio_init();
    clock_init();
    ext_comp_init();
    adc12_init();
#ifdef DEBUG_UART
    uart_init();
#endif

#ifdef RADIO
    // Radio init functions, comment this for other workloads
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

    if (snapshot_valid == true) {
        // P1OUT |= BIT5;  // Debug, restore() starts
        // restore();
        // Does not return here!!!
        // Return to the next line of backup()...
    } else {
        // snapshot_valid = false when
        // ..previous snapshot failed or 1st boot (both rare)
        // Normal boot...

        // Init atom_state
        for (uint8_t i = 0; i < ATOM_FUNC_NUM; i++) {
            atom_state[i].adapt_threshold = PROFILING_THRESHOLD;
            atom_state[i].check_fail = false;
#ifdef V_EXE_HISTORY_SIZE
            atom_state[i].v_exe_hist_index = 0;
#endif
#ifdef DELAY_COUNTER
            atom_state[i].delay_cnt = 1;    // Profiling at the first rrun
#endif
        }
        snapshot_valid = true;  // Temporary line, make sure atom_state init only once
    }

    // Load .data to RAM
    fastmemcpy(&__datastart, &__romdatastart, &__dataend - &__datastart);

    __set_SP_register(&__stackend);  // Runtime stack

    int main();  // Suppress implicit decl. warning
    main();
}

void atom_func_start(uint8_t func_id) {
    DISABLE_EXTCOMP_INTERRUPT;

#ifdef DEBUG_GPIO
    P7OUT |= BIT0;      // Indicate overhead
#endif

    // If the task failed before, reset the profiling
    if (atom_state[func_id].check_fail) {
        atom_state[func_id].check_fail = false;
        atom_state[func_id].adapt_threshold += 2;   // Increase threshold by ~50mV
        // Prevent illegal values
        if (atom_state[func_id].adapt_threshold > THRESHOLD_TABLE_MAX_INDEX) {
            atom_state[func_id].adapt_threshold = THRESHOLD_TABLE_MAX_INDEX;
        }
    }

    // Sleep here if (Vcc < adapt_threshold) until adapt_threshold is hit
    // ..or continue directly if (Vcc > adapt_threshold)
    set_threshold(adc_to_threshold[atom_state[func_id].adapt_threshold]);
    // set_threshold(FIXED_THRESHOLD);
    COMPARATOR_DELAY;
    if (P3IN & BIT0) {          // Enough energy
        profiling = false;
        set_threshold(DEFAULT_LO_THRESHOLD);
    } else {                    // Not enough energy
#ifdef DELAY_COUNTER
        if (--atom_state[func_id].delay_cnt == 0) {
            profiling = true;
            atom_state[func_id].delay_cnt = DELAY_COUNTER;
        }
#else
        profiling = true;
#endif
        if (profiling) {
            // Take a Vcc reading
            // P3.0 interrupt preempted by this, so we have to manually sleep later
            adc_r1 = sample_vcc();
            // Clear and start Timer A0
            TA0CTL = TASSEL__ACLK | MC__CONTINUOUS | TACLR;
        }
        P3IFG &= ~BIT0;         // Clear pending falling edge interrupt
        P3IES &= ~BIT0;         // Detect rising edge next
        ENABLE_EXTCOMP_INTERRUPT;
#ifdef DEBUG_GPIO
        P7OUT &= ~BIT0;         // Indicate overhead
#endif
#ifdef  DEBUG_GPIO
        P1OUT |= BIT4;          // Debug
#endif
        // Sleep and wait for energy refills...
        __low_power_mode_3();
        __nop();
        // ...Wake from the ISR when adapt_threshold is hit
        DISABLE_EXTCOMP_INTERRUPT;

        // *** Charging cycle ends, discharging cycle starts ***
#ifdef DEBUG_GPIO
        P7OUT |= BIT0;          // Indicate overhead
#endif
        if (profiling) {
            timer_cnt1 = TA0R;      // Take a time reading, get T_charge
            // Check if charging time is too short
            if (timer_cnt1 > MIN_PROFILING_TIMER_CNT) {
                // Continue profiling
                // Take a Vcc reading, get Delta V_charge
                adc_r2 = sample_vcc();
                d_v_charge = (int16_t) adc_r2 - (int16_t) adc_r1;
            } else {
                // Charging time too short, stop profiling
                profiling = false;
                TA0CTL &= ~MC;      // Stop Timer A0
            }
        }
    }
    atom_state[func_id].check_fail = true;
#ifdef DEBUG_GPIO
    P7OUT &= ~BIT0;             // Indicate overhead
#endif
#ifdef DISCONNECT_SUPPLY_PROFILING
    P1OUT |= BIT5;              // Disconnect supply, only valid when P1.5 is connected
#endif
#ifdef DEBUG_TASK_INDICATOR
    P1OUT |= BIT0;              // Debug
#endif
    // Atomic function starts...
}

void atom_func_end(uint8_t func_id) {
    // ... Atomic function ends
#ifdef DEBUG_TASK_INDICATOR
    P1OUT &= ~BIT0;
#endif
#ifdef DISCONNECT_SUPPLY_PROFILING
    P1OUT &= ~BIT5;             // Reconnect supply, only valid when P1.5 is connected
#endif
#ifdef DEBUG_COMPLETION_INDICATOR
    // Indicate completion
    P7OUT |= BIT1;
    __delay_cycles(0xF);
    P7OUT &= ~BIT1;
#endif
    // *** Discharging cycle ends ***
#ifdef DEBUG_GPIO
    P7OUT |= BIT0;              // Indicate overhead
#endif
    // Succefully complete, clear the check_fail flag
    atom_state[func_id].check_fail = false;
    // If target end threshold is violated, though didn't die (1.8V < Vcc < Vtarget_end)
    if ((P3IN & BIT0) == 0) {
        atom_state[func_id].adapt_threshold += 2;
        // Prevent illegal values
        if (atom_state[func_id].adapt_threshold > THRESHOLD_TABLE_MAX_INDEX) {
            atom_state[func_id].adapt_threshold = THRESHOLD_TABLE_MAX_INDEX;
        }
        profiling = false;
    }

    if (profiling) {
        TA0CTL &= ~MC;                      // Stop Timer A0
        timer_cnt2 = TA0R - timer_cnt1;     // Take a time reading, get T_exe
        adc_r1 = sample_vcc();              // Take a Vcc reading, get Delta V_exe
        d_v_discharge = (int16_t) adc_r2 - (int16_t) adc_r1;
        // Calculate the actual voltage drop (integer method)
        v_exe = (uint16_t) ((((timer_cnt2 * 0x100) / timer_cnt1) * d_v_charge +
                            (d_v_discharge * 0x100)) / 0x100);

        if (v_exe < MAX_ADC_READING) {      // Discard illegal results >= MAX_ADC_READING
#ifdef V_EXE_HISTORY_SIZE
            atom_state[func_id].v_exe_history[atom_state[func_id].v_exe_hist_index] = v_exe;
            if (++atom_state[func_id].v_exe_hist_index == V_EXE_HISTORY_SIZE) {
                atom_state[func_id].v_exe_hist_index = 0;

                // Calculate v_exe_mean
                uint32_t v_exe_mean = 0;
                for (int i = 0; i < V_EXE_HISTORY_SIZE; ++i) {
                    v_exe_mean += atom_state[func_id].v_exe_history[i];
                }
                v_exe_mean /= V_EXE_HISTORY_SIZE;

                // Adapt the next threshold
                atom_state[func_id].adapt_threshold = (uint8_t) (v_exe_mean / ADC_STEP);
                // Prevent illegal values
                if (atom_state[func_id].adapt_threshold > THRESHOLD_TABLE_MAX_INDEX) {
                    atom_state[func_id].adapt_threshold = THRESHOLD_TABLE_MAX_INDEX;
                }
            }
#else   // No V_exe history table
            atom_state[func_id].adapt_threshold = (uint8_t) (v_exe / ADC_STEP);
            // Prevent illegal values
            if (atom_state[func_id].adapt_threshold > THRESHOLD_TABLE_MAX_INDEX) {
                atom_state[func_id].adapt_threshold = THRESHOLD_TABLE_MAX_INDEX;
            }
#endif  // V_EXE_HISTORY_SIZE

#ifdef DEBUG_UART
            // UART debug info
            char str_buffer[20];
            uart_send_str(uitoa_10(v_exe, str_buffer));
            // uart_send_str(" ");
            // uart_send_str(uitoa_10(timer_cnt1, str_buffer));
            // uart_send_str(" ");
            // uart_send_str(uitoa_10(timer_cnt2, str_buffer));
            uart_send_str("\n\r");
#endif
        }
    }

    if (P3IN & BIT0) P3IFG &= ~BIT0;  // Prevent fake interrupt when Vcc > Vtarget_end here
#ifdef DEBUG_GPIO
    P7OUT &= ~BIT0;     // Indicate overhead
#endif
    ENABLE_EXTCOMP_INTERRUPT;
}


#ifdef LINEAR_ADAPTATION
// Linear adaptation utility

// The following parameters should be dedicated to each function later
uint16_t PERSISTENT x_min = 0xFFFF;
uint16_t PERSISTENT x_max = 0;
uint16_t PERSISTENT y_min = 0xFFFF;
uint16_t PERSISTENT y_max = 0;
// uint16_t PERSISTENT slope = 0;
uint16_t PERSISTENT offset = ADC_STEP * 8;
uint16_t PERSISTENT y_curr = ADC_STEP * PROFILING_THRESHOLD;

void atom_func_start_linear(uint8_t func_id, uint16_t x) {
    // If the task failed before, reset the profiling
    if (atom_state[func_id].check_fail) {
        atom_state[func_id].check_fail = false;
        offset += ADC_STEP;
    }

    if (x_min < x_max) {
        y_curr = (y_max - y_min) * x / (x_max - x_min) + offset;
    }

    // Sleep here if (Vcc < adapt_threshold) until adapt_threshold is hit
    // ..or continue directly if (Vcc > adapt_threshold)
    set_threshold(adc_to_threshold[y_curr / ADC_STEP]);
    COMPARATOR_DELAY;
    if (P3IN & BIT0) {          // Enough energy
        profiling = false;
        set_threshold(DEFAULT_LO_THRESHOLD);
    } else {                    // Not enough energy
#ifdef DELAY_COUNTER
        if (--atom_state[func_id].delay_cnt == 0) {
            profiling = true;
            atom_state[func_id].delay_cnt = DELAY_COUNTER;
        }
#else
        profiling = true;
#endif
        if (profiling) {
            // Take a Vcc reading
            // P3.0 interrupt preempted by this, so we have to manually sleep later
            adc_r1 = sample_vcc();
            // Clear and start Timer A0
            TA0CTL = TASSEL__ACLK | MC__CONTINUOUS | TACLR;
        }
        P3IFG &= ~BIT0;         // Clear pending falling edge interrupt
        P3IES &= ~BIT0;         // Detect rising edge next
        ENABLE_EXTCOMP_INTERRUPT;
#ifdef DEBUG_GPIO
        P7OUT &= ~BIT0;         // Indicate overhead
#endif
#ifdef  DEBUG_GPIO
        P1OUT |= BIT4;          // Debug
#endif
        // Sleep and wait for energy refills...
        __low_power_mode_3();
        __nop();
        // ...Wake from the ISR when adapt_threshold is hit
        DISABLE_EXTCOMP_INTERRUPT;

        // *** Charging cycle ends, discharging cycle starts ***
#ifdef DEBUG_GPIO
        P7OUT |= BIT0;          // Indicate overhead
#endif
        if (profiling) {
            timer_cnt1 = TA0R;      // Take a time reading, get T_charge
            // Check if charging time is too short
            if (timer_cnt1 > MIN_PROFILING_TIMER_CNT) {
                // Continue profiling
                // Take a Vcc reading, get Delta V_charge
                adc_r2 = sample_vcc();
                d_v_charge = (int16_t) adc_r2 - (int16_t) adc_r1;
            } else {
                // Charging time too short, stop profiling
                profiling = false;
                TA0CTL &= ~MC;      // Stop Timer A0
            }
        }
    }
    atom_state[func_id].check_fail = true;




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
    uint8_t temp = (uint8_t) (y_curr / ADC_STEP + 1 + TARGET_END_THRESHOLD);
    if (temp > 31) temp = 31;
    set_CEREF1(temp);
    COMPARATOR_DELAY;  // May sleep here until Hi Vth is reached
    set_CEREF1(TARGET_END_THRESHOLD);

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
    atom_state[func_id].check_fail = true;
    P7OUT &= ~BIT0;     // Indicate overhead

    P1OUT |= BIT0;  // Debug
    // Run the atomic function...
}

void atom_func_end_linear(uint8_t func_id, uint8_t x) {
    // ...Atomic function ends
    P1OUT &= ~BIT0;

#ifdef DEBUG_COMPLETION_INDICATOR
    // Indicate completion
    P7OUT |= BIT1;
    __delay_cycles(0xF);
    P7OUT &= ~BIT1;
#endif

    // *** Discharging cycle ends ***
    P7OUT |= BIT0;      // Indicate overhead

    atom_state[func_id].check_fail = false;     // Succefully complete
    if (!(CECTL1 & CEOUT)) {     // If target end threshold is not met
        offset += ADC_STEP;
    }
    TA0CTL &= ~MC;  // Stop Timer A0

    if (timer_cnt1 > MIN_PROFILING_TIMER_CNT) {    // Don't profile if charging time is too short
        // Take a Vcc reading, get Delta V_exe
        adc_r1 = sample_vcc();
        d_v_discharge = (int16_t) adc_r2 - (int16_t) adc_r1;

        // Take a time reading, get T_exe
        timer_cnt2 = TA0R - timer_cnt1;  // T_exe

        // Calculate the actual voltage drop
        // Integer method
        v_exe = (uint16_t) ((((timer_cnt2 * 0x100) / timer_cnt1) * d_v_charge +
                            (d_v_discharge * 0x100)) / 0x100);

        if (v_exe < 4096) {  // Discard illegal results over 4096
            if (x <= x_min) {
                x_min = x;
                y_min = v_exe;
            }
            if (x >= x_max) {
                x_max = x;
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

#endif  // LINEAR_ADAPTATION
