/*
 * Copyright (c) 2019-2020, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* ------ Includes ----------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <msp430fr5994.h>

#include "eusci_b_spi.h"
#include "gpio.h"
#include "nRF24L01.h"

#include "ic.h"

#define ASSERT

/* ------ Types -------------------------------------------------------------*/
struct nrfReadResult {
  uint8_t status;
  uint8_t response;
};
/* ------ Global Variables --------------------------------------------------*/
volatile static uint8_t TXData = 0x00;
volatile static uint8_t RXData = 0x00;
volatile static uint8_t ticks = 0;
uint8_t dummy_payload[32] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                             0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                             0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                             0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

/* ------ Function Prototypes -----------------------------------------------*/

// Initialize GPIO settings
static void radio_gpio_init();

// Transmit data, and get response
uint8_t spiTransaction(const uint8_t data);

// Send a command to the nrf24 and record the status register and response
static void nrf24Command(struct nrfReadResult* res, const uint8_t address,
                         const uint8_t command);

// Read the status register and a byte of data
static void nrf24ReadByte(struct nrfReadResult* res, const uint8_t address);

static void nrf24WritePayload(struct nrfReadResult* res, uint8_t* payload, uint8_t size);

// Assert c==true, otherwise indicate test fail and stall.
void assert(bool c);

/* ------ Function Definitions ----------------------------------------------*/

uint8_t spiTransaction(const uint8_t data) {
  EUSCI_B_SPI_transmitData(EUSCI_B1_BASE, data);
  while (!EUSCI_B_SPI_getInterruptStatus(EUSCI_B1_BASE,
                                         EUSCI_B_SPI_RECEIVE_INTERRUPT))
    ;
  uint8_t result = EUSCI_B_SPI_receiveData(EUSCI_B1_BASE);
  return result;
}

static void nrf24ReadByte(struct nrfReadResult* res, const uint8_t address) {
  GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN5);
  res->status = spiTransaction(address | RF24_R_REGISTER);
  res->response = spiTransaction(RF24_NOP);
  GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN5);
}

static void nrf24Command(struct nrfReadResult* res, const uint8_t address,
                         const uint8_t command) {
  GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN5);
  res->status = spiTransaction(address | RF24_W_REGISTER);
  res->response = spiTransaction(command);
  GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN5);
}

static void nrf24WritePayload(struct nrfReadResult* res, uint8_t* payload, uint8_t size) {
  GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN5);
  res->status = spiTransaction(RF24_W_TX_PAYLOAD);
  while (size--) spiTransaction(*payload++);
  GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN5);
}


// __attribute__((optimize(0))) void cs_init() {
//   CSCTL0_H = 0xA5;            // Unlock CS
//   // FRCTL0_H = 0xA5;            // Unlock FRAM ctrl
//   // FRCTL0_L = FRAM_WAIT << 4;  // wait states

//   CSCTL1 = DCORSEL | DCOFSEL_4;  // 16 MHz

//   // Set ACLK = VLO; MCLK = DCO; SMCLK = DCO
//   CSCTL2 = SELA_1 + SELS_3 + SELM_3;

//   // ACLK, SMCLK, MCLK dividers
//   CSCTL3 = DIVA_0 + DIVS_1 + DIVM_1;

//   CSCTL0_H = 0x01; // Lock Register
// }

// void gpio_init() {
//   // Need to initialize all ports/pins to reduce power consump'n
//   P1OUT = 0;  // LEDs on P1.0 and P1.1
//   P1DIR = 0xff;
//   P2OUT = 0;
//   P2DIR = 0xff;
//   P3OUT = 0;
//   P3DIR = 0xff;
//   P4OUT = BIT0;  // Pull-up on board
//   P4DIR = 0xff;
//   P6OUT = 0;
//   P6DIR = 0xff;
//   P7OUT = 0;
//   P7DIR = 0xff;
//   P8OUT = 0;
//   P8DIR = 0xff;
//   PM5CTL0 &= ~LOCKLPM5;
// }

// void target_init() {
//   cs_init();
//   gpio_init();
// }

#ifdef ASSERT
void indicate_test_fail() { P1OUT |= BIT1 | BIT2; }

