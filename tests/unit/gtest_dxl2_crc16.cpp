/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Unit tests for dxl2_crc16
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
 * @file gtest_dxl2_crc16.cpp
 * @brief Unit tests for dxl2_crc16.
 * @details Verifies the CRC against the ROBOTIS reference packet.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <gtest/gtest.h>

#include "dxl2_test_support.h"

/* ==============================================================================
 * UNIT TEST IMPLEMENTATIONS
 * =============================================================================*/

/** @brief CRC of the ROBOTIS PING reference packet is 0x4E19. */
TEST(Dxl2Crc16, MatchesPingReference)
{
    static const uint8_t ping[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x03, 0x00, 0x01};

    EXPECT_EQ(dxl2_crc16(ping, sizeof(ping)), 0x4E19U);
}

/** @brief Empty input yields the initial CRC value. */
TEST(Dxl2Crc16, EmptyInputIsZero)
{
    EXPECT_EQ(dxl2_crc16(nullptr, 0U), 0U);
}

/** @brief A NULL buffer is not dereferenced, whatever the length. */
TEST(Dxl2Crc16, NullInputIsZero)
{
    EXPECT_EQ(dxl2_crc16(nullptr, 8U), 0U);
}
