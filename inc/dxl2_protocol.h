/**
 * @file dxl2_protocol.h
 * @brief DYNAMIXEL Protocol 2.0 packet encoding, parsing, and CRC helpers.
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
 * @defgroup dxl2_protocol Codec
 * @ingroup dxl2
 * @brief CRC-16, incremental stream parser and packet encoder with byte stuffing.
 *
 * @par Public interface includes:
 * @li CRC: @ref dxl2_crc16.
 * @li Parser: @ref dxl2_parser_init, @ref dxl2_parser_feed.
 * @li Encoder: @ref dxl2_encode.
 * @{
 */

#ifndef DXL2_PROTOCOL_H
#define DXL2_PROTOCOL_H

/** @name Includes */
/** @{ */

#include <stddef.h>
#include <stdint.h>

#include "dxl2_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @} */

/** @name Packet limits and errors */
/** @{ */

/** Maximum unstuffed parameter payload size supported by this implementation. */
#define DXL2_MAX_PARAMETER_SIZE (DXL2_MAX_PACKET_SIZE - 10U)

/** Error returned for invalid input, malformed packets, or insufficient capacity. */
#define DXL2_PARSE_ERROR (-1)

/** @} */

/** @name Packet and parser state */
/** @{ */

/**
 * @brief One decoded Protocol 2.0 packet.
 *
 * @details Byte stuffing has been removed from @p parameters. For a Status
 *          Packet (instruction 0x55), the status error byte is the first
 *          parameter.
 */
struct dxl2_packet
{
    uint8_t id;                                  /**< Packet source/destination ID. */
    uint8_t instruction;                         /**< Instruction byte, 0x55 for Status. */
    uint16_t parameter_length;                   /**< Number of valid bytes in parameters. */
    uint8_t parameters[DXL2_MAX_PARAMETER_SIZE]; /**< Unstuffed parameter bytes. */
};

/**
 * @brief Incremental parser state for a byte stream.
 *
 * @details Keep one instance per input stream and initialize it before
 *          feeding bytes.
 */
struct dxl2_parser
{
    uint8_t buffer[DXL2_MAX_PACKET_SIZE]; /**< In-progress wire packet, header to CRC. */
    size_t length;                        /**< Number of bytes currently in buffer. */
};

/** @} */

/** @name CRC and packet parsing */
/** @{ */

/**
 * @brief Calculate the Protocol 2.0 CRC-16 checksum.
 *
 * @details Uses polynomial 0x8005 and an initial CRC value of zero.
 *
 * @param data Bytes covered by the CRC; may be NULL only when @p length is zero.
 * @param length Number of bytes to process.
 * @return Calculated 16-bit CRC.
 */
uint16_t dxl2_crc16(const uint8_t* data, size_t length);

/**
 * @brief Reset a parser before starting or restarting a byte stream.
 * @param parser Parser state to initialize; NULL is ignored.
 */
void dxl2_parser_init(struct dxl2_parser* parser);

/**
 * @brief Feed one byte to the incremental packet parser.
 *
 * @details The parser searches for the Protocol 2.0 header, waits for the
 *          declared packet length, validates CRC, then removes byte stuffing.
 *          Continue feeding bytes after a parse error so the parser can find
 *          the next packet header.
 *
 * @param parser Parser state.
 * @param byte Next byte from the input stream.
 * @param packet Output packet; updated only when a complete valid packet is found.
 * @retval 1 A complete valid packet is available in @p packet.
 * @retval 0 More bytes are required.
 * @retval DXL2_PARSE_ERROR Invalid arguments or a malformed/CRC-invalid packet.
 */
int dxl2_parser_feed(struct dxl2_parser* parser, uint8_t byte, struct dxl2_packet* packet);

/** @} */

/** @name Packet encoding */
/** @{ */

/**
 * @brief Encode a Protocol 2.0 packet, adding byte stuffing and CRC.
 *
 * @param id Packet ID.
 * @param instruction Instruction byte.
 * @param parameters Unstuffed parameter bytes, or NULL when the length is zero.
 * @param parameter_length Number of parameter bytes.
 * @param output Destination buffer for the complete wire packet.
 * @param output_capacity Capacity of @p output in bytes.
 * @param output_length Receives the encoded packet length on success.
 * @retval 0 Packet encoded successfully.
 * @retval DXL2_PARSE_ERROR Invalid input or insufficient output capacity.
 */
int dxl2_encode(uint8_t id, uint8_t instruction, const uint8_t* parameters,
                uint16_t parameter_length, uint8_t* output, size_t output_capacity,
                size_t* output_length);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DXL2_PROTOCOL_H */

/** @} */
