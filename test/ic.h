// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef TEST_IC_H_
#define TEST_IC_H_

#include <stdint.h>
#include <stddef.h>

void atom_func_start(uint8_t func_id);
void atom_func_end(uint8_t func_id);

void profiling_start(uint8_t func_id);
void profiling_end(uint8_t func_id);

// configs
#define AES_128_ENC      0
#define AES_128_DEC      1
#define TEST_FUNC        2
#define ATOM_FUNC_NUM    3

#endif  // TEST_IC_H_
