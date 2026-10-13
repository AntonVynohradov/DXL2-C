/**
 * @file dxl2_link.h
 * @brief Transport-independent Protocol 2.0 link: stream in, packets out.
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
 * @defgroup dxl2_link Link
 * @ingroup dxl2
 * @brief Transport-independent link that owns a parser and dispatches packets.
 *
 * @details The link owns a parser, dispatches every valid packet to a handler,
 *          and encodes outgoing packets into a caller-supplied write function.
 *          It has no RTOS or driver dependency: the integrator moves received
 *          bytes from the UART/USB/socket into @ref dxl2_link_receive and
 *          provides the byte writer.
 *
 *          A link is not thread-safe. Call @ref dxl2_link_receive from one
 *          context only (for example an RX thread, not an interrupt handler),
 *          and serialize @ref dxl2_link_send calls from several contexts with
 *          a mutex.
 *
 * @par Public interface includes:
 * @li Lifecycle: @ref dxl2_link_init.
 * @li Message flow: @ref dxl2_link_receive, @ref dxl2_link_send,
 *     @ref dxl2_link_send_status.
 * @li Diagnostics: @ref dxl2_link_rx_packets, @ref dxl2_link_invalid_packets.
 * @{
 */

#ifndef DXL2_LINK_H
#define DXL2_LINK_H

/** @name Includes */
/** @{ */

#include <stddef.h>
#include <stdint.h>

#include "dxl2_protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @} */

/** @name Type definitions */
/** @{ */

/**
 * @brief Write a complete wire packet to the transport.
 *
 * @param context Value of dxl2_link_config::write_context.
 * @param data Encoded packet, including header and CRC.
 * @param length Number of bytes in @p data.
 * @return 0 on success, a negative value on failure (returned by the send call).
 */
typedef int (*dxl2_write_fn)(void* context, const uint8_t* data, size_t length);

/**
 * @brief Handle one valid packet received by the link.
 *
 * @details The packet pointer is valid only for the duration of the callback.
 *          Copy any data that must be retained after the callback returns. The
 *          handler may call @ref dxl2_link_send on the same link to answer.
 *
 * @param packet Decoded packet with byte stuffing removed.
 * @param context Value of dxl2_link_config::handler_context.
 */
typedef void (*dxl2_packet_handler_t)(const struct dxl2_packet* packet, void* context);

/** @brief Link configuration passed to @ref dxl2_link_init. */
struct dxl2_link_config
{
    dxl2_write_fn write;           /**< Byte writer; NULL makes the link receive-only. */
    void* write_context;           /**< Context for write. */
    dxl2_packet_handler_t handler; /**< Packet handler; NULL discards (still counted). */
    void* handler_context;         /**< Context for handler. */
};

/** @brief Link state. Treat the members as private; use the functions below. */
struct dxl2_link
{
    struct dxl2_link_config config; /**< Copy of the configuration. */
    struct dxl2_parser parser;      /**< Receive stream parser. */
    struct dxl2_packet packet;      /**< Last decoded packet. */
    uint32_t rx_packets;            /**< Valid packets received. */
    uint32_t invalid_packets;       /**< Malformed or CRC-invalid packets rejected. */
};

/** @} */

/** @name Public function declarations */
/** @{ */

/**
 * @brief Initialize a link and reset its parser and counters.
 * @param link Link state to initialize.
 * @param config Configuration copied into the link.
 * @retval 0 Link ready.
 * @retval DXL2_PARSE_ERROR @p link or @p config is NULL.
 */
int dxl2_link_init(struct dxl2_link* link, const struct dxl2_link_config* config);

/**
 * @brief Feed received bytes; calls the handler for each complete valid packet.
 *
 * @details Bytes may arrive in any chunking. Malformed or CRC-invalid packets
 *          are dropped and counted, and the parser resynchronizes on the next
 *          header.
 *
 * @param link Link state.
 * @param data Received bytes; may be NULL only when @p length is zero.
 * @param length Number of bytes in @p data.
 */
void dxl2_link_receive(struct dxl2_link* link, const uint8_t* data, size_t length);

/**
 * @brief Encode one packet (stuffing and CRC added) and write it.
 *
 * @param link Link state.
 * @param id Destination ID.
 * @param instruction Instruction byte.
 * @param parameters Unstuffed parameters, or NULL when @p parameter_length is zero.
 * @param parameter_length Number of parameter bytes.
 * @retval 0 Packet written.
 * @retval DXL2_PARSE_ERROR Invalid arguments, packet too large, or no writer.
 * @retval Other negative value Error returned by the write function.
 */
int dxl2_link_send(struct dxl2_link* link, uint8_t id, uint8_t instruction,
                   const uint8_t* parameters, uint16_t parameter_length);

/**
 * @brief Send a Status Packet: error byte followed by @p data.
 *
 * @param link Link state.
 * @param id ID of the answering device.
 * @param error Status error byte (DXL2_ERR_*, optionally with DXL2_ERR_ALERT_BIT).
 * @param data Status data, or NULL when @p data_length is zero.
 * @param data_length Number of data bytes (at most DXL2_MAX_PARAMETER_SIZE - 1).
 * @return Same as @ref dxl2_link_send.
 */
int dxl2_link_send_status(struct dxl2_link* link, uint8_t id, uint8_t error, const uint8_t* data,
                          uint16_t data_length);

/** @brief Number of valid packets received since @ref dxl2_link_init. */
uint32_t dxl2_link_rx_packets(const struct dxl2_link* link);

/** @brief Number of malformed or CRC-invalid packets rejected since @ref dxl2_link_init. */
uint32_t dxl2_link_invalid_packets(const struct dxl2_link* link);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DXL2_LINK_H */

/** @} */
