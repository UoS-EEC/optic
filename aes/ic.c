#include <msp430fr5994.h>
#include "ic.h"

typedef struct AtomFuncState_s {
    uint8_t calibrated; // 0: need to calibrate, 1: no need
    uint8_t check_fail; // set at the function beginning, reset at the end; so '1' at the entry means failed
    uint8_t resume_thr; // resume threshold, represented as the resistor tap setting of the internal comparator
    uint8_t backup_thr; // backup threshold, represented as the resistor tap setting of the internal comparator
} AtomFuncState;

AtomFuncState atom_state[ATOM_FUNC_NUM];

uint8_t need_calibrate PERSISTENT = 1;
uint8_t check_fail PERSISTENT;
uint8_t resume_threshold PERSISTENT = 20; // resistor tap setting for VREF0, init 21/32, 2.625V
uint8_t backup_threshold PERSISTENT = 16; // resistor tap setting for VREF1, init 17/32, 2.125V


uint16_t adc_r1;
uint16_t adc_r2;
uint16_t adc_reading;



void adc12_init(void) {
    // Configure ADC12
    
    REFCTL0 |= REFVSEL_1;                   // 2.0 V reference selected, comment this to use 1.2V

    ADC12CTL0 = ADC12SHT0_2 |               // 16 cycles sample and hold time
                ADC12ON     ;               // ADC12 on
    ADC12CTL1 = ADC12PDIV_3 |               // Predive by 64
                ADC12SHP    ;               // SAMPCON is from the sampling timer
    // ADC12CTL2 = ADC12RES_2;                // Default: 12-bit conversion results, 14 cycles conversion time

    // Use battery monitor (1/2 AVcc)    
    // ADC12CTL3 = ADC12BATMAP;                // 1/2AVcc channel selected for ADC ch A31
    // ADC12MCTL0 = ADC12INCH_31 |
    //              ADC12VRSEL_1 ;             // VR+ = VREF buffered, VR- = Vss

    // Use P1.2
    P1SEL1 |= BIT2;                         // Configure P1.2 for ADC
    P1SEL0 |= BIT2;
    ADC12MCTL0 = ADC12INCH_2  |             // Select ch A2 at P1.2
                 ADC12VRSEL_1 ;             // VR+ = VREF buffered, VR- = Vss

    ADC12IER0 = ADC12IE0;                   // Enable ADC conv complete interrupt
}

// ADC12 interrupt service routine
void __attribute__ ((interrupt(ADC12_B_VECTOR))) ADC12_ISR (void) {
    switch(__even_in_range(ADC12IV, ADC12IV__ADC12RDYIFG))
    {
        case ADC12IV__NONE:        break;   // Vector  0:  No interrupt
        case ADC12IV__ADC12OVIFG:  break;   // Vector  2:  ADC12MEMx Overflow
        case ADC12IV__ADC12TOVIFG: break;   // Vector  4:  Conversion time overflow
        case ADC12IV__ADC12HIIFG:  break;   // Vector  6:  ADC12BHI
        case ADC12IV__ADC12LOIFG:  break;   // Vector  8:  ADC12BLO
        case ADC12IV__ADC12INIFG:  break;   // Vector 10:  ADC12BIN
        case ADC12IV__ADC12IFG0:            // Vector 12:  ADC12MEM0
            adc_reading = ADC12MEM0;
            __bic_SR_register_on_exit(LPM0_bits);
            break;   
        case ADC12IV__ADC12IFG1:   break;   // Vector 14:  ADC12MEM1
        case ADC12IV__ADC12IFG2:   break;   // Vector 16:  ADC12MEM2
        case ADC12IV__ADC12IFG3:   break;   // Vector 18:  ADC12MEM3
        case ADC12IV__ADC12IFG4:   break;   // Vector 20:  ADC12MEM4
        case ADC12IV__ADC12IFG5:   break;   // Vector 22:  ADC12MEM5
        case ADC12IV__ADC12IFG6:   break;   // Vector 24:  ADC12MEM6
        case ADC12IV__ADC12IFG7:   break;   // Vector 26:  ADC12MEM7
        case ADC12IV__ADC12IFG8:   break;   // Vector 28:  ADC12MEM8
        case ADC12IV__ADC12IFG9:   break;   // Vector 30:  ADC12MEM9
        case ADC12IV__ADC12IFG10:  break;   // Vector 32:  ADC12MEM10
        case ADC12IV__ADC12IFG11:  break;   // Vector 34:  ADC12MEM11
        case ADC12IV__ADC12IFG12:  break;   // Vector 36:  ADC12MEM12
        case ADC12IV__ADC12IFG13:  break;   // Vector 38:  ADC12MEM13
        case ADC12IV__ADC12IFG14:  break;   // Vector 40:  ADC12MEM14
        case ADC12IV__ADC12IFG15:  break;   // Vector 42:  ADC12MEM15
        case ADC12IV__ADC12IFG16:  break;   // Vector 44:  ADC12MEM16
        case ADC12IV__ADC12IFG17:  break;   // Vector 46:  ADC12MEM17
        case ADC12IV__ADC12IFG18:  break;   // Vector 48:  ADC12MEM18
        case ADC12IV__ADC12IFG19:  break;   // Vector 50:  ADC12MEM19
        case ADC12IV__ADC12IFG20:  break;   // Vector 52:  ADC12MEM20
        case ADC12IV__ADC12IFG21:  break;   // Vector 54:  ADC12MEM21
        case ADC12IV__ADC12IFG22:  break;   // Vector 56:  ADC12MEM22
        case ADC12IV__ADC12IFG23:  break;   // Vector 58:  ADC12MEM23
        case ADC12IV__ADC12IFG24:  break;   // Vector 60:  ADC12MEM24
        case ADC12IV__ADC12IFG25:  break;   // Vector 62:  ADC12MEM25
        case ADC12IV__ADC12IFG26:  break;   // Vector 64:  ADC12MEM26
        case ADC12IV__ADC12IFG27:  break;   // Vector 66:  ADC12MEM27
        case ADC12IV__ADC12IFG28:  break;   // Vector 68:  ADC12MEM28
        case ADC12IV__ADC12IFG29:  break;   // Vector 70:  ADC12MEM29
        case ADC12IV__ADC12IFG30:  break;   // Vector 72:  ADC12MEM30
        case ADC12IV__ADC12IFG31:  break;   // Vector 74:  ADC12MEM31
        case ADC12IV__ADC12RDYIFG: break;   // Vector 76:  ADC12RDY
        default: break;
    }
}

