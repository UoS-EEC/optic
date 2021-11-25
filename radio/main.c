/*
 * Copyright (c) 2020-2021, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

#include "radio/hal_spi_rf.h"
#include "radio/msp_nrf24.h"
#include "radio/nRF24L01.h"

#include "lib/ips.h"

uint8_t payload[32] = "Hiya Baobeie, how are you today?";
uint8_t addr[5] = "latte";

// Transmit a payload (<= 96B) through nrf24l01+
void radio_tx_payload(uint8_t* payload, uint8_t length) {
    // atom_func_start(RADIO_TX_PAYLOAD);
    atom_func_start_linear(RADIO_TX_PAYLOAD, length);

    // Write payload to TX fifo
    uint8_t length_copy = length;
    while (length_copy--) {
        nrf24_w_payload(payload, 16);
    }

    nrf24_power_up();                           // Power up

    RF_TRX_BEGIN();
    nrf24_wait_tx_done();
    RF_TRX_END();

    nrf24_wr_reg(RF24_STATUS, RF24_TX_DS);      // Clear power-up data sent flag
    nrf24_wr_reg(RF24_CONFIG, RF24_EN_CRC);     // Power down
    __delay_cycles(1600);

    // atom_func_end(RADIO_TX_PAYLOAD);
    atom_func_end_linear(RADIO_TX_PAYLOAD, length);
}

uint8_t __attribute__((section(".persistent"))) i = 6;

int main(void) {
    nrf24_init();           // Radio init
    nrf24_w_tx_addr(addr);  // Set tx address
    nrf24_enable_irq();     // Enable radio IRQ

    for (;;) {
        radio_tx_payload(payload, i);
        if (--i == 0) {
            i = 6;
        }
    }
}
