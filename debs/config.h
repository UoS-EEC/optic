// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef DEBS_CONFIG_H_
#define DEBS_CONFIG_H_

#include <stdint.h>         // For uint8__t type

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


#define PROFILING           // Whether profiling
#define DISCONNECT_SUPPLY_PROFILING     // Whether disconnect supply when profiling
                                        // Connect P1.5 to the short-circuiting gate to actually disconnect

#define COMPARATOR_DELAY            __delay_cycles(280)     // 35us

// Target end threshold: 96 Voltage: 2.005 V
// Threshold convert table:
// uint8_t adc_to_threshold[51] = {
//     93,     //   0, 2.038V
//     90,     //   1, 2.072V
//     88,     //   2, 2.096V
//     85,     //   3, 2.133V
//     83,     //   4, 2.158V
//     80,     //   5, 2.197V
//     78,     //   6, 2.223V
//     76,     //   7, 2.250V
//     73,     //   8, 2.293V
//     71,     //   9, 2.322V
//     69,     //  10, 2.351V
//     67,     //  11, 2.382V
//     65,     //  12, 2.413V
//     63,     //  13, 2.445V
//     61,     //  14, 2.478V
//     59,     //  15, 2.512V
//     57,     //  16, 2.547V
//     56,     //  17, 2.565V
//     54,     //  18, 2.601V
//     52,     //  19, 2.639V
//     51,     //  20, 2.658V
//     49,     //  21, 2.697V
//     47,     //  22, 2.737V
//     46,     //  23, 2.758V
//     44,     //  24, 2.800V
//     43,     //  25, 2.822V
//     41,     //  26, 2.866V
//     40,     //  27, 2.888V
//     39,     //  28, 2.911V
//     37,     //  29, 2.958V
//     36,     //  30, 2.982V
//     35,     //  31, 3.006V
//     33,     //  32, 3.057V
//     32,     //  33, 3.082V
//     31,     //  34, 3.108V
//     30,     //  35, 3.135V
//     29,     //  36, 3.162V
//     28,     //  37, 3.189V
//     26,     //  38, 3.246V
//     25,     //  39, 3.275V
//     24,     //  40, 3.304V
//     23,     //  41, 3.334V
//     22,     //  42, 3.365V
//     21,     //  43, 3.396V
//     20,     //  44, 3.428V
//     19,     //  45, 3.460V
//     18,     //  46, 3.493V
//     17,     //  47, 3.527V
//     16,     //  48, 3.561V
//     15,     //  49, 3.596V
//     15,     //  50, 3.596V
// };


#define DEFAULT_HI_THRESHOLD        54      // See table above
#define DEFAULT_LO_THRESHOLD        96      // See table above


// #define DEBUG_GPIO
// #define DEBUG_UART
// #define DEBUG_COMPLETION_INDICATOR
#define DEBUG_TASK_INDICATOR
// #define DEBUG_ADC_INDICATOR


#endif  // DEBS_CONFIG_H_
