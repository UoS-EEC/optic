#include <msp430fr5994.h>
#include "test/ic.h"

uint16_t ADCvar;
extern uint16_t adc_reading;

int main(void) {
    RTCCTL13 &= ~(RTCHOLD);  // Start RTC
    // ADC12CTL0 |= ADC12ENC;

    for (;;) {
        __bis_SR_register(LPM3_bits | GIE);
        // ... Wake up from RTC event

        // ~67us
        P1OUT |= BIT5;  // debug
        ADCvar = sample_vcc();
        P1OUT &= ~BIT5;  // debug
    }

    return 0;
}

void __attribute__((interrupt(RTC_C_VECTOR))) RTC_ISR(void) {
    switch (__even_in_range(RTCIV, RTCIV__RT1PSIFG)) {
        case RTCIV__RTCTEVIFG:              // RTCEVIFG
            // P1OUT ^= BIT0;                  // Toggle P1.0 LED
            __bic_SR_register_on_exit(LPM3_bits);
            break;
        default: break;
    }
}
