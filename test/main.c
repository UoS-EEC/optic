#include <msp430fr5994.h>
#include "ic.h"

int main(void) {
    for (;;) {

        __delay_cycles(1000000);
        P1OUT ^= BIT0;
    }

    return 0;
}
