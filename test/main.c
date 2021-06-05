// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#include <msp430fr5994.h>
#include "test/ic.h"

// uint16_t ADCvar;

void dummy_function(void) {
    profiling_start(TEST_FUNC);
    P1OUT |= BIT0;
    __delay_cycles(40000);  // 5ms delay
    // __delay_cycles(8000);  // 1ms delay
    P1OUT &= ~BIT0;
    // Should see P1.0 be 0->1->0 if this function ends completely
    profiling_end(TEST_FUNC);
}

int main(void) {
    // RTCCTL13 &= ~(RTCHOLD);  // Start RTC

    for (;;) {
        // P4OUT |= BIT1;  // debug
        // ADCvar = sample_vcc();
        // P4OUT &= ~BIT1;  // debug
        // __bis_SR_register(LPM3_bits | GIE);
        // // ... Wake up from RTC event

        // __delay_cycles(4000000);  // Dummy 500ms delay

        dummy_function();
    }

    return 0;
}

