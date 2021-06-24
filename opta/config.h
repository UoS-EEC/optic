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

#define COMPE_DEFAULT_HI_THRESHOLD    0x0017
#define COMPE_DEFAULT_LO_THRESHOLD    0x1300
#define TARGET_END_THRESHOLD        19
#define PROFILING_INIT_THRESHOLD    27
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

#define V_EXE_HISTORY_SIZE      10

// #define FLOAT_POINT_ARITHMETIC  // Comment to use integer method

#endif  // OPTA_CONFIG_H_
