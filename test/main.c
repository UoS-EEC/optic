#include <msp430fr5994.h>
#include "ic.h"

int main(void) {
    // Setup RTC Timer
    RTCCTL0_H = RTCKEY_H;                   // Unlock RTC

    RTCCTL0_L = RTCTEVIE_L;                 // RTC event interrupt enable

    RTCCTL13 = RTCTEV_1 | RTCHOLD;  // Counter Mode, 32-kHz crystal, 16-bit ovf

    RTCCTL13 &= ~(RTCHOLD);                 // Start RTC

    // __bis_SR_register(LPM3_bits | GIE);     // Enter LPM3 mode w/ interrupts enabled
    // __no_operation();

    for (;;) {
        __delay_cycles(1000000);
        P1OUT ^= BIT5;
    }

    return 0;
}

void __attribute__((interrupt(RTC_C_VECTOR))) RTC_ISR(void) {
    switch (__even_in_range(RTCIV, RTCIV__RT1PSIFG)) {
        case RTCIV__RTCTEVIFG:              // RTCEVIFG
            P1OUT ^= BIT0;                  // Toggle P1.0 LED
            break;
        default: break;
    }
}
