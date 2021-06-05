# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.1)

# configs
set(MSP430_GCC_DIR "/opt/msp430-gcc-9.2.0")
set(DEVICE "msp430fr5994")
set(USB_PORT usbmodem141101)

# set toolchain
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain.cmake)

project(IC-Atomic)
enable_language(C ASM)

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(aes)
add_subdirectory(test)
add_subdirectory(lib)