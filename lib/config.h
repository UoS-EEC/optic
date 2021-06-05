// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef LIB_CONFIG_H_
#define LIB_CONFIG_H_

// #define DEBS
#define TRACK_STACK

// Indicating the maximum size of each FRAM snapshot
#define STACK_SIZE 0x0100  // 256 bytes stack, should >= __stack_size in .ld
// #define HEAP_SIZE 0x00A0  // 160 bytes heap, if there is heap
#define DATA_SIZE 0x0800  // at most 2KB
#define BSS_SIZE 0x0800  // at most 2KB

#define MAX_ADC_READING 4095
#define MAX_COMPE_RTAP 32  // Max comparator E resistor tap setting

#define COMPE_DEFAULT_HI_THRESHOLD    0x0017
#define COMPE_DEFAULT_LO_THRESHOLD    0x1300

#endif  // LIB_CONFIG_H_
