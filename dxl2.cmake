################################################################################
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: Library target definition. Can be included directly by an
#              integrating project: include(path/to/dxl2.cmake)
#
# Author:      Anton Vynohradov
# Email:       avynohradov@systemfromscratch.com
# License:     Apache License 2.0
# Copyright:   (c) 2026 Anton Vynohradov
#
# References:
#   - DYNAMIXEL Protocol 2.0: https://docs.robotis.com/docs/dxl/protocol/protocol2/
#   - MISRA C:2012 Guidelines for the use of the C language in critical systems
#   - CMake Documentation: https://cmake.org/documentation/
#
# Last Updated: October 2026
# SPDX-License-Identifier: Apache-2.0
#
################################################################################

# ==============================================================================
# CMAKE CONFIGURATION
# ==============================================================================

list(APPEND CMAKE_MODULE_PATH

    ${CMAKE_CURRENT_LIST_DIR}/cmake
)

# ==============================================================================
# VERSION EXTRACTION
# ==============================================================================

include(version) # Include the version extraction and parsing logic

# ==============================================================================
# INCLUDE DIRECTORIES
# ==============================================================================

list(APPEND DXL2_HEADERS

    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

# ==============================================================================
# SOURCE FILES
# ==============================================================================

list(APPEND DXL2_SOURCES

    ${CMAKE_CURRENT_LIST_DIR}/src/dxl2_protocol.c
    ${CMAKE_CURRENT_LIST_DIR}/src/dxl2_link.c
)

set_source_files_properties(${DXL2_SOURCES} PROPERTIES LANGUAGE C)

# ==============================================================================
# PROJECT CONFIGURATION
# ==============================================================================

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND DXL2_COMPILE_DEFINITIONS "DXL2_DEBUG")
endif()

set(DXL2_LIBRARY_TYPE "STATIC")        # Default to static library, can be overridden by CMake presets
set(DXL2_LIBRARY_INTERFACE "PUBLIC")   # Interface scope for consumers of the library

# ==============================================================================
# UNIT TESTS
# ==============================================================================

if(DXL2_UNIT_TESTS)

    list(REMOVE_ITEM DXL2_SOURCES
        # Add any source files that should be excluded from the main library when unit tests are enabled
    )

    list(APPEND DXL2_UNIT_TEST_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/tests/unit/gtest_dxl2_crc16.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tests/unit/gtest_dxl2_parser.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tests/unit/gtest_dxl2_encode.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tests/unit/gtest_dxl2_defs.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tests/unit/gtest_dxl2_link_init.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tests/unit/gtest_dxl2_link_receive.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tests/unit/gtest_dxl2_link_send.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tests/unit/dxl2_test_mocks.cpp
    )

    list(APPEND DXL2_UNIT_TEST_HEADERS
        ${CMAKE_CURRENT_LIST_DIR}/tests/unit
    )

    list(APPEND DXL2_HEADERS ${DXL2_UNIT_TEST_HEADERS})
    list(APPEND DXL2_SOURCES ${DXL2_UNIT_TEST_SOURCES})

    set(DXL2_LIBRARY_TYPE "INTERFACE")         # Sources are compiled into the test executable (coverage)
    set(DXL2_LIBRARY_INTERFACE "INTERFACE")    # Consumers (the test executable) link against this target

endif()

# ==============================================================================
# LIBRARY TARGET
# ==============================================================================

set(DXL2_C "dxl2")
add_library(${DXL2_C} ${DXL2_LIBRARY_TYPE} ${DXL2_SOURCES})
target_include_directories(${DXL2_C} ${DXL2_LIBRARY_INTERFACE} ${DXL2_HEADERS})
target_compile_definitions(${DXL2_C} ${DXL2_LIBRARY_INTERFACE} ${DXL2_COMPILE_DEFINITIONS})

if(DXL2_COMPILE_OPTIONS) # Add any additional compile options if defined
    target_compile_options(${DXL2_C} ${DXL2_LIBRARY_INTERFACE} ${DXL2_COMPILE_OPTIONS})
endif()
