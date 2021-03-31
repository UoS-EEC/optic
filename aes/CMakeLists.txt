#
# Copyright (c) 2020, University of Southampton and Contributors.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME "aes")

add_executable(
    ${TARGET_NAME}
    main.c
    aesa.c
    aesa.h
    ic.c
    ic.h
)

set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".elf")

target_include_directories(${TARGET_NAME} PRIVATE ${MSP430_GCC_DIR}/include)

target_compile_options(
    ${TARGET_NAME}
    PRIVATE -std=c99
    PRIVATE -mmcu=${DEVICE} 
    # PRIVATE -O2
    PRIVATE -msmall
    PRIVATE -g 
    # PRIVATE -mhwmult=none
    PRIVATE -mhwmult=f5series
    PRIVATE -Wall
    PRIVATE -fno-zero-initialized-in-bss  # We don't want to zero out whole bss on every boot
)

# linker
target_link_directories(
    ${TARGET_NAME} 
    PRIVATE ${MSP430_GCC_DIR}/include
    PRIVATE ${MSP430_GCC_DIR}/msp430-elf/lib/
    PRIVATE ${MSP430_GCC_DIR}/lib/gcc/msp430-elf/9.2.0
)
target_link_options(
    ${TARGET_NAME}
    # PRIVATE -T ${MSP430_GCC_DIR}/include/${DEVICE}.ld  # If you want to include msp430 original linker command files
    PRIVATE -T ${PROJECT_SOURCE_DIR}/support/${DEVICE}.ld
    PRIVATE -T ${MSP430_GCC_DIR}/include/${DEVICE}_symbols.ld
    # PRIVATE -nostdlib  # Include -nodefaultlibs and -nostartfiles
    # PRIVATE -nodefaultlibs
    PRIVATE -nostartfiles
)

add_upload(${TARGET_NAME})