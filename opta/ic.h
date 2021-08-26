// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef OPTA_IC_H_
#define OPTA_IC_H_

#include <stdint.h>
#include <stddef.h>

void atom_func_start(uint8_t func_id);
void atom_func_end(uint8_t func_id);
void atom_func_start_linear(uint8_t func_id, uint16_t param);
void atom_func_end_linear(uint8_t func_id, uint8_t param);

// Configs
#define AES_128_ENC         0
#define AES_128_DEC         1
#define AES_256_ENC         0
#define AES_256_DEC         1

#define DMA                 0

#define UART_SEND_STR_SZ    0

#define ATOM_FUNC_NUM       2   // Maximum number of tasks in all tests


#endif  // OPTA_IC_H_
