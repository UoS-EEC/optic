// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include "test/ic.h"

// uint16_t ADCvar;

void dummy_function(void) {
    profiling_start(TEST_FUNC);
    P1OUT |= BIT0;
    // __delay_cycles(40000);  // 5ms delay
    uint16_t i = 4000;
    while (i--) {}
    P1OUT &= ~BIT0;
    // Should see P1.0 be 0->1->0 if this function ends completely
    profiling_end(TEST_FUNC);
}

int main(void) {
    for (;;) {
        dummy_function();
    }

    return 0;
}

