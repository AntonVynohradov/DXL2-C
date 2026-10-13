################################################################################
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: Native part of the DXL2 GUI tool: links the static DXL2 library
#              (dxl2.cmake) into a shared library loaded by Python (ctypes).
#
# Author:      Anton Vynohradov
# Email:       avynohradov@systemfromscratch.com
# License:     Apache License 2.0
# Copyright:   (c) 2026 Anton Vynohradov
#
# Usage:
#   cmake -S tools/dxl2_gui/native -B tools/dxl2_gui/build -G "Unix Makefiles"
#   cmake --build tools/dxl2_gui/build
#
# Last Updated: October 2026
# SPDX-License-Identifier: Apache-2.0
#
################################################################################

cmake_minimum_required(VERSION 3.20)

project(DXL2_GUI_NATIVE
    DESCRIPTION "DXL2 GUI native bridge (simulator + host link)"
    LANGUAGES C
    )

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# The static library is linked into a shared object, so it must be position independent
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Only DXL2_TOOL_API functions are exported from the shared library
set(CMAKE_C_VISIBILITY_PRESET hidden)

# ==============================================================================
# STATIC DXL2 LIBRARY
# ==============================================================================

set(DXL2_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../..)
include(${DXL2_ROOT}/dxl2.cmake)

# ==============================================================================
# SHARED BRIDGE LIBRARY
# ==============================================================================

add_library(dxl2_gui_native SHARED
    ${CMAKE_CURRENT_LIST_DIR}/dxl2_sim.c
    ${CMAKE_CURRENT_LIST_DIR}/dxl2_host.c
)

target_include_directories(dxl2_gui_native PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(dxl2_gui_native PRIVATE dxl2)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dxl2_gui_native PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

if(MINGW)
    # No libgcc DLL dependency, so any 64-bit Python can load the library
    target_link_options(dxl2_gui_native PRIVATE -static-libgcc)
endif()

# Predictable file name next to the GUI: dxl2_gui_native.dll / libdxl2_gui_native.so
set_target_properties(dxl2_gui_native PROPERTIES
    PREFIX ""
    OUTPUT_NAME "dxl2_gui_native"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)
