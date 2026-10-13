/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Unit tests for dxl2_link_receive
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
 * @file gtest_dxl2_link_receive.cpp
 * @brief Unit tests for dxl2_link_receive.
 * @details Covers chunked streams with garbage and invalid packet accounting.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <gtest/gtest.h>

#include "dxl2_test_support.h"

/* ==============================================================================
 * UNIT TEST IMPLEMENTATIONS
 * =============================================================================*/

/** @brief Dispatches both packets of a stream fed in 3-byte chunks with noise. */
TEST(Dxl2LinkReceive, ChunkedStreamWithGarbage)
{
    static const uint8_t noise[] = {0x00, 0xFF, 0x13, 0xFF, 0xFF};
    uint8_t stream[3U * DXL2_MAX_PACKET_SIZE];
    size_t stream_length = 0U;
    ReceivedPackets received = {};
    struct dxl2_link_config config = {};
    struct dxl2_link link;
    size_t length = 0U;

    config.handler = record_packet;
    config.handler_context = &received;

    std::memcpy(stream, noise, sizeof(noise));
    stream_length = sizeof(noise);
    ASSERT_EQ(dxl2_encode(1U, DXL2_INST_PING, nullptr, 0U, stream + stream_length,
                          sizeof(stream) - stream_length, &length),
              0);
    stream_length += length;
    std::memcpy(stream + stream_length, noise, sizeof(noise));
    stream_length += sizeof(noise);
    ASSERT_EQ(dxl2_encode(DXL2_BROADCAST_ID, DXL2_INST_ACTION, nullptr, 0U, stream + stream_length,
                          sizeof(stream) - stream_length, &length),
              0);
    stream_length += length;

    ASSERT_EQ(dxl2_link_init(&link, &config), 0);
    for (size_t i = 0U; i < stream_length; i += 3U)
    {
        size_t chunk = (stream_length - i) < 3U ? (stream_length - i) : 3U;

        dxl2_link_receive(&link, stream + i, chunk);
    }

    ASSERT_EQ(received.count, 2);
    EXPECT_EQ(received.ids[0], 1U);
    EXPECT_EQ(received.instructions[0], DXL2_INST_PING);
    EXPECT_EQ(received.ids[1], DXL2_BROADCAST_ID);
    EXPECT_EQ(received.instructions[1], DXL2_INST_ACTION);
    EXPECT_EQ(dxl2_link_rx_packets(&link), 2U);
    EXPECT_EQ(dxl2_link_invalid_packets(&link), 0U);
}

/** @brief A CRC-invalid packet is counted and not dispatched. */
TEST(Dxl2LinkReceive, InvalidPacketIsCounted)
{
    static const uint8_t bad_ping[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x03, 0x00, 0x01, 0x19, 0x00};
    ReceivedPackets received = {};
    struct dxl2_link_config config = {};
    struct dxl2_link link;

    config.handler = record_packet;
    config.handler_context = &received;

    ASSERT_EQ(dxl2_link_init(&link, &config), 0);
    dxl2_link_receive(&link, bad_ping, sizeof(bad_ping));

    EXPECT_EQ(received.count, 0);
    EXPECT_EQ(dxl2_link_invalid_packets(&link), 1U);
    EXPECT_EQ(dxl2_link_rx_packets(&link), 0U);
}

/** @brief Without a handler, valid packets are still counted. */
TEST(Dxl2LinkReceive, CountsWithoutHandler)
{
    static const uint8_t ping[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x03, 0x00, 0x01, 0x19, 0x4E};
    struct dxl2_link_config config = {};
    struct dxl2_link link;

    ASSERT_EQ(dxl2_link_init(&link, &config), 0);
    dxl2_link_receive(&link, ping, sizeof(ping));
    dxl2_link_receive(&link, nullptr, 4U);
    dxl2_link_receive(nullptr, ping, sizeof(ping));

    EXPECT_EQ(dxl2_link_rx_packets(&link), 1U);
    EXPECT_EQ(dxl2_link_invalid_packets(&link), 0U);
}
