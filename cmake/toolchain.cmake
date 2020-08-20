# skip CMake's check for a working compiler
include(CMakeForceCompiler)

# find toolchain
find_program(TC-GCC msp430-elf-gcc ${MSP430_GCC_DIR}/bin)
find_program(TC-GXX msp430-elf-g++ ${MSP430_GCC_DIR}/bin)
find_program(MSPDEBUG mspdebug)

# define toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_ASM_COMPILER ${TC-GCC} CACHE INTERNAL "")
set(CMAKE_C_COMPILER ${TC-GCC} CACHE INTERNAL "")
set(CMAKE_CXX_COMPIER ${TC-GXX} CACHE INTERNAL "")

# Prevent CMake from testing the compilers
set(CMAKE_C_COMPILER_WORKS 1 CACHE INTERNAL "")
set(CMAKE_CXX_COMPILER_WORKS 1 CACHE INTERNAL "")

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug type by default")


function(add_upload EXECUTABLE)
  add_custom_target(upload_${EXECUTABLE}
    COMMAND ${MSPDEBUG} tilib -v 3300 -d usbmodem1441401 "prog ${EXECUTABLE}.elf"
    DEPENDS ${EXECUTABLE})
endfunction(add_upload)
