# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.1)

FOREACH(METHOD "opta" "debs" "samoyed")
    set(TARGET_NAME "radio-${METHOD}")
    add_executable(
        ${TARGET_NAME}
        main.c
        hal_spi_rf.c
        hal_spi_rf.h
        msp_nrf24.c
        msp_nrf24.h
        nRF24L01.h
    )
    include(${PROJECT_SOURCE_DIR}/cmake/app_compile_options.cmake)
ENDFOREACH()
