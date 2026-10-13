################################################################################
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: Extracts the library version from the latest Git tag (vX.Y.Z)
#              and exposes it as DXL2_VERSION_* compile definitions.
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
# DEFAULT VERSION
# ==============================================================================

# Used when the source tree has no Git metadata or no vX.Y.Z tag yet; keep it in
# sync with the fallback values in dxl2.h.
set(DXL2_DEFAULT_VERSION "1.0.0")

# ==============================================================================
# VERSION EXTRACTION
# ==============================================================================

find_package(Git QUIET)

set(DXL2_GIT_TAG "")
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
        RESULT_VARIABLE DXL2_GIT_DESCRIBE_RESULT
        ERROR_VARIABLE DXL2_GIT_DESCRIBE_ERROR
        OUTPUT_VARIABLE DXL2_GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )
endif()

# ==============================================================================
# VERSION PARSING
# ==============================================================================

string(REGEX MATCH "^[Vv]([0-9]+)\\.([0-9]+)\\.([0-9]+)$" DXL2_VERSION_MATCH "${DXL2_GIT_TAG}")

if(NOT DXL2_VERSION_MATCH)
    message(WARNING "No vX.Y.Z Git tag found (${DXL2_GIT_DESCRIBE_ERROR}); "
                    "using default version ${DXL2_DEFAULT_VERSION}")
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" DXL2_VERSION_MATCH "${DXL2_DEFAULT_VERSION}")
endif()

# ==============================================================================
# VERSION COMPONENTS
# ==============================================================================

set(DXL2_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(DXL2_VERSION_MINOR "${CMAKE_MATCH_2}")
set(DXL2_VERSION_PATCH "${CMAKE_MATCH_3}")

set(DXL2_VERSION "${DXL2_VERSION_MAJOR}.${DXL2_VERSION_MINOR}.${DXL2_VERSION_PATCH}")

message(STATUS "DXL2 library is version: ${DXL2_VERSION}")

# ==============================================================================
# CONFIGURE VERSION DEFINITIONS
# ==============================================================================

list(APPEND DXL2_COMPILE_DEFINITIONS
    DXL2_VERSION_MAJOR=${DXL2_VERSION_MAJOR}
    DXL2_VERSION_MINOR=${DXL2_VERSION_MINOR}
    DXL2_VERSION_PATCH=${DXL2_VERSION_PATCH}
    DXL2_VERSION="${DXL2_VERSION}"
)