void comp_init(void) {
    P1SEL1 |= BIT2;                         // P1.2 C2 for Vcompare
    P1SEL0 |= BIT2;

    // P3SEL1 |= BIT0;                         // P3.0 C12 for Vcompare
    // P3SEL0 |= BIT0;

    P1DIR  |= BIT1;                         // P1.1 COUT output direction
    P1SEL1 |= BIT1;                         // Select COUT function on P1.1/COUT

    // REFCTL0 |= REFVSEL_1;

    // Setup Comparator_E
    // CECTL0 = CEIPEN | CEIPSEL_12;             // Enable V+, input channel C12
    CECTL0 = CEIPEN | CEIPSEL_2;            // Enable V+, input channel C2

    CECTL1 = CEPWRMD_1|                       // Normal power mode
             CEF      |
             CEFDLY_3 ;
    // CECTL1 = CEMRVS   |                     // CMRVL selects the refV - VREF0
    //          CEPWRMD_1|                     // Normal power mode / Ultra-low power mode
    //          CEF      |                     // Output filter enabled            
    //          CEFDLY_3 ;                     // Output filter delay 3600 ns
                                            
    CECTL2 = CEREFL_2 |                     // VREF 2.0 V is selected
             CERS_2   |                     // VREF applied to R-ladder
             CERSEL   |                     // to -terminal
             CEREF0_20|
             CEREF1_16;   

    // CECTL3 = CEPD12   ;                     // Input Buffer Disable @P3.0/C12
    CECTL3 = CEPD2;                         // C2

    CEINT  = CEIE     |                     // Interrupt enabled
             CEIIE    ;                     // Inverted interrupt enabled
    //          CERDYIE  ;                     // Ready interrupt enabled

    CECTL1 |= CEON;                         // Turn On Comparator_E
}

// Comparator E ISR
void __attribute__ ((interrupt(COMP_E_VECTOR))) Comp_ISR (void) {
    switch(__even_in_range(CEIV, CEIV_CERDYIFG)) {
        case CEIV_NONE: break;
        case CEIV_CEIFG:
            CEINT &= ~CEIFG;
            // P1OUT |= BIT4;
            __bic_SR_register_on_exit(LPM4_bits);
            break;
        case CEIV_CEIIFG:
            CEINT &= ~CEIIFG;
            // P1OUT &= ~BIT4;
            __bis_SR_register_on_exit(LPM4_bits | GIE);
            break;
        case CEIV_CERDYIFG:
            CEINT &= ~CERDYIFG;
            break;
        default: break;
    }
}

void atom_func_start(void) {
    if (check_fail) need_calibrate = 1;

    if (need_calibrate) {
        // wait for the highest voltage, assumed to be 3.6 V
        // CECTL1 &= ~CEON;
        CECTL2 &= ~CEREF0;
        CECTL2 |= CEREF0_27; // set VREF0 = 28/32 Vref, 3.5V with 2V ref and 1/2 voltage divider
        CECTL1 |= CEMRVS;
        // CECTL1 |= CEON;
        __bis_SR_register(LPM4_bits | GIE);
        CECTL1 &= ~CEMRVS;

        // calibrate
        P1OUT |= BIT3; // short-circuit the supply
        ADC12CTL0 |= ADC12ENC | ADC12SC; // Start sampling & conversion
        __bis_SR_register(LPM0_bits | GIE);
        adc_r1 = adc_reading;
    } else {
        CECTL2 &= ~CEREF0;
        CECTL2 |= (uint16_t) resume_threshold;
        CECTL1 |= CEMRVS;
        __bis_SR_register(LPM4_bits | GIE);
        CECTL1 &= ~CEMRVS;
    }

    check_fail = 1; // this is reset at the end of atomic function; if failed, this is still set on reboots

    P1OUT |= BIT4; // for debugging, indicating function starts
}

void atom_func_end(void) {
    P1OUT &= ~BIT4; // for debugging, indicating function ends

    if (need_calibrate) {
        // calibrate
        ADC12CTL0 |= ADC12ENC | ADC12SC;    // Start sampling/conversion
        __bis_SR_register(LPM0_bits);
        adc_r2 = adc_reading;
        P1OUT &= ~BIT3; // reconnect the supply
        
        // check this type cast
        // check its conversion for energy / charges
        resume_threshold = (uint8_t) ((double) (adc_r1 - adc_r2) / 4095.0 * 32.0) + 1 + backup_threshold; 

        need_calibrate = 0;
    } 

    check_fail = 0;
    // else {
    //     if (!check_fail)
    // }
}