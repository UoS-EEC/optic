/*
 * Copyright (c) 2020-2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <msp430.h>
#include <stdint.h>

#include "nRF24L01.h"

#include "radio/hal_spi_rf.h"

void nrf24_spi_init(void) {
    // configure RF_MISO_PIN, RF_MOSI_PIN
    RF_PORT_SEL0 |=   RF_MISO_PIN | RF_MOSI_PIN;
    RF_PORT_SEL1 &= ~(RF_MISO_PIN | RF_MOSI_PIN);

    // configure RF_SCLK_PIN
    RF_SCLK_PORT_SEL0 |=  RF_SCLK_PIN;
    RF_SCLK_PORT_SEL1 &= ~RF_SCLK_PIN;

    // configure RF_CS_N_PIN
    RF_CS_N_PORT_SEL0 &= ~RF_CS_N_PIN;
    RF_CS_N_PORT_SEL1 &= ~RF_CS_N_PIN;
    RF_CS_N_PORT_DIR  |=  RF_CS_N_PIN;
    RF_CS_N_PORT_OUT  |=  RF_CS_N_PIN;

    /* Keep peripheral in reset state*/
    UCB1CTL1 |= UCSWRST;

    /* Configuration
     * -  8-bit
     * -  Master Mode
     * -  3-pin, active low
     * -  synchronous mode
     * -  MSB first
     * -  Clock phase select = captured on first edge
     * -  SMCLK as clock source
     * -  Spi clk is adjusted corresponding to systemClock as the highest rate
     *    supported by the supported radios: this could be optimized and done
     *    after chip detect.
     */
    UCB1CTL0 = (UCMST | UCSYNC | UCMSB | UCCKPH) >> 8;
    UCB1CTL1 |= UCSSEL_2;

    // f_BitClock = f_BRClock / prescalerValue
    UCB1BR1 = 0x00;
    UCB1BR0 = 0x08;

    /* Release for operation */
    UCB1CTL1 &= ~UCSWRST;
}

uint8_t nrf24_spi_transaction(uint8_t data) {
    UCB1TXBUF = data;
    while (!(UCB1IFG & UCRXIFG)) {}
    return UCB1RXBUF;
}

