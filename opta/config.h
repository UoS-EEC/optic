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

#define DEFAULT_HI_THRESHOLD        68      // 2.4V
#define DEFAULT_LO_THRESHOLD        99      // 2.0V
#define TARGET_END_THRESHOLD        120
#define PROFILING_INIT_THRESHOLD    26
#define FIXED_PROFILING_THRESHOLD   23
// adc_to_threshold[63] = {
//     116,  //   0, 1.836V
//     113,  //   1, 1.863V
//     110,  //   2, 1.892V
//     107,  //   3, 1.921V
//     105,  //   4, 1.941V
//     102,  //   5, 1.972V
//      99,  //   6, 2.004V
//      97,  //   7, 2.025V
//      94,  //   8, 2.059V
//      92,  //   9, 2.082V
//      89,  //  10, 2.117V
//      87,  //  11, 2.142V
//      85,  //  12, 2.167V
//      82,  //  13, 2.205V
//      80,  //  14, 2.231V
//      78,  //  15, 2.258V
//      76,  //  16, 2.286V
//      74,  //  17, 2.315V
//      72,  //  18, 2.344V
//      70,  //  19, 2.374V
//      68,  //  20, 2.404V
//      67,  //  21, 2.420V
//      65,  //  22, 2.452V
//      63,  //  23, 2.484V
//      61,  //  24, 2.518V
//      60,  //  25, 2.535V
//      58,  //  26, 2.570V
//      57,  //  27, 2.588V
//      55,  //  28, 2.624V
//      53,  //  29, 2.662V
//      52,  //  30, 2.681V
//      50,  //  31, 2.720V
//      49,  //  32, 2.740V
//      48,  //  33, 2.760V
//      46,  //  34, 2.802V
//      45,  //  35, 2.823V
//      44,  //  36, 2.844V
//      42,  //  37, 2.889V
//      41,  //  38, 2.911V
//      40,  //  39, 2.934V
//      39,  //  40, 2.957V
//      37,  //  41, 3.005V
//      36,  //  42, 3.029V
//      35,  //  43, 3.054V
//      34,  //  44, 3.079V
//      33,  //  45, 3.105V
//      32,  //  46, 3.131V
//      31,  //  47, 3.158V
//      30,  //  48, 3.185V
//      29,  //  49, 3.212V
//      28,  //  50, 3.240V
//      27,  //  51, 3.268V
//      26,  //  52, 3.297V
//      25,  //  53, 3.327V
//      24,  //  54, 3.357V
//      23,  //  55, 3.387V
//      22,  //  56, 3.418V
//      21,  //  57, 3.450V
//      20,  //  58, 3.482V
//      19,  //  59, 3.515V
//      18,  //  60, 3.549V
//      18,  //  61, 3.549V
//      17,  //  62, 3.583V
// };


#define MIN_PROFILING_TIMER_CNT     4
#define V_EXE_HISTORY_SIZE          10

#define DISCONNECT_SUPPLY_PROFILING     // Disconnect supply when profiling
                                        // Remember to configure P1.5 for short-circuiting
                                        // .. both in HW and SW

#define LINEAR_ADAPT_COEFFICIENT    4


#define DEBUG_GPIO
// #define DEBUG_UART


#endif  // OPTA_CONFIG_H_
