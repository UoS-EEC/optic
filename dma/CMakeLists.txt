# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.1)

FOREACH(METHOD "opta" "debs")
    set(TARGET_NAME "dma-${METHOD}")
    add_executable(
        ${TARGET_NAME}
        main.c
        dma.c
        dma.h
    )
    include(${PROJECT_SOURCE_DIR}/cmake/app_compile_options.cmake)
ENDFOREACH()
