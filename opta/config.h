// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef OPTA_CONFIG_H_
#define OPTA_CONFIG_H_

// #define DEBS
#define TRACK_STACK

// Indicating the maximum size of each FRAM snapshot
#define REGISTER_SIZE   15
#define STACK_SIZE  0x0100  // 256 bytes stack, should >= __stack_size in .ld
// #define HEAP_SIZE   0x00A0  // 160 bytes heap, if there is heap
#define DATA_SIZE   0x0800  // at most 2KB
#define BSS_SIZE    0x0800  // at most 2KB

#define MAX_ADC_READING 4095
#define MAX_COMPE_RTAP  32  // Max comparator E resistor tap setting
#define UNIT_COMPE_ADC  128



#define OPTA
// #define DEBS

// Disconnect supply when profiling
// Connect P1.5 to the short-circuiting gate to actually disconnect
// #define DISCONNECT_SUPPLY_PROFILING


#define DEFAULT_HI_THRESHOLD        66      // 2.40V
// #define DEFAULT_HI_THRESHOLD        96      // Fixed threshold
#define DEFAULT_LO_THRESHOLD        113     //
// #define MIN_THRESHOLD        116
#define PROFILING_INIT_THRESHOLD    42      // Index
// #define FIXED_THRESHOLD             84

#define THRESHOLD_TABLE_MAX_INDEX   63
#define ADC_STEP                    32

#define COMPARATOR_DELAY    __delay_cycles(180)

// Threshold convert table
uint8_t adc_to_threshold[63] = {
    113,  //   0, 1.834V
    110,  //   1, 1.862V
    107,  //   2, 1.891V
    104,  //   3, 1.921V
    102,  //   4, 1.941V
     99,  //   5, 1.972V
     96,  //   6, 2.005V
     94,  //   7, 2.027V
     91,  //   8, 2.061V
     89,  //   9, 2.084V
     86,  //  10, 2.120V
     84,  //  11, 2.145V
     82,  //  12, 2.171V
     80,  //  13, 2.197V
     78,  //  14, 2.223V
     76,  //  15, 2.250V
     74,  //  16, 2.278V
     72,  //  17, 2.307V
     70,  //  18, 2.336V
     68,  //  19, 2.367V
     66,  //  20, 2.397V
     64,  //  21, 2.429V
     62,  //  22, 2.462V
     61,  //  23, 2.478V
     59,  //  24, 2.512V
     57,  //  25, 2.547V
     56,  //  26, 2.565V
     54,  //  27, 2.601V
     53,  //  28, 2.620V
     51,  //  29, 2.658V
     50,  //  30, 2.677V
     48,  //  31, 2.717V
     47,  //  32, 2.737V
     46,  //  33, 2.758V
     44,  //  34, 2.800V
     43,  //  35, 2.822V
     42,  //  36, 2.843V
     40,  //  37, 2.888V
     39,  //  38, 2.911V
     38,  //  39, 2.934V
     37,  //  40, 2.958V
     36,  //  41, 2.982V
     34,  //  42, 3.031V
     33,  //  43, 3.057V
     32,  //  44, 3.082V
     31,  //  45, 3.108V
     30,  //  46, 3.135V
     29,  //  47, 3.162V
     28,  //  48, 3.189V
     27,  //  49, 3.217V
     26,  //  50, 3.246V
     25,  //  51, 3.275V
     24,  //  52, 3.304V
     23,  //  53, 3.334V
     22,  //  54, 3.365V
     21,  //  55, 3.396V
     20,  //  56, 3.428V
     19,  //  57, 3.460V
     19,  //  58, 3.460V
     18,  //  59, 3.493V
     17,  //  60, 3.527V
     16,  //  61, 3.561V
     15,  //  62, 3.596V
};


#define MIN_PROFILING_TIMER_CNT     4
#define V_EXE_HISTORY_SIZE          10
#define LINEAR_ADAPT_COEFFICIENT    4


#define DEBUG_GPIO
// #define DEBUG_UART


#endif  // OPTA_CONFIG_H_
