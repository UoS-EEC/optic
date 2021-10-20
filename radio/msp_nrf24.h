/*
 * Copyright (c) 2020-2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RADIO_MSP_NRF24_H_
#define RADIO_MSP_NRF24_H_

// P2.5 IRQ
#define     RF_IRQ_PORT_VECTOR     PORT2_VECTOR
#define     RF_IRQ_OUT             P2OUT
#define     RF_IRQ_DIR             P2DIR
#define     RF_IRQ_IN              P2IN
#define     RF_IRQ_REN             P2REN
#define     RF_IRQ_SEL0            P2SEL0
#define     RF_IRQ_SEL1            P2SEL1
#define     RF_IRQ_PxIES           P2IES
#define     RF_IRQ_PxIFG           P2IFG
#define     RF_IRQ_PxIE            P2IE
#define     RF_IRQ_PIN             BIT5

// P3.6 CE
#define     RF_CE_OUT              P3OUT
#define     RF_CE_DIR              P3DIR
#define     RF_CE_SEL0             P3SEL0
#define     RF_CE_SEL1             P3SEL1
#define     RF_CE_PIN              BIT6


#define RF_TRX_BEGIN()              st(RF_CE_OUT |= RF_CE_PIN;)
#define RF_TRX_END()                st(RF_CE_OUT &= ~RF_CE_PIN;)

uint8_t nrf24_wr_reg(uint8_t address, uint8_t data);
uint8_t nrf24_rd_reg(uint8_t address, uint8_t* data);
uint8_t nrf24_cmd(uint8_t cmd);
uint8_t nrf24_w_payload(uint8_t* payload, uint8_t size);
void nrf24_ce_irq_pins_init();
void nrf24_init();
void nrf24_wait_tx_done();
void nrf24_enable_irq();
void nrf24_disable_irq();
void nrf24_power_up();
void nrf24_w_tx_addr(uint8_t* addr);
void nrf24_w_rx_addr_p0(uint8_t *addr);
void nrf24_open_pipe(uint8_t pipeid);
uint8_t nrf24_r_rx_payload(uint8_t *data, uint8_t len);

#endif  // RADIO_MSP_NRF24_H_
