// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef OPTA_CONFIG_H_
#define OPTA_CONFIG_H_

#include <stdint.h>         // For uint8__t type

#define TRACK_STACK

// Indicating the maximum size of each FRAM snapshot
#define REGISTER_SIZE   15
#define STACK_SIZE  0x0100  // 256 bytes stack, should >= __stack_size in .ld
// #define HEAP_SIZE   0x00A0  // 160 bytes heap, if there is heap
#define DATA_SIZE   0x0800  // at most 2KB
#define BSS_SIZE    0x0800  // at most 2KB

#define MAX_ADC_READING 4095

// Disconnect supply when profiling
// Connect P1.5 to the short-circuiting gate to actually disconnect
// #define DISCONNECT_SUPPLY_PROFILING

#define COMPARATOR_DELAY            __delay_cycles(280)     // 35us

#define DEFAULT_HI_THRESHOLD        65      // Value from threshold table
#define DEFAULT_LO_THRESHOLD        95      // Value from threshold table
                                            // Should be 2V
                                            // otherwise our ADC doesn't work
#define PROFILING_THRESHOLD         31      // Index from threshold table, initial threshold
#define FIXED_THRESHOLD             35      // Used in test
#define THRESHOLD_TABLE_MAX_INDEX   38
#define ADC_STEP                    32

// Target end threshold: 95 Voltage: 1.999 V
// Threshold convert table:
uint8_t adc_to_threshold[51] = {
    92,     //   0, 2.032V
    89,     //   1, 2.067V
    87,     //   2, 2.091V
    84,     //   3, 2.127V
    82,     //   4, 2.153V
    79,     //   5, 2.192V
    77,     //   6, 2.218V
    75,     //   7, 2.246V
    72,     //   8, 2.288V
    70,     //   9, 2.317V
    68,     //  10, 2.347V
    66,     //  11, 2.378V
    64,     //  12, 2.409V
    62,     //  13, 2.442V
    60,     //  14, 2.475V
    58,     //  15, 2.509V
    56,     //  16, 2.544V
    55,     //  17, 2.562V
    53,     //  18, 2.598V
    51,     //  19, 2.636V
    50,     //  20, 2.655V
    48,     //  21, 2.695V
    47,     //  22, 2.715V
    45,     //  23, 2.756V
    44,     //  24, 2.777V
    42,     //  25, 2.820V
    41,     //  26, 2.842V
    39,     //  27, 2.887V
    38,     //  28, 2.910V
    37,     //  29, 2.934V
    35,     //  30, 2.982V
    34,     //  31, 3.006V
    33,     //  32, 3.031V
    31,     //  33, 3.083V
    30,     //  34, 3.109V
    29,     //  35, 3.136V
    28,     //  36, 3.163V
    27,     //  37, 3.191V
    26,     //  38, 3.219V
    25,     //  39, 3.248V
    24,     //  40, 3.277V
    22,     //  41, 3.337V
    21,     //  42, 3.368V
    20,     //  43, 3.400V
    19,     //  44, 3.432V
    18,     //  45, 3.464V
    18,     //  46, 3.464V
    17,     //  47, 3.498V
    16,     //  48, 3.531V
    15,     //  49, 3.566V
    14,     //  50, 3.601V
};


#define MIN_PROFILING_TIMER_CNT     4
// #define V_EXE_HISTORY_SIZE          3   // Better be < 10
#define DELAY_COUNTER               5   // Should be > 1, otherwise comment

#define LINEAR_ADAPTATION
#ifdef LINEAR_ADAPTATION

#define LINEAR_FIT_OVERHEAD
// #define METHOD1
#define METHOD2
#if defined(METHOD1)
#define HIST_SIZE                   5
#define DELAY_COUNTER_LINEAR        10
#elif defined(METHOD2)
#else
#error Specify a linear adaptation method.
#endif



#endif


// #define DEBUG_GPIO
// #define DEBUG_UART
#define DEBUG_COMPLETION_INDICATOR
#define DEBUG_TASK_INDICATOR
// #define DEBUG_ADC_INDICATOR

// #define RADIO

#endif  // OPTA_CONFIG_H_
