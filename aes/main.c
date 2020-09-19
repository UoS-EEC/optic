#include <msp430fr5994.h>
#include "aesa.h"
#include "ic.h"

// unsigned char plaintext[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
//                             0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
// unsigned char ciphertext[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
//                       0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
// unsigned char iv[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// reference answer
// unsigned char ciphertext[] = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
//                             0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};

// the second set
unsigned char plaintext2[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
                              0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
                              0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 
                              0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};
unsigned char ciphertext2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char deciphertext2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
unsigned char iv[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};



unsigned char input[2048] =
    "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo "
    "ligula eget dolor. Aenean massa. Cum sociis natoque penatibus et magnis "
    "dis parturient montes, nascetur ridiculus mus. Donec quam felis, "
    "ultricies nec, pellentesque eu, pretium quis, sem. Nulla consequat massa "
    "quis enim. Donec pede justo, fringilla vel, aliquet nec, vulputate eget, "
    "arcu. In enim justo, rhoncus ut, imperdiet a, venenatis vitae, justo. "
    "Nullam dictum felis eu pede mollis pretium. Integer tincidunt. Cras "
    "dapibus. Vivamus elementum semper nisi. Aenean vulputate eleifend tellus. "
    "Aenean leo ligula, porttitor eu, consequat vitae, eleifend ac, enim. "
    "Aliquam lorem ante, dapibus in, viverra quis, feugiat a, tellus. "
    "Phasellus viverra nulla ut metus varius laoreet. Quisque rutrum. Aenean "
    "imperdiet. Etiam ultricies nisi vel augue. Curabitur ullamcorper "
    "ultricies nisi. Nam eget dui. Lorem ipsum dolor sit amet, consectetuer "
    "adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa. Cum "
    "sociis natoque penatibus et magnis dis parturient montes, nascetur "
    "ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium "
    "quis, sem. Nulla consequat massa quis enim. Donec pede justo, fringilla "
    "vel, aliquet nec, vulputate eget, arcu. In enim justo, rhoncus ut, "
    "imperdiet a, venenatis vitae, justo. Nullam dictum felis eu pede mollis "
    "pretium. Integer tincidunt. Cras dapibus. Vivamus elementum semper nisi. "
    "Aenean vulputate eleifend tellus. Aenean leo ligula, porttitor eu, "
    "consequat vitae, eleifend ac, enim. Aliquam lorem ante, dapibus in, "
    "viverra quis, feugiat a, tellus. Phasellus viverra nulla ut metus varius "
    "laoreet. Quisque rutrum. Aenean imperdiet. Etiam ultricies nisi vel "
    "augue. Curabitur ullamcorper ultricies nisi. Nam eget dui. Lorem ipsum "
    "dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget "
    "dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis "
    "parturient montes, nascetur ridiculus mus. Donec quam felis, ultricies "
    "nec, pellentesque eu, pretium quis, sem. Nulla consequat massa quis enim. "
    "Donec pede justo";




void timer_init(void) {
    TA0CCTL0 = CCIE;                // TACCR0 interrupt enabled
    TA0CCR0 = 0xFFFF;
    TA0CTL = TASSEL__SMCLK | ID__8; // SMCLK, divided by 8
}

int main(void) {
    // timer_init();

    // __enable_interrupt();
    // __bis_SR_register(GIE);

    // P1OUT |= BIT4; // debug
    // __bis_SR_register(LPM4_bits | GIE);
    
    for (;;) {
        // ******* Timer ******
        // P1OUT |= BIT0;
        // TA0CTL |= MC__UP;
        // __bis_SR_register(LPM0_bits);
        // P1OUT &= ~BIT0;


        // ******* AESA module test ******
        // P1OUT |= BIT0;
        // aes_128_enc(key, iv, plaintext2, ciphertext2, 2);
        // aes_128_dec(key, iv, ciphertext2, deciphertext2, 2);
        aes_128_enc(key, iv, input, input, 127);
        aes_128_dec(key, iv, input, input, 127);
        // P1OUT &= ~BIT0;



        // ******* ADC reading test ******
        // P1OUT |= BIT0;
        // ADC12CTL0 |= ADC12ENC | ADC12SC;    // Start sampling/conversion
        // __bis_SR_register(LPM0_bits);
        // P1OUT &= ~BIT0;
        
        // __bis_SR_register(LPM4_bits | GIE);
    }

    return 0;
}

// Timer0_A0 interrupt service routine
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_ISR (void) {
    // TA0CCR0 += 0xFFFF;                       // Add Offset to TA0CCR0
    TA0CTL &= ~MC; // halt timer
    __bic_SR_register_on_exit(LPM0_bits);
}

