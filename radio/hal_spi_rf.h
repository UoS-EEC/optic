/*
 * Copyright (c) 2020-2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RADIO_HAL_SPI_RF_H_
#define RADIO_HAL_SPI_RF_H_

/* Pins defined for msp430fr5994 */

/* Pin map
 * nrf24     - msp430fr5994
 * Pin1 GND  - GND
 * Pin2 VCC  - VCC
 * Pin3 CE   - P3.6
 * Pin4 CS_N - P3.5
 * Pin5 SCLK - P5.2
 * Pin6 MOSI - P5.0
 * Pin7 MISO - P5.1
 * Pin8 IRQ  - P2.5
 */

/* nrf24 SPI pins */
// P5.0 UCB1MOSI
// P5.1 UCB1MISO
#define     RF_PORT_SEL0           P5SEL0
#define     RF_PORT_SEL1           P5SEL1
#define     RF_PORT_OUT            P5OUT
#define     RF_PORT_DIR            P5DIR
#define     RF_PORT_IN             P5IN
#define     RF_PORT_REN            P5REN
#define     RF_MOSI_PIN            BIT0
#define     RF_MISO_PIN            BIT1

// P5.2 UCB1CLK
#define     RF_SCLK_PORT_SEL0      P5SEL0
#define     RF_SCLK_PORT_SEL1      P5SEL1
#define     RF_SCLK_PIN            BIT2

// P3.5 CS_N
#define     RF_CS_N_PORT_SEL0      P3SEL0
#define     RF_CS_N_PORT_SEL1      P3SEL1
#define     RF_CS_N_PORT_OUT       P3OUT
#define     RF_CS_N_PORT_DIR       P3DIR
#define     RF_CS_N_PIN            BIT5

#define st(x)      do { x } while (0)
#define RF_SPI_BEGIN()              st(RF_CS_N_PORT_OUT &= ~RF_CS_N_PIN;)
#define RF_SPI_END()                st(RF_CS_N_PORT_OUT |= RF_CS_N_PIN;)


/* PROTOTYPES */

void nrf24_spi_init(void);
uint8_t nrf24_spi_transaction(uint8_t data);


#endif  // RADIO_HAL_SPI_RF_H_
