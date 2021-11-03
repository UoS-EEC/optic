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

#ifdef OPTA
#include "opta/ic.h"
#elif defined(DEBS)
#include "debs/ic.h"
#else
#error Specify a method.
#endif

uint8_t payload[32] = "Hiya Baobeie, how are you today?";
uint8_t addr[5] = "latte";

// Transmit a 32B payload through nrf24l01+
void radio_tx_payload(uint8_t* payload) {
    atom_func_start(RADIO_TX_PAYLOAD);
    /*** Write payload to Tx Fifo ***/
    // 1 level
    nrf24_w_payload(payload, 32);
    // 2 levels
    // nrf24_wr_payload(dummy_payload, 32);
    // 3 levels
    // nrf24_wr_payload(dummy_payload, 32);

    /*** Activate Tx ***/
    nrf24_power_up();
    // Set CE high for at least 10us (too short on this clone) to trigger Tx
    // Goes back to Standby-1 when everything sent
    RF_TRX_BEGIN();
    // If opearting at 8MHz...
    __delay_cycles(80);  // ~10us for 1 level (32B)
    // __delay_cycles(2400);  // ~300us for 2 levels (64B)
    // __delay_cycles(3600);  // ~450us for 3 levels (96B)
    RF_TRX_END();

    /*** Wait for Tx done ***/
    nrf24_wait_tx_done();
    // Clear power-up data sent flag
    nrf24_wr_reg(RF24_STATUS, RF24_TX_DS);

    /*** Power down ***/
    nrf24_wr_reg(RF24_CONFIG, RF24_EN_CRC);
    __delay_cycles(1600);
    atom_func_end(RADIO_TX_PAYLOAD);
}

int main(void) {
    nrf24_init();           // Radio init
    nrf24_w_tx_addr(addr);  // Set tx address
    nrf24_enable_irq();     // Enable radio IRQ

    for (;;) {
        radio_tx_payload(payload);
    }
}
