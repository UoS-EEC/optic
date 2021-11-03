/*
 * Copyright (c) 2020-2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <msp430.h>
#include <stdint.h>

#include "radio/hal_spi_rf.h"
#include "radio/nRF24L01.h"

#include "radio/msp_nrf24.h"

uint8_t nrf24_wr_reg(uint8_t address, uint8_t data) {
    RF_SPI_BEGIN();
    uint8_t status = nrf24_spi_transaction(RF24_W_REGISTER | address);
    nrf24_spi_transaction(data);
    RF_SPI_END();
    return status;
}

uint8_t nrf24_rd_reg(uint8_t address, uint8_t* data) {
    RF_SPI_BEGIN();
    uint8_t status = nrf24_spi_transaction(RF24_R_REGISTER | address);
    *data = nrf24_spi_transaction(RF24_NOP);
    RF_SPI_END();
    return status;
}

uint8_t nrf24_cmd(uint8_t cmd) {
    RF_SPI_BEGIN();
    uint8_t status = nrf24_spi_transaction(cmd);
    RF_SPI_END();
    return status;
}

uint8_t nrf24_w_payload(uint8_t* payload, uint8_t size) {
    RF_SPI_BEGIN();
    uint8_t status = nrf24_spi_transaction(RF24_W_TX_PAYLOAD);
    while (size--) {
        nrf24_spi_transaction(*payload++);
    }
    RF_SPI_END();
    return status;
}

void nrf24_ce_irq_pins_init() {
    // Configure IRQ as GPIO style input on the MSP430
    RF_IRQ_SEL0 &= ~RF_IRQ_PIN;
    RF_IRQ_SEL1 &= ~RF_IRQ_PIN;
    RF_IRQ_DIR  &= ~RF_IRQ_PIN;     // Input
    RF_IRQ_REN  &= ~RF_IRQ_PIN;     // Disable internal pullup/pulldown, using external pulldown
    // RF_IRQ_OUT  &= ~RF_IRQ_PIN;     // Don't care
    RF_IRQ_PxIES |=  RF_IRQ_PIN;    // Interrupt at High-to-Low transition
    // RF_IRQ_PxIFG = 0;               // Clear all pending interrupts at the port

    // Configure CE
    RF_CE_SEL0  &= ~RF_CE_PIN;
    RF_CE_SEL1  &= ~RF_CE_PIN;
    RF_CE_DIR   |=  RF_CE_PIN;
    RF_CE_OUT   &= ~RF_CE_PIN;
}

void nrf24_init() {
    // Radio should be in Power Down in 10.3ms after VDD >= 1.9V
    // Wait for 10.3ms (10.3 / 1K * 8M) of Power On Reset
    __delay_cycles(82400);

    // Clear all IRQ flags
    nrf24_wr_reg(RF24_STATUS, RF24_RX_DR | RF24_TX_DS | RF24_MAX_RT);
    // Close all pipes
    nrf24_wr_reg(RF24_EN_RXADDR, 0x00);     // Enable Rx data pipe 0 only
    nrf24_wr_reg(RF24_EN_AA, 0x00);         // Clear all auto acks

    nrf24_wr_reg(RF24_DYNPD, 0x00);         // Disable dyn. payload length
    nrf24_wr_reg(RF24_SETUP_RETR, 0x00);    // Disable auto retransmission

    // 2Mbps air data rate, 0dBm TX output power
    nrf24_wr_reg(RF24_RF_SETUP, 0x0F);
    // Set the channel to 120, i.e. 2520MHz
    nrf24_wr_reg(RF24_RF_CH, 120);
    // Set address width, 5 bytes
    nrf24_wr_reg(RF24_SETUP_AW, 0x03);
    // Set pipe 0 payload width
    nrf24_wr_reg(RF24_RX_PW_P0, 32);        // Pipe 0, 32 bytes
    // Power down, enable 1 byte CRC
    nrf24_wr_reg(RF24_CONFIG, RF24_EN_CRC);

    nrf24_cmd(RF24_FLUSH_TX);               // Flush TX FIFO
    nrf24_cmd(RF24_FLUSH_RX);               // Flush RX FIFO
}


void nrf24_wait_tx_done() {
    __bis_SR_register(LPM0_bits | GIE);
    // Wait for interrupt from RF_IRQ_PIN...
}

void nrf24_enable_irq() {
    RF_IRQ_PxIFG &= ~RF_IRQ_PIN;
    RF_IRQ_PxIE |=  RF_IRQ_PIN;
}

void nrf24_disable_irq() {
    RF_IRQ_PxIE &= ~RF_IRQ_PIN;
}

void __attribute__((interrupt(RF_IRQ_PORT_VECTOR))) RF_IRQ_PORT_ISR() {
    switch (__even_in_range(P2IV, P2IV__P2IFG7)) {
        case P2IV__NONE:    break;          // Vector  0:  No interrupt
        case P2IV__P2IFG0:  break;          // Vector  2:  P2.0 interrupt flag
        case P2IV__P2IFG1:  break;          // Vector  4:  P2.1 interrupt flag
        case P2IV__P2IFG2:  break;          // Vector  6:  P2.2 interrupt flag
        case P2IV__P2IFG3:  break;          // Vector  8:  P2.3 interrupt flag
        case P2IV__P2IFG4:  break;          // Vector  10:  P2.4 interrupt flag
        case P2IV__P2IFG5:                  // Vector  12:  P2.5 interrupt flag
            __bic_SR_register_on_exit(LPM0_bits);
            P2IFG &= ~BIT5;
            break;
        case P2IV__P2IFG6:  break;          // Vector  14:  P2.6 interrupt flag
        case P2IV__P2IFG7:  break;          // Vector  16:  P2.7 interrupt flag
        default: break;
    }
}


// Enable Standby-I, 26uA power draw
void nrf24_power_up() {
    nrf24_wr_reg(RF24_CONFIG, RF24_PWR_UP | RF24_EN_CRC);
    __delay_cycles(12000);  // Should be in Standby-I in 1.5ms
}

void nrf24_w_tx_addr(uint8_t* addr) {
    RF_SPI_BEGIN();
    nrf24_spi_transaction(RF24_TX_ADDR | RF24_W_REGISTER);
    for (int i = 4; i >= 0; --i) {
        nrf24_spi_transaction(addr[i]);
    }
    RF_SPI_END();
}

void nrf24_w_rx_addr_p0(uint8_t *addr) {
    RF_SPI_BEGIN();
    nrf24_spi_transaction(RF24_RX_ADDR_P0 | RF24_W_REGISTER);
    for (int i = 4; i >= 0; --i) {
        nrf24_spi_transaction(addr[i]);
    }
    RF_SPI_END();
}

void nrf24_open_pipe(uint8_t pipeid) {
    if (pipeid > 5)
        return;
    nrf24_wr_reg(RF24_EN_RXADDR, (1 << pipeid));
}

uint8_t nrf24_r_rx_payload(uint8_t *data, uint8_t len) {
    RF_SPI_BEGIN();
    uint8_t rf_status = nrf24_spi_transaction(RF24_R_RX_PAYLOAD);
    for (uint8_t i = 0; i < len; ++i) {
        data[i]  = nrf24_spi_transaction(RF24_NOP);
    }
    RF_SPI_END();
    // The RX pipe this data belongs to is stored in STATUS
    return ((rf_status & 0x0E) >> 1);
}
