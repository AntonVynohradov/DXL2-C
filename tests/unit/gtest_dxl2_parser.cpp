/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Unit tests for dxl2_parser_init and dxl2_parser_feed
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
 * @file gtest_dxl2_parser.cpp
 * @brief Unit tests for the incremental packet parser.
 * @details Covers reference packets, byte stuffing, CRC and stuffing errors.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <gtest/gtest.h>

#include "dxl2_test_support.h"

/* ==============================================================================
 * UNIT TEST IMPLEMENTATIONS
 * =============================================================================*/

/** @brief Parses the ROBOTIS PING reference packet. */
TEST(Dxl2Parser, ParsesPingReference)
{
    static const uint8_t ping[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x03, 0x00, 0x01, 0x19, 0x4E};
    struct dxl2_parser parser;
    struct dxl2_packet packet;

    dxl2_parser_init(&parser);

    EXPECT_EQ(feed_all(&parser, ping, sizeof(ping), &packet), 1);
    EXPECT_EQ(packet.id, 1U);
    EXPECT_EQ(packet.instruction, DXL2_INST_PING);
    EXPECT_EQ(packet.parameter_length, 0U);
}

/** @brief Removes byte stuffing from an encoded packet. */
TEST(Dxl2Parser, UnstuffsRoundTrip)
{
    static const uint8_t parameters[] = {0xFF, 0xFF, 0xFD};
    struct dxl2_parser parser;
    struct dxl2_packet packet;
    uint8_t encoded[DXL2_MAX_PACKET_SIZE];
    size_t encoded_length = 0U;

    ASSERT_EQ(dxl2_encode(1U, DXL2_INST_WRITE, parameters, sizeof(parameters), encoded,
                          sizeof(encoded), &encoded_length),
              0);
    dxl2_parser_init(&parser);

    EXPECT_EQ(feed_all(&parser, encoded, encoded_length, &packet), 1);
    EXPECT_EQ(packet.parameter_length, sizeof(parameters));
    EXPECT_EQ(std::memcmp(packet.parameters, parameters, sizeof(parameters)), 0);
}

/** @brief Rejects a packet with a wrong CRC. */
TEST(Dxl2Parser, RejectsCrcError)
{
    static const uint8_t bad_ping[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x03, 0x00, 0x01, 0x19, 0x00};
    struct dxl2_parser parser;
    struct dxl2_packet packet;

    dxl2_parser_init(&parser);

    EXPECT_EQ(feed_all(&parser, bad_ping, sizeof(bad_ping), &packet), DXL2_PARSE_ERROR);
}

/** @brief Rejects FF FF FD not followed by the stuffing byte, even with a valid CRC. */
TEST(Dxl2Parser, RejectsMalformedStuffing)
{
    static const uint8_t parameters[] = {0xFF, 0xFF, 0xFD};
    struct dxl2_parser parser;
    struct dxl2_packet packet;
    uint8_t encoded[DXL2_MAX_PACKET_SIZE];
    size_t encoded_length = 0U;
    uint16_t crc;

    ASSERT_EQ(dxl2_encode(1U, DXL2_INST_WRITE, parameters, sizeof(parameters), encoded,
                          sizeof(encoded), &encoded_length),
              0);
    encoded[11] = 0xFEU;
    crc = dxl2_crc16(encoded, encoded_length - 2U);
    encoded[encoded_length - 2U] = (uint8_t) (crc & 0xFFU);
    encoded[encoded_length - 1U] = (uint8_t) (crc >> 8);
    dxl2_parser_init(&parser);

    EXPECT_EQ(feed_all(&parser, encoded, encoded_length, &packet), DXL2_PARSE_ERROR);
}

/** @brief Rejects NULL arguments. */
TEST(Dxl2Parser, RejectsNullArguments)
{
    struct dxl2_parser parser;
    struct dxl2_packet packet;

    dxl2_parser_init(&parser);
    dxl2_parser_init(nullptr);

    EXPECT_EQ(dxl2_parser_feed(nullptr, 0xFFU, &packet), DXL2_PARSE_ERROR);
    EXPECT_EQ(dxl2_parser_feed(&parser, 0xFFU, nullptr), DXL2_PARSE_ERROR);
}
