/**
 * @file dxl2_sim.h
 * @brief Simulated DYNAMIXEL device built on the DXL2 link, for host tools.
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
 * @defgroup dxl2_sim Device simulator
 * @brief Virtual DYNAMIXEL device used by the DXL2 GUI tool.
 *
 * @details The simulator owns a @ref dxl2_link and a Control Table modeled on
 *          the XL430-W250 (a subset of its items). Request bytes go in with
 *          @ref dxl2_sim_process, the encoded Status Packets come out of the
 *          same call. It is host tooling, not part of the embedded library.
 * @{
 */

#ifndef DXL2_SIM_H
#define DXL2_SIM_H

/** @name Includes */
/** @{ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dxl2.h"
#include "dxl2_tool_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @} */

/** @name Defines and macros */
/** @{ */

/** Size of the simulated Control Table in bytes (last item ends at 147). */
#define DXL2_SIM_TABLE_SIZE 147U

/** Capacity of the status output buffer of one @ref dxl2_sim_process call. */
#define DXL2_SIM_TX_CAPACITY (4U * DXL2_MAX_PACKET_SIZE)

/** Model number reported by PING (XL430-W250). */
#define DXL2_SIM_MODEL_NUMBER 1060U

/** Firmware version reported by PING. */
#define DXL2_SIM_FIRMWARE_VERSION 52U

/** @} */

/** @name Type definitions */
/** @{ */

/** @brief Simulated device state. Treat the members as private. */
struct dxl2_sim
{
    struct dxl2_link link;                         /**< Protocol link of the device. */
    uint8_t table[DXL2_SIM_TABLE_SIZE];            /**< Control Table contents. */
    uint8_t pending_data[DXL2_MAX_PARAMETER_SIZE]; /**< Data of a REG_WRITE. */
    uint16_t pending_address;                      /**< Address of a REG_WRITE. */
    uint16_t pending_length;                       /**< Length of a REG_WRITE. */
    bool pending_valid;                            /**< REG_WRITE waiting for ACTION. */
    uint8_t tx[DXL2_SIM_TX_CAPACITY];              /**< Status bytes produced so far. */
    size_t tx_length;                              /**< Number of bytes in tx. */
};

/** @} */

/** @name Public function declarations */
/** @{ */

/** @brief Size of struct dxl2_sim, for callers that allocate it opaquely. */
DXL2_TOOL_API size_t dxl2_sim_size(void);

/**
 * @brief Initialize a device with factory defaults and the given ID.
 * @retval 0 Device ready.
 * @retval -1 @p sim is NULL or @p id is above DXL2_MAX_ID.
 */
DXL2_TOOL_API int dxl2_sim_init(struct dxl2_sim* sim, uint8_t id);

/**
 * @brief Feed request bytes and collect the Status Packets the device sends.
 *
 * @param sim Device state.
 * @param rx Request bytes in any chunking; may be NULL when @p rx_length is zero.
 * @param rx_length Number of request bytes.
 * @param tx Output buffer for the status bytes.
 * @param tx_capacity Capacity of @p tx.
 * @return Number of bytes written to @p tx (excess bytes are dropped).
 */
DXL2_TOOL_API size_t dxl2_sim_process(struct dxl2_sim* sim, const uint8_t* rx, size_t rx_length,
                                      uint8_t* tx, size_t tx_capacity);

/** @brief Advance the simulation one step: Present Position moves toward Goal Position. */
DXL2_TOOL_API void dxl2_sim_tick(struct dxl2_sim* sim);

/** @brief Current device ID (Control Table item 7). */
DXL2_TOOL_API uint8_t dxl2_sim_id(const struct dxl2_sim* sim);

/**
 * @brief Copy raw Control Table bytes.
 * @retval 0 Bytes copied.
 * @retval -1 Invalid arguments or range outside the table.
 */
DXL2_TOOL_API int dxl2_sim_read_table(const struct dxl2_sim* sim, uint16_t address, uint8_t* out,
                                      uint16_t length);

/** @brief Set the Hardware Error Status item (non-zero raises the alert bit). */
DXL2_TOOL_API void dxl2_sim_set_hardware_error(struct dxl2_sim* sim, uint8_t value);

/** @brief Number of malformed or CRC-invalid packets the device rejected. */
DXL2_TOOL_API uint32_t dxl2_sim_invalid_packets(const struct dxl2_sim* sim);

/** @brief Number of Control Table items. */
DXL2_TOOL_API size_t dxl2_sim_item_count(void);

/**
 * @brief Describe one Control Table item.
 * @param index Item index, below @ref dxl2_sim_item_count.
 * @param address Receives the item address.
 * @param size Receives the item size in bytes.
 * @param writable Receives 1 for read/write items, 0 for read-only items.
 * @param name Receives a pointer to the static item name.
 * @retval 0 Item described.
 * @retval -1 Invalid index or NULL output pointer.
 */
DXL2_TOOL_API int dxl2_sim_item_info(size_t index, uint16_t* address, uint8_t* size,
                                     uint8_t* writable, const char** name);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DXL2_SIM_H */

/** @} */
