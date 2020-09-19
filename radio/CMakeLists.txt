#
# Copyright (c) 2019-2020, University of Southampton and Contributors.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# cmake_minimum_required(VERSION 3.13)
# set(CODESEC F)
# set(DATASEC S)
# set(WS 0)
# set(TESTNAME nrf24)
# set(SOURCES 
#   main.c 
#   eusci_b_spi.c 
#   eusci_b_spi.h
#   nRF24L01.h
#   gpio.c
#   gpio.h
#   )
# add_executable(${TESTNAME} ${SOURCES})
# include(../../../cmake/tail.cmake)




cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME "radio")
set(MSP430_GCC_DIR "/opt/msp430-gcc")
set(DEVICE "msp430fr5994")

add_executable(
    ${TARGET_NAME}
    main.c
    eusci_b_spi.c 
    eusci_b_spi.h
    nRF24L01.h
    gpio.c
    gpio.h
    ic.c
    ic.h
)

set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".elf")

target_include_directories(${TARGET_NAME} PRIVATE ${MSP430_GCC_DIR}/include)
# target_include_directories(one PUBLIC something)

target_compile_options(
    ${TARGET_NAME}
    PRIVATE -DMSP430_ARCH
    PRIVATE -std=c99
    PRIVATE -mcpu=msp430
    # PRIVATE -mmcu=${DEVICE} 
    # PRIVATE -O2
    PRIVATE -msmall
    PRIVATE -g 
    # PRIVATE -mhwmult=none
    PRIVATE -mhwmult=f5series
    # PRIVATE -fno-common
    PRIVATE -Wall
    PRIVATE -fno-zero-initialized-in-bss # We don't want to zero out whole bss on every boot
    PRIVATE -DNDEBUG
)

# linker
target_link_directories(
    ${TARGET_NAME} 
    PRIVATE ${MSP430_GCC_DIR}/include
    PRIVATE ${MSP430_GCC_DIR}/msp430-elf/lib/430
    PRIVATE ${MSP430_GCC_DIR}/lib/gcc/msp430-elf/7.3.2/430
)
target_link_options(
    ${TARGET_NAME}
    # PRIVATE -T ${MSP430_GCC_DIR}/include/${DEVICE}.ld
    PRIVATE -T ${CMAKE_CURRENT_LIST_DIR}/../${DEVICE}.ld
    PRIVATE -T ${MSP430_GCC_DIR}/include/${DEVICE}_symbols.ld
    # PRIVATE -nostdlib
    # PRIVATE -ffreestanding
    PRIVATE -nostartfiles
)

add_upload(${TARGET_NAME})