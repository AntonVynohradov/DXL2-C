/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Unit tests for dxl2_link_init and the link counters
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
 * @file gtest_dxl2_link_init.cpp
 * @brief Unit tests for dxl2_link_init.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <gtest/gtest.h>

#include "dxl2_test_support.h"

/* ==============================================================================
 * UNIT TEST IMPLEMENTATIONS
 * =============================================================================*/

/** @brief Rejects NULL link or configuration. */
TEST(Dxl2LinkInit, RejectsNullArguments)
{
    struct dxl2_link_config config = {};
    struct dxl2_link link;

    EXPECT_EQ(dxl2_link_init(nullptr, &config), DXL2_PARSE_ERROR);
    EXPECT_EQ(dxl2_link_init(&link, nullptr), DXL2_PARSE_ERROR);
}

/** @brief Clears counters and copies the configuration. */
TEST(Dxl2LinkInit, ResetsState)
{
    WriteCapture capture = {};
    struct dxl2_link_config config = {};
    struct dxl2_link link;

    config.write = capture_write;
    config.write_context = &capture;
    std::memset(&link, 0xA5, sizeof(link));

    ASSERT_EQ(dxl2_link_init(&link, &config), 0);
    EXPECT_EQ(dxl2_link_rx_packets(&link), 0U);
    EXPECT_EQ(dxl2_link_invalid_packets(&link), 0U);
    EXPECT_EQ(link.config.write, capture_write);
    EXPECT_EQ(link.config.write_context, &capture);
    EXPECT_EQ(link.parser.length, 0U);
}

/** @brief Counters of a NULL link read as zero. */
TEST(Dxl2LinkInit, NullLinkCountersAreZero)
{
    EXPECT_EQ(dxl2_link_rx_packets(nullptr), 0U);
    EXPECT_EQ(dxl2_link_invalid_packets(nullptr), 0U);
}
