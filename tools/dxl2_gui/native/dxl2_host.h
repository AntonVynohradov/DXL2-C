/**
 * @file dxl2_host.h
 * @brief Host (controller) side of the DXL2 link, for the GUI tool.
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
 * @defgroup dxl2_host Host link
 * @brief Controller-side wrapper of @ref dxl2_link used by the DXL2 GUI tool.
 *
 * @details Instruction Packets are encoded by @ref dxl2_link_send into a
 *          caller buffer; received bytes go through @ref dxl2_link_receive and
 *          the decoded packets are queued until the caller takes them. Every
 *          byte the GUI puts on the wire or decodes passes through the library.
 * @{
 */

#ifndef DXL2_HOST_H
#define DXL2_HOST_H

/** @name Includes */
/** @{ */

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

/** Number of decoded packets the host keeps until they are taken. */
#define DXL2_HOST_RX_QUEUE 32U

/** @} */

/** @name Type definitions */
/** @{ */

/** @brief Host link state. Treat the members as private. */
struct dxl2_host
{
    struct dxl2_link link;                        /**< Protocol link. */
    struct dxl2_packet queue[DXL2_HOST_RX_QUEUE]; /**< Decoded packets not yet taken. */
    size_t head;                                  /**< Index of the oldest packet. */
    size_t count;                                 /**< Number of queued packets. */
    uint32_t dropped;                             /**< Packets lost to a full queue. */
    uint8_t* tx;                                  /**< Output buffer of the current build. */
    size_t tx_capacity;                           /**< Capacity of tx. */
    size_t tx_length;                             /**< Bytes written to tx. */
};

/** @} */

/** @name Public function declarations */
/** @{ */

/** @brief Size of struct dxl2_host, for callers that allocate it opaquely. */
DXL2_TOOL_API size_t dxl2_host_size(void);

/** @brief Size of the library's maximum wire packet (DXL2_MAX_PACKET_SIZE). */
DXL2_TOOL_API size_t dxl2_host_max_packet_size(void);

/**
 * @brief Initialize the host link.
 * @retval 0 Host ready.
 * @retval -1 @p host is NULL.
 */
DXL2_TOOL_API int dxl2_host_init(struct dxl2_host* host);

/**
 * @brief Encode an Instruction Packet through @ref dxl2_link_send.
 *
 * @param host Host state.
 * @param id Target ID.
 * @param instruction Instruction byte.
 * @param parameters Parameter bytes, or NULL when @p parameter_length is zero.
 * @param parameter_length Number of parameter bytes.
 * @param output Output buffer for the wire bytes.
 * @param output_capacity Capacity of @p output.
 * @return Number of wire bytes, or -1 when the packet does not fit or is invalid.
 */
DXL2_TOOL_API int dxl2_host_build(struct dxl2_host* host, uint8_t id, uint8_t instruction,
                                  const uint8_t* parameters, uint16_t parameter_length,
                                  uint8_t* output, size_t output_capacity);

/**
 * @brief Feed received bytes; complete packets are queued.
 * @return Number of packets waiting in the queue.
 */
DXL2_TOOL_API size_t dxl2_host_receive(struct dxl2_host* host, const uint8_t* data, size_t length);

/**
 * @brief Take the oldest queued packet.
 *
 * @param host Host state.
 * @param id Receives the packet ID.
 * @param instruction Receives the instruction byte (0x55 for Status).
 * @param parameters Receives the unstuffed parameters (Status: error byte first).
 * @param capacity Capacity of @p parameters.
 * @return Number of parameter bytes copied, or -1 when the queue is empty.
 */
DXL2_TOOL_API int dxl2_host_take(struct dxl2_host* host, uint8_t* id, uint8_t* instruction,
                                 uint8_t* parameters, size_t capacity);

/** @brief Drop queued packets and any partial packet in the parser. */
DXL2_TOOL_API void dxl2_host_flush(struct dxl2_host* host);

/** @brief Valid packets received (library counter). */
DXL2_TOOL_API uint32_t dxl2_host_rx_packets(const struct dxl2_host* host);

/** @brief Malformed or CRC-invalid packets rejected (library counter). */
DXL2_TOOL_API uint32_t dxl2_host_invalid_packets(const struct dxl2_host* host);

/** @brief CRC-16 of @p data (library implementation). */
DXL2_TOOL_API uint16_t dxl2_host_crc16(const uint8_t* data, size_t length);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DXL2_HOST_H */

/** @} */
