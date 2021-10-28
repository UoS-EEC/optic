// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef DEBS_IC_H_
#define DEBS_IC_H_

#include <stdint.h>

void atom_func_start(uint8_t func_id);
void atom_func_end(uint8_t func_id);

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


#endif  // DEBS_IC_H_
