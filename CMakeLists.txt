# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.1)

# configs
set(MSP430_GCC_DIR "/opt/msp430-gcc-9.2.0")
set(DEVICE "msp430fr5994")
set(USB_PORT usbmodem1421401)

# set toolchain
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain.cmake)

project(OPTA)
enable_language(C ASM)

# set common include paths
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${MSP430_GCC_DIR}/include
)

add_subdirectory(debs)
add_subdirectory(opta)
add_subdirectory(dma)
add_subdirectory(aes)
add_subdirectory(radio)
add_subdirectory(uart)
