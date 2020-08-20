cmake_minimum_required(VERSION 3.1)

# configs
set(MSP430_GCC_DIR "/opt/msp430-gcc")
set(DEVICE "msp430fr5994")

# set toolchain
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain.cmake)

project(IC-Atomic)
enable_language(C ASM)

add_subdirectory(aes)