#include <msp430fr5994.h>
#include "aesa.h"
#include "ic.h"

unsigned char plaintext[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
                            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
unsigned char ciphertext[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
unsigned char iv[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// reference answer
// unsigned char ciphertext[] = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
//                             0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};


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


void gpio_init(void) {
    // Need to initialize all ports/pins to reduce power consump'n
    P1OUT = 0; 
    P1DIR = 0xff;
    P2OUT = 0;
    P2DIR = 0xff;
    P3OUT = 0;
    P3DIR = 0xff;
    P4OUT = 0; 
    P4DIR = 0xff;
    P7OUT = 0;
    P7DIR = 0xff;
    P8OUT = 0;
    P8DIR = 0xff;

    /* Keep-alive signal to external comparator */
    // P6OUT = BIT0;    // resistor enable
    // P6REN = BIT0;    // Pull-up mode
    // P6DIR = ~(BIT0); // All but 0 to output
    // P6OUT = BIT0;
    // P6DIR = 0xff;

    /* Interrupts from S1 and S2 buttons */
    // P5DIR &= ~(BIT5 | BIT6);        // All but 5,6 to output direction
    // P5OUT |= BIT5 | BIT6;           // Pull-up resistor
    // P5REN |= BIT5 | BIT6;           // Pull-up mode
    // P5IES = BIT5 | BIT6;            // Falling edge

    /* Interrupts from S2 button */
    // P5DIR &= ~BIT5;          // bit 5 input direction
    // P5OUT |= BIT5;           // Pull-up resistor
    // P5REN |= BIT5;           // Pull-up resistor enable
    // P5IES = BIT5;            // Falling edge

    // P5SEL0 = 0;
    // P5SEL1 = 0;
    // P5IFG = 0;               // Clear pending interrupts
    // P5IE = BIT5;             // Enable restore irq

    // Disable GPIO power-on default high-impedance mode
    PM5CTL0 &= ~LOCKLPM5;
}

void timer_init(void) {
    TA0CCTL0 = CCIE;                // TACCR0 interrupt enabled
    TA0CCR0 = 0xFFFF;
    TA0CTL = TASSEL__SMCLK | ID__8; // SMCLK, divided by 8
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    gpio_init();
    timer_init();
    adc12_init(); 
    comp_init();

    // __enable_interrupt();
    __bis_SR_register(GIE);

    // __bis_SR_register(LPM4_bits);
    
    for (;;) {
        // P1OUT |= BIT0;
        // TA0CTL |= MC__UP;
        // __bis_SR_register(LPM0_bits);
        // P1OUT &= ~BIT0;

        // P1OUT |= BIT0;
        // aes_128_enc(key, plaintext, ciphertext, iv, 1);
        // aes_128_enc(key, input, input, 128); // 128 might not work
        // aes_128_enc(key, input, input, 64);
        // aes_128_enc(key, input+0x100, input+0x100, 32);
        aes_128_enc(key, input, input, iv, 32);
        // aes_128_enc(key, input, input, 16);
        // aes_128_enc(key, input, input, 8);
        // aes_128_enc(key, input, input, 4);
        // aes_128_enc(key, input, input, 2);
        // P1OUT &= ~BIT0;

        // ADC12CTL0 |= ADC12ENC | ADC12SC;    // Start sampling/conversion
        // __bis_SR_register(LPM0_bits);

        // TA0CTL |= MC__UP;
        // __bis_SR_register(LPM0_bits);
        
        // __bis_SR_register(LPM4_bits | GIE);
    }

    return 0;
}


// Port 5 interrupt service routine
// void __attribute__ ((__interrupt__(PORT5_VECTOR))) Port5_ISR(void) {
//     if (P5IFG & BIT5) { 
//         if (__get_SR_register() & LPM4_bits) {
//             __bic_SR_register_on_exit(LPM4_bits);
//         } else {
//             __bis_SR_register_on_exit(LPM4_bits | GIE);
//         }
//         P5IFG &= ~BIT5; // Clear interrupt flags
//     } 
// }

// Timer0_A0 interrupt service routine
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_ISR (void) {
    // TA0CCR0 += 0xFFFF;                       // Add Offset to TA0CCR0
    TA0CTL &= ~MC; // halt timer
    __bic_SR_register_on_exit(LPM0_bits);
}

