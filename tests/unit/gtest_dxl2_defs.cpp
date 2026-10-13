/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Unit tests for the little-endian field helpers
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
 * @file gtest_dxl2_defs.cpp
 * @brief Unit tests for dxl2_get_u16/u32 and dxl2_put_u16/u32.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <gtest/gtest.h>

#include "dxl2_test_support.h"

/* ==============================================================================
 * UNIT TEST IMPLEMENTATIONS
 * =============================================================================*/

/** @brief 16-bit values are written and read little endian. */
TEST(Dxl2Defs, U16LittleEndian)
{
    uint8_t buffer[2];

    dxl2_put_u16(buffer, 0x1234U);

    EXPECT_EQ(buffer[0], 0x34U);
    EXPECT_EQ(buffer[1], 0x12U);
    EXPECT_EQ(dxl2_get_u16(buffer), 0x1234U);
}

/** @brief 32-bit values are written and read little endian. */
TEST(Dxl2Defs, U32LittleEndian)
{
    uint8_t buffer[4];

    dxl2_put_u32(buffer, 0xA1B2C3D4UL);

    EXPECT_EQ(buffer[0], 0xD4U);
    EXPECT_EQ(buffer[3], 0xA1U);
    EXPECT_EQ(dxl2_get_u32(buffer), 0xA1B2C3D4UL);
}
