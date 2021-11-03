// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include "dma/dma.h"
#include <msp430fr5994.h>
#ifdef OPTA
#include "opta/ic.h"
#elif defined(DEBS)
#include "debs/ic.h"
#else
#error Specify a method.
#endif

void dma(uint8_t* src, uint8_t* dst, uint16_t sz) {
    atom_func_start(DMA);

    // Configure DMA channel 0
    __data20_write_long((uintptr_t) &DMA0SA, (uintptr_t) src);
                                            // Source block address
    __data20_write_long((uintptr_t) &DMA0DA, (uintptr_t) dst);
                                            // Destination single address
    DMA0SZ = sz;                            // Block size
    DMA0CTL = DMADT_1 | DMASRCINCR_3 | DMADSTINCR_3;    // Rpt, inc
    DMA0CTL |= DMAEN;                       // Enable DMA0

    DMA0CTL |= DMAREQ;                  // Trigger block transfer

    // CPU should be halted here until the transfer is completed
    while (!(DMA0CTL & DMAIFG)) {}
    DMA0CTL &= ~DMAIFG;

    atom_func_end(DMA);
}
