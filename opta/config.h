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
#define DEFAULT_LO_THRESHOLD        96      // Value from threshold table
                                            // Should be just above 2V
                                            // otherwise our ADC doesn't work
#define PROFILING_THRESHOLD         31      // Index from threshold table, initial threshold
#define FIXED_THRESHOLD             35      // Used in test
#define THRESHOLD_TABLE_MAX_INDEX   38
#define ADC_STEP                    32

// Target end threshold: 94 Voltage: 2.010 V
// Threshold convert table:
uint8_t adc_to_threshold[51] = {
    92,     //   0, 2.032V
    89,     //   1, 2.067V
    86,     //   2, 2.103V
    84,     //   3, 2.127V
    81,     //   4, 2.165V
    79,     //   5, 2.192V
    76,     //   6, 2.232V
    74,     //   7, 2.260V
    72,     //   8, 2.288V
    70,     //   9, 2.317V
    68,     //  10, 2.347V
    66,     //  11, 2.378V
    64,     //  12, 2.409V
    62,     //  13, 2.442V
    60,     //  14, 2.475V
    58,     //  15, 2.509V
    56,     //  16, 2.544V
    54,     //  17, 2.580V
    53,     //  18, 2.598V
    51,     //  19, 2.636V
    49,     //  20, 2.675V
    48,     //  21, 2.695V
    46,     //  22, 2.735V
    45,     //  23, 2.756V
    43,     //  24, 2.798V
    42,     //  25, 2.820V
    40,     //  26, 2.864V
    39,     //  27, 2.887V
    38,     //  28, 2.910V
    36,     //  29, 2.957V
    35,     //  30, 2.982V
    34,     //  31, 3.006V
    32,     //  32, 3.057V
    31,     //  33, 3.083V
    30,     //  34, 3.109V
    29,     //  35, 3.136V
    28,     //  36, 3.163V
    27,     //  37, 3.191V
    26,     //  38, 3.219V
    24,     //  39, 3.277V
    23,     //  40, 3.307V
    22,     //  41, 3.337V
    21,     //  42, 3.368V
    20,     //  43, 3.400V
    19,     //  44, 3.432V
    18,     //  45, 3.464V
    17,     //  46, 3.498V
    16,     //  47, 3.531V
    15,     //  48, 3.566V
    15,     //  49, 3.566V
    14,     //  50, 3.601V
};

#define MIN_PROFILING_TIMER_CNT     4
// #define V_EXE_HISTORY_SIZE          3   // Better be < 10
#define DELAY_COUNTER               5   // Should be > 1, otherwise comment

#define LINEAR_ADAPTATION
#ifdef LINEAR_ADAPTATION

// #define METHOD1
#define METHOD2
#if defined(METHOD1)
#define HIST_SIZE                   5
#define LINEAR_FIT_OVERHEAD
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
