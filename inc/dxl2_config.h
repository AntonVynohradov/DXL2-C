/**
 * @file dxl2_config.h
 * @brief DXL2 build-time configuration parameters.
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
 * @defgroup dxl2 DXL2 API
 * @brief Public API of the DYNAMIXEL Protocol 2.0 library.
 * @details This module groups the public interface and its supporting
 *          submodules: @ref dxl2_config, @ref dxl2_defs, @ref dxl2_protocol
 *          and @ref dxl2_link.
 */

/**
 * @defgroup dxl2_config Configuration
 * @ingroup dxl2
 * @brief Build-time configuration for DXL2.
 *
 * @details Every parameter can be overridden by the integrator with a compiler
 *          definition (for example `-DDXL2_MAX_PACKET_SIZE=1024U`) without
 *          editing this file.
 *
 * @par Configuration elements in this group:
 * @li @ref DXL2_MAX_PACKET_SIZE : largest wire packet the parser and encoder accept.
 * @{
 */

#ifndef DXL2_CONFIG_H
#define DXL2_CONFIG_H

/** @name Defines and macros */
/** @{ */

/**
 * @def DXL2_MAX_PACKET_SIZE
 * @brief Maximum complete wire-packet size, including header and CRC, in bytes.
 *
 * @details Sizes the parser buffer, the decoded packet and the link transmit
 *          buffer, so it directly drives the RAM footprint of one link.
 */
#ifndef DXL2_MAX_PACKET_SIZE
#define DXL2_MAX_PACKET_SIZE 256U
#endif

/** @} */

#endif /* DXL2_CONFIG_H */

/** @} */
