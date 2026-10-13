/**
 * @file dxl2_defs.h
 * @brief DYNAMIXEL Protocol 2.0 constants and little-endian field helpers.
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
 * @defgroup dxl2_defs Definitions
 * @ingroup dxl2
 * @brief Protocol 2.0 IDs, instructions, error codes and byte-order helpers.
 *
 * @details Values come from the ROBOTIS Protocol 2.0 specification
 *          (https://docs.robotis.com/docs/dxl/protocol/protocol2/).
 * @{
 */

#ifndef DXL2_DEFS_H
#define DXL2_DEFS_H

/** @name Includes */
/** @{ */

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @} */

/** @name Device IDs */
/** @{ */

/** Highest ID a device may use. */
#define DXL2_MAX_ID 252U
/** Broadcast ID: every device executes the instruction. */
#define DXL2_BROADCAST_ID 0xFEU

/** @} */

/** @name Instructions */
/** @{ */

#define DXL2_INST_PING 0x01U
#define DXL2_INST_READ 0x02U
#define DXL2_INST_WRITE 0x03U
#define DXL2_INST_REG_WRITE 0x04U
#define DXL2_INST_ACTION 0x05U
#define DXL2_INST_FACTORY_RESET 0x06U
#define DXL2_INST_REBOOT 0x08U
#define DXL2_INST_CLEAR 0x10U
#define DXL2_INST_BACKUP 0x20U
/** Instruction byte of a Status Packet. */
#define DXL2_INST_STATUS 0x55U
#define DXL2_INST_SYNC_READ 0x82U
#define DXL2_INST_SYNC_WRITE 0x83U
#define DXL2_INST_FAST_SYNC_READ 0x8AU
#define DXL2_INST_BULK_READ 0x92U
#define DXL2_INST_BULK_WRITE 0x93U
#define DXL2_INST_FAST_BULK_READ 0x9AU

/** @} */

/**
 * @name Status Packet error byte
 * @details Bits 0-6 carry the error number, bit 7 is the hardware alert flag.
 */
/** @{ */

#define DXL2_ERR_NONE 0x00U
#define DXL2_ERR_RESULT_FAIL 0x01U
#define DXL2_ERR_INSTRUCTION 0x02U
#define DXL2_ERR_CRC 0x03U
#define DXL2_ERR_DATA_RANGE 0x04U
#define DXL2_ERR_DATA_LENGTH 0x05U
#define DXL2_ERR_DATA_LIMIT 0x06U
#define DXL2_ERR_ACCESS 0x07U
/** Hardware alert flag: the device has a Hardware Error Status set. */
#define DXL2_ERR_ALERT_BIT 0x80U

/** @} */

/**
 * @name Little-endian field access
 * @details All multi-byte Protocol 2.0 fields (addresses, lengths, register
 *          data) are little endian.
 */
/** @{ */

/** @brief Read a little-endian 16-bit value. */
static inline uint16_t dxl2_get_u16(const uint8_t* data)
{
    return (uint16_t) ((uint16_t) data[0] | ((uint16_t) data[1] << 8));
}

/** @brief Read a little-endian 32-bit value. */
static inline uint32_t dxl2_get_u32(const uint8_t* data)
{
    return (uint32_t) data[0] | ((uint32_t) data[1] << 8) | ((uint32_t) data[2] << 16)
           | ((uint32_t) data[3] << 24);
}

/** @brief Write a 16-bit value in little-endian order. */
static inline void dxl2_put_u16(uint8_t* data, uint16_t value)
{
    data[0] = (uint8_t) (value & 0xFFU);
    data[1] = (uint8_t) (value >> 8);
}

/** @brief Write a 32-bit value in little-endian order. */
static inline void dxl2_put_u32(uint8_t* data, uint32_t value)
{
    data[0] = (uint8_t) (value & 0xFFU);
    data[1] = (uint8_t) ((value >> 8) & 0xFFU);
    data[2] = (uint8_t) ((value >> 16) & 0xFFU);
    data[3] = (uint8_t) (value >> 24);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DXL2_DEFS_H */

/** @} */
