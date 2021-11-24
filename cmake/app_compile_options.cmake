# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".elf")

target_compile_options(
    ${TARGET_NAME}
    PRIVATE -std=c99
    PRIVATE -mmcu=${DEVICE} 
    PRIVATE -O0
    PRIVATE -msmall
    # PRIVATE -g                              # Generate gdb debug info
    PRIVATE -mhwmult=f5series
    PRIVATE -Wall
    PRIVATE -fno-zero-initialized-in-bss    # We don't want to zero out whole bss on every boot
    PRIVATE -fdata-sections                 # Separate data from code
    PRIVATE -ffunction-sections             # Separate code into translation units
)

# linker
target_link_directories(
    ${TARGET_NAME} 
    PRIVATE ${MSP430_GCC_DIR}/include
    PRIVATE ${MSP430_GCC_DIR}/msp430-elf/lib/
    PRIVATE ${MSP430_GCC_DIR}/lib/gcc/msp430-elf/9.2.0
)


IF(${METHOD} STREQUAL "opta")
    target_link_libraries(
        ${TARGET_NAME}
        PRIVATE opta
        PRIVATE mul_f5
    )
ELSEIF(${METHOD} STREQUAL "debs")
    target_link_libraries(
        ${TARGET_NAME}
        PRIVATE debs
        PRIVATE mul_f5
    )
ENDIF()


target_link_options(
    ${TARGET_NAME}
    # PRIVATE -T ${MSP430_GCC_DIR}/include/${DEVICE}.ld   # msp430 original linker command file
    PRIVATE -T ${PROJECT_SOURCE_DIR}/lib/${DEVICE}.ld
    PRIVATE -T ${MSP430_GCC_DIR}/include/${DEVICE}_symbols.ld
    PRIVATE -nostartfiles
    PRIVATE -Wl,--gc-sections               # Discard unused functions
)

add_upload(${TARGET_NAME})
