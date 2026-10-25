/**
 * @file dxl2.h
 * @brief Umbrella header of the DYNAMIXEL Protocol 2.0 library.
 *
 * Project: DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Author: Anton Vynohradov
 * Email: avynohradov@systemfromscratch.com
 *
 * @copyright Copyright (c) 2026 Anton Vynohradov
 * @license Apache License 2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup dxl2 DXL2 API
 * @brief Public API of the DYNAMIXEL Protocol 2.0 library.
 *
 * @details Include this single header to use the library:
 * @li dxl2_config.h   build-time configuration (buffer sizes)
 * @li dxl2_defs.h     instructions, error codes, IDs, little-endian helpers
 * @li dxl2_protocol.h CRC-16, incremental parser, encoder with byte stuffing
 * @li dxl2_link.h     transport-independent link (stream in, packets out)
 * @{
 */

#ifndef DXL2_H
#define DXL2_H

/** @name Includes */
/** @{ */

#include "dxl2_config.h"
#include "dxl2_defs.h"
#include "dxl2_protocol.h"
#include "dxl2_link.h"

/** @} */

/**
 * @name Library version
 * @details Normally injected by cmake/version.cmake from the Git tag; the
 *          values below apply to builds that do not use CMake.
 */
/** @{ */

#ifndef DXL2_VERSION_MAJOR
#define DXL2_VERSION_MAJOR 1
#endif

#ifndef DXL2_VERSION_MINOR
#define DXL2_VERSION_MINOR 0
#endif

#ifndef DXL2_VERSION_PATCH
#define DXL2_VERSION_PATCH 0
#endif

/** Version string "MAJOR.MINOR.PATCH". */
#ifndef DXL2_VERSION
#define DXL2_VERSION "1.0.0"
#endif

/** @} */

#endif /* DXL2_H */

/** @} */
