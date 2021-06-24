// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef OPTA_IC_H_
#define OPTA_IC_H_

#include <stdint.h>
#include <stddef.h>

void atom_func_start(uint8_t func_id);
void atom_func_end(uint8_t func_id);

// Configs
#define AES_128_ENC      0
#define AES_128_DEC      1
#define ATOM_FUNC_NUM    2

#endif  // OPTA_IC_H_
