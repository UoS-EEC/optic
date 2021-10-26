// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef OPTA_IC_H_
#define OPTA_IC_H_

#include <stdint.h>

void atom_func_start(uint8_t func_id);
void atom_func_end(uint8_t func_id);
void atom_func_start_linear(uint8_t func_id, uint16_t param);
void atom_func_end_linear(uint8_t func_id, uint8_t param);

// Configs
// Atomic function IDs
#define AES_128_ENC         0
#define AES_128_DEC         1
#define AES_256_ENC         0
#define AES_256_DEC         1

#define DMA                 0

#define UART_SEND_STR_SZ    0

#define RADIO_TX_PAYLOAD    0

// Maximum number of atomic functions
#define ATOM_FUNC_NUM       2


#endif  // OPTA_IC_H_
