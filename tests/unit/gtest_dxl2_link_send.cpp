/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Unit tests for dxl2_link_send and dxl2_link_send_status
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
 * @file gtest_dxl2_link_send.cpp
 * @brief Unit tests for the link transmit path.
 * @details Covers the ROBOTIS status reference packet and error propagation.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <gtest/gtest.h>

#include "dxl2_test_support.h"

/* ==============================================================================
 * UNIT TEST IMPLEMENTATIONS
 * =============================================================================*/

/** @brief ROBOTIS reference: status of ID 1 to PING, model 1030, firmware 38. */
TEST(Dxl2LinkSend, PingStatusReference)
{
    static const uint8_t expected[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x07, 0x00,
                                       0x55, 0x00, 0x06, 0x04, 0x26, 0x65, 0x5D};
    static const uint8_t data[] = {0x06, 0x04, 0x26};
    WriteCapture capture = {};
    struct dxl2_link_config config = {};
    struct dxl2_link link;

    config.write = capture_write;
    config.write_context = &capture;

    ASSERT_EQ(dxl2_link_init(&link, &config), 0);
    ASSERT_EQ(dxl2_link_send_status(&link, 1U, DXL2_ERR_NONE, data, sizeof(data)), 0);

    EXPECT_EQ(capture.writes, 1);
    ASSERT_EQ(capture.length, sizeof(expected));
    EXPECT_EQ(std::memcmp(capture.data, expected, sizeof(expected)), 0);
}

/** @brief A receive-only link (no writer) cannot send. */
TEST(Dxl2LinkSend, ReceiveOnlyLinkRejectsSend)
{
    struct dxl2_link_config config = {};
    struct dxl2_link link;

    ASSERT_EQ(dxl2_link_init(&link, &config), 0);

    EXPECT_EQ(dxl2_link_send(&link, 1U, DXL2_INST_PING, nullptr, 0U), DXL2_PARSE_ERROR);
    EXPECT_EQ(dxl2_link_send(nullptr, 1U, DXL2_INST_PING, nullptr, 0U), DXL2_PARSE_ERROR);
}

/** @brief The writer's error code is returned to the caller. */
TEST(Dxl2LinkSend, PropagatesWriteError)
{
    struct dxl2_link_config config = {};
    struct dxl2_link link;

    config.write = failing_write;

    ASSERT_EQ(dxl2_link_init(&link, &config), 0);

    EXPECT_EQ(dxl2_link_send(&link, 1U, DXL2_INST_PING, nullptr, 0U), DXL2_TEST_WRITE_ERROR);
}

/** @brief Oversized or NULL status data is rejected before anything is written. */
TEST(Dxl2LinkSend, StatusRejectsInvalidData)
{
    static const uint8_t data[DXL2_MAX_PARAMETER_SIZE] = {0};
    WriteCapture capture = {};
    struct dxl2_link_config config = {};
    struct dxl2_link link;

    config.write = capture_write;
    config.write_context = &capture;

    ASSERT_EQ(dxl2_link_init(&link, &config), 0);

    EXPECT_EQ(dxl2_link_send_status(&link, 1U, 0U, data, sizeof(data)), DXL2_PARSE_ERROR);
    EXPECT_EQ(dxl2_link_send_status(&link, 1U, 0U, nullptr, 1U), DXL2_PARSE_ERROR);
    EXPECT_EQ(capture.writes, 0);
}

/** @brief An encoder error is returned and nothing is written. */
TEST(Dxl2LinkSend, PropagatesEncodeError)
{
    WriteCapture capture = {};
    struct dxl2_link_config config = {};
    struct dxl2_link link;

    config.write = capture_write;
    config.write_context = &capture;

    ASSERT_EQ(dxl2_link_init(&link, &config), 0);

    EXPECT_EQ(dxl2_link_send(&link, 1U, DXL2_INST_WRITE, nullptr, 1U), DXL2_PARSE_ERROR);
    EXPECT_EQ(capture.writes, 0);
}

/** @brief A status without data carries only the error byte. */
TEST(Dxl2LinkSend, StatusWithoutData)
{
    WriteCapture capture = {};
    struct dxl2_link_config config = {};
    struct dxl2_link link;

    config.write = capture_write;
    config.write_context = &capture;

    ASSERT_EQ(dxl2_link_init(&link, &config), 0);
    ASSERT_EQ(dxl2_link_send_status(&link, 1U, DXL2_ERR_INSTRUCTION, nullptr, 0U), 0);

    ASSERT_EQ(capture.length, 11U);
    EXPECT_EQ(capture.data[7], DXL2_INST_STATUS);
    EXPECT_EQ(capture.data[8], DXL2_ERR_INSTRUCTION);
}
