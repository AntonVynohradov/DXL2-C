/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Unit tests for dxl2_encode
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
 * @file gtest_dxl2_encode.cpp
 * @brief Unit tests for the packet encoder.
 * @details Covers the reference packet, byte stuffing and argument validation.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <gtest/gtest.h>

#include "dxl2_test_support.h"

/* ==============================================================================
 * UNIT TEST IMPLEMENTATIONS
 * =============================================================================*/

/** @brief Produces the ROBOTIS PING reference packet byte for byte. */
TEST(Dxl2Encode, MatchesPingReference)
{
    static const uint8_t ping[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x03, 0x00, 0x01, 0x19, 0x4E};
    uint8_t encoded[DXL2_MAX_PACKET_SIZE];
    size_t encoded_length = 0U;

    ASSERT_EQ(
        dxl2_encode(1U, DXL2_INST_PING, nullptr, 0U, encoded, sizeof(encoded), &encoded_length), 0);
    EXPECT_EQ(encoded_length, sizeof(ping));
    EXPECT_EQ(std::memcmp(encoded, ping, sizeof(ping)), 0);
}

/** @brief Inserts 0xFD after FF FF FD in the parameters. */
TEST(Dxl2Encode, StuffsHeaderSequence)
{
    static const uint8_t parameters[] = {0xFF, 0xFF, 0xFD};
    uint8_t encoded[DXL2_MAX_PACKET_SIZE];
    size_t encoded_length = 0U;

    ASSERT_EQ(dxl2_encode(1U, DXL2_INST_WRITE, parameters, sizeof(parameters), encoded,
                          sizeof(encoded), &encoded_length),
              0);
    EXPECT_EQ(encoded[7], DXL2_INST_WRITE);
    EXPECT_EQ(encoded[8], 0xFFU);
    EXPECT_EQ(encoded[9], 0xFFU);
    EXPECT_EQ(encoded[10], 0xFDU);
    EXPECT_EQ(encoded[11], 0xFDU);
}

/** @brief Rejects invalid arguments and insufficient capacity. */
TEST(Dxl2Encode, RejectsInvalidArguments)
{
    uint8_t encoded[DXL2_MAX_PACKET_SIZE];
    size_t encoded_length = 0U;

    EXPECT_EQ(
        dxl2_encode(1U, DXL2_INST_PING, nullptr, 0U, nullptr, sizeof(encoded), &encoded_length),
        DXL2_PARSE_ERROR);
    EXPECT_EQ(dxl2_encode(1U, DXL2_INST_PING, nullptr, 0U, encoded, sizeof(encoded), nullptr),
              DXL2_PARSE_ERROR);
    EXPECT_EQ(
        dxl2_encode(1U, DXL2_INST_WRITE, nullptr, 1U, encoded, sizeof(encoded), &encoded_length),
        DXL2_PARSE_ERROR);
    EXPECT_EQ(dxl2_encode(1U, DXL2_INST_PING, nullptr, 0U, encoded, 9U, &encoded_length),
              DXL2_PARSE_ERROR);
}

/** @brief Rejects a parameter block larger than DXL2_MAX_PARAMETER_SIZE. */
TEST(Dxl2Encode, RejectsOversizedParameters)
{
    static const uint8_t parameters[DXL2_MAX_PARAMETER_SIZE + 1U] = {0};
    uint8_t encoded[DXL2_MAX_PACKET_SIZE];
    size_t encoded_length = 0U;

    EXPECT_EQ(dxl2_encode(1U, DXL2_INST_WRITE, parameters, sizeof(parameters), encoded,
                          sizeof(encoded), &encoded_length),
              DXL2_PARSE_ERROR);
}

/** @brief Rejects an output buffer too small for the parameters. */
TEST(Dxl2Encode, RejectsCapacityForParameters)
{
    static const uint8_t parameters[] = {0x01};
    uint8_t encoded[10];
    size_t encoded_length = 0U;

    EXPECT_EQ(dxl2_encode(1U, DXL2_INST_WRITE, parameters, sizeof(parameters), encoded,
                          sizeof(encoded), &encoded_length),
              DXL2_PARSE_ERROR);
    EXPECT_EQ(encoded_length, 0U);
}

/** @brief Rejects an output buffer that has no room for the stuffing byte. */
TEST(Dxl2Encode, RejectsCapacityForStuffing)
{
    static const uint8_t parameters[] = {0xFF, 0xFF, 0xFD};
    uint8_t encoded[14];
    size_t encoded_length = 0U;

    EXPECT_EQ(dxl2_encode(1U, DXL2_INST_WRITE, parameters, sizeof(parameters), encoded, 13U,
                          &encoded_length),
              DXL2_PARSE_ERROR);
    EXPECT_EQ(dxl2_encode(1U, DXL2_INST_WRITE, parameters, sizeof(parameters), encoded, 14U,
                          &encoded_length),
              0);
    EXPECT_EQ(encoded_length, 14U);
}
