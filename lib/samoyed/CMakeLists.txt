# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME "samoyed")

add_library(
    ${TARGET_NAME}
    samoyed.c
    config.h
    ${PROJECT_SOURCE_DIR}/lib/ips.h
)

include(${PROJECT_SOURCE_DIR}/cmake/lib_compile_options.cmake)