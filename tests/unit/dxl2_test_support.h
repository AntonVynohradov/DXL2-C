/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Test support utilities and mocks
 *
 * Author:      Anton Vynohradov
 * Email:       avynohradov@systemfromscratch.com
 *
 * License:     Apache License 2.0
 *
 * Copyright (c) 2026 Anton Vynohradov
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
 ******************************************************************************/

/**
 * @file dxl2_test_support.h
 * @brief Test support utilities and mocks.
 * @details Declares the transport/handler mocks and helper APIs for unit tests.
 */

#ifndef DXL2_TEST_SUPPORT_H
#define DXL2_TEST_SUPPORT_H

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C"
{
#include "dxl2.h"
}

/* ==============================================================================
 * DEFINES & MACROS
 * =============================================================================*/

/** Maximum number of packets recorded by @ref ReceivedPackets. */
#define DXL2_TEST_MAX_RECORDED 8

/** Return value of @ref failing_write. */
#define DXL2_TEST_WRITE_ERROR (-5)

/* ==============================================================================
 * TYPE DEFINITIONS
 * =============================================================================*/

/**
 * @brief Capture of the last packet written to the transport.
 * @details Used as write_context for @ref capture_write.
 */
struct WriteCapture
{
    uint8_t data[DXL2_MAX_PACKET_SIZE];
    size_t length;
    int writes;
};

/**
 * @brief Record of packets dispatched by the link.
 * @details Used as handler_context for @ref record_packet.
 */
struct ReceivedPackets
{
    uint8_t ids[DXL2_TEST_MAX_RECORDED];
    uint8_t instructions[DXL2_TEST_MAX_RECORDED];
    int count;
};

/* ==============================================================================
 * PUBLIC FUNCTION DECLARATIONS
 * =============================================================================*/

/**
 * @brief   dxl2_write_fn mock that copies the packet into a WriteCapture.
 * @param   context - WriteCapture instance
 * @param   data - Encoded packet
 * @param   length - Packet length
 * @return  0
 */
int capture_write(void* context, const uint8_t* data, size_t length);

/**
 * @brief   dxl2_write_fn mock that always fails.
 * @return  DXL2_TEST_WRITE_ERROR
 */
int failing_write(void* context, const uint8_t* data, size_t length);

/**
 * @brief   dxl2_packet_handler_t mock that records ID and instruction.
 * @param   packet - Decoded packet
 * @param   context - ReceivedPackets instance
 */
void record_packet(const struct dxl2_packet* packet, void* context);

/**
 * @brief   Feed a whole buffer to a parser one byte at a time.
 * @param   parser - Initialized parser
 * @param   data - Stream bytes
 * @param   length - Number of bytes
 * @param   packet - Output packet
 * @return  Result of the last dxl2_parser_feed() call (0 for an empty buffer)
 */
int feed_all(struct dxl2_parser* parser, const uint8_t* data, size_t length,
             struct dxl2_packet* packet);

#endif /* DXL2_TEST_SUPPORT_H */