void assert(bool c) {
  if (!c) {
    indicate_test_fail();
    while (1);  // Stall
  }
}
#endif

static void radio_gpio_init() {
  /* ------ Setup I/O ------ */
  /* Select Port 5
   * Set Pin 0,1,2 (C0,C1,C2) to Primary Module Function,
   * UCB1MOSI, UCB1MISO, UCB1CLK.
   */
  GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5,
                                             GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2,
                                             GPIO_PRIMARY_MODULE_FUNCTION);

  // Select Port 3
  // Set Pin 5,6 (B5,B6) as Output
  // CSN, CE
  GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN5 + GPIO_PIN6);

  // De-assert chip selects
  // GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);  // BME280
  GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN5);  // NRF24

  // Select Port
  // Set Pin 5 (A12) as Input
  // IRQ
  GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN5);

  // Expose SMCLK at P3.4 (B4)
  GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P3, GPIO_PIN4,
                                              GPIO_SECONDARY_MODULE_FUNCTION);
}

int main(void) {
  /* Initialise Clock System and GPIO
   * DCO = 16 MHz; VLO = ~10 kHz
   * ACLK = VLO; MCLK = DCO/2; SMCLK = DCO/2
   * All Ports Output Low
   * Unlock LPM5 Hi-Z
   */
  // target_init();
  // cs_init();
  radio_gpio_init();

  struct nrfReadResult result;

  /* ------ Setup SPI ------ */
  // Initialize SPI as Master
  EUSCI_B_SPI_initMasterParam param = {0};
  param.selectClockSource = EUSCI_B_SPI_CLOCKSOURCE_SMCLK;
  param.clockSourceFrequency = 8000000;
  param.desiredSpiClock = 1000000;  // 1 Mbps (max. 8 Mbps for radio)
  param.msbFirst = EUSCI_B_SPI_MSB_FIRST;
  param.clockPhase = EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
  param.clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
  param.spiMode = EUSCI_B_SPI_3PIN;
  EUSCI_B_SPI_initMaster(EUSCI_B1_BASE, &param);
  EUSCI_B_SPI_enable(EUSCI_B1_BASE);

  /*EUSCI_B_SPI_enableInterrupt(EUSCI_B1_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT);*/
  /*__enable_interrupt();*/

  /* ------ Test nrf24: Power Down -> Standby-I ------ */
  // Radio should be in Power Down in 10.3ms after VDD >= 1.9V
  // Wait for 10.3ms (10.3 / 1K * 8M)
  __delay_cycles(82400);

  // Flush TX FIFO
  GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN5);
  spiTransaction(RF24_FLUSH_TX);
  GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN5);

  // clear power-up data sent flag, write 1 to clear
  // 0x0E default
  nrf24Command(&result, RF24_STATUS, RF24_TX_DS | 0x0E);

  // clear PWR_UP, 0x08 default CONFIG state
  nrf24Command(&result, RF24_CONFIG, 0x08);

  // Read the STATUS and CONFIG registers
  nrf24ReadByte(&result, RF24_CONFIG);
  assert(result.status == 0b00001110);    // STATUS should be 0b00001110
  assert(result.response == 0b00001000);  // CONFIG should be 0b00001000

  // Power Down -> Standby-I
  nrf24Command(&result, RF24_CONFIG, RF24_EN_CRC | RF24_PWR_UP);
  assert(result.status == 0b00001110);
  assert(result.response == 0);

  // Should be in Standby-I in 1.5ms
  __delay_cycles(1300);
  // Power Down current draw is 900nA. (x100V/A = 90uV = 0.09 mV)
  // PD -> SI transient 285 uA (x100V/A = 28.5mV)
  // Standy-I current draw is 22 uA. (x100V/A = 2.2mV)
  // Read STATUS and CONFIG registers
  // Read the STATUS and CONFIG registers
  nrf24ReadByte(&result, RF24_CONFIG);
  assert(result.status == 0b00001110);    // STATUS should be 0b00001110
  assert(result.response == 0b00001010);  // CONFIG should be 0b00001010

  for (;;) {
    // atom_func_start(RADIO);

    /* ------ Test: Standby-I -> Tx Mode -> Standy-I ------ */
    // Clear power-up data sent flag
    // nrf24Command(&result, RF24_STATUS, RF24_TX_DS | 0x0E);
    nrf24Command(&result, RF24_STATUS, RF24_TX_DS);

    nrf24ReadByte(&result, RF24_FIFO_STATUS);
#ifdef ASSERT
    assert(result.status == 0x0E);
    assert(result.response == 0x11);
#endif

    // Power up
    nrf24Command(&result, RF24_CONFIG, RF24_EN_CRC | RF24_PWR_UP);
#ifdef ASSERT
    assert(result.status == 0b00001110);
    assert(result.response == 0);
#endif

    // Disable Enchanced Shockburst;
    // Clear auto acknowledge; 1Mbps; Disable retransmit;
    nrf24Command(&result, RF24_EN_AA, 0);  // Clear all auto ack
#ifdef ASSERT
    assert(result.status == 0b00001110);
    assert(result.response == 0);
#endif

    nrf24Command(&result, RF24_SETUP_RETR, 0);  // Disable retransmit
#ifdef ASSERT
    assert(result.status == 0b00001110);
    assert(result.response == 0);
#endif

//     nrf24Command(&result, RF24_RF_SETUP, 0b00000111);  // 1 Mbps
// #ifdef ASSERT
//     assert(result.status == 0b00001110);
//     assert(result.response == 0);
// #endif

    // Write payload to Tx Fifo
    nrf24WritePayload(&result, dummy_payload, 32);
    // nrf24Command(&result, RF24_W_TX_PAYLOAD, 0xAA);  // 1 byte dummy payload
    // nrf24Command(&result, RF24_W_TX_PAYLOAD, 0xAA);  // 1 byte dummy payload
    // nrf24Command(&result, RF24_W_TX_PAYLOAD, 0xAA);  // 1 byte dummy payload
    // nrf24Command(&result, RF24_W_TX_PAYLOAD, 0xAA);  // 1 byte dummy payload
    // nrf24Command(&result, RF24_W_TX_PAYLOAD, 0xAA);  // 1 byte dummy payload
    // nrf24Command(&result, RF24_W_TX_PAYLOAD, 0xAA);  // 1 byte dummy payload
    // nrf24Command(&result, RF24_W_TX_PAYLOAD, 0xAA);  // 1 byte dummy payload
    // nrf24Command(&result, RF24_W_TX_PAYLOAD, 0xAA);  // 1 byte dummy payload
    // nrf24Command(&result, RF24_W_TX_PAYLOAD, 0xAA);  // 1 byte dummy payload
#ifdef ASSERT
    assert(result.status == 0b00001110);
    assert(result.response == 0);
#endif

    // Set CE high for at least 10us (too short on this clone) to trigger Tx
    // Goes back to Standby-1 when everything sent
    GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
    __delay_cycles(60); // 100us 
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN6);
    // Air time:
    // Preamble 1 byte, Address 5 byte, Payload 1 byte, CRC 1 byte
    // 8 bytes * 8 bits/byte @ 1 Mbps = 64 us

    // atom_func_end(RADIO);

    // P1OUT |= BIT0;
    // __delay_cycles(400000);  // 0.5s
    // P1OUT &= ~BIT0;
    __delay_cycles(400000);  // 0.5s
  }
  

  /* ------ Test: Standby-I -> Rx Mode -> Standy-I ------ */
  // nrf24Command(&result, RF24_CONFIG, RF24_EN_CRC | RF24_PWR_UP | RF24_PRIM_RX);
  // assert(result.status == 0b00001110);
  // assert(result.response == 0);

  // // RX Mode when CE is HIGH
  // // Current draw at 2Mbps LNA-H is 12.3 mA (x100V/A = 1.23V)
  // // Measured 1.56V
  // GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
  // __delay_cycles(4000);  // 0.5ms
  // GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN6);

  // // Trigger Power Down
  // nrf24Command(&result, RF24_CONFIG, RF24_EN_CRC);
  // assert(result.status == 0b00001110);
  // assert(result.response == 0);

  /*__disable_interrupt();*/
  // end_experiment();
  // ^  P1.4 High
}

// USCI_B1 Interrupt Service Routine
/*__attribute__((interrupt(USCI_B1_VECTOR))) void USCI_B1_ISR(void) { ticks++;
 * }*/

