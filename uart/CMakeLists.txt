# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.1)

FOREACH(METHOD "opta" "debs")
    set(TARGET_NAME "uart-${METHOD}")
    add_executable(
        ${TARGET_NAME}
        main.c
    )
    include(${PROJECT_SOURCE_DIR}/cmake/app_compile_options.cmake)
ENDFOREACH()
