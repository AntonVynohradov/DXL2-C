################################################################################
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: Host-buildable examples (DXL2_EXAMPLES=ON). The Zephyr example
#              is a Zephyr application and is built with west instead.
#
# Author:      Anton Vynohradov
# Email:       avynohradov@systemfromscratch.com
# License:     Apache License 2.0
# Copyright:   (c) 2026 Anton Vynohradov
#
# SPDX-License-Identifier: Apache-2.0
#
################################################################################

# ==============================================================================
# BARE METAL (host simulation of the board, runs as a test)
# ==============================================================================

add_executable(dxl2_bare_metal_sim
    bare_metal/main.c
    bare_metal/board_host.c
)
target_link_libraries(dxl2_bare_metal_sim PRIVATE dxl2)

add_test(NAME example_bare_metal COMMAND dxl2_bare_metal_sim)
set_tests_properties(example_bare_metal PROPERTIES LABELS "example")

# ==============================================================================
# HOST SERIAL PORT
# ==============================================================================

if(WIN32)
    set(DXL2_SERIAL_PORT_SOURCE host_serial/serial_port_win32.c)
else()
    set(DXL2_SERIAL_PORT_SOURCE host_serial/serial_port_posix.c)
endif()

add_executable(dxl2_ping
    host_serial/main.c
    ${DXL2_SERIAL_PORT_SOURCE}
)
target_link_libraries(dxl2_ping PRIVATE dxl2)
