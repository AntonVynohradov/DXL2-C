/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Transport and handler mocks shared by the unit tests
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
 * @file dxl2_test_mocks.cpp
 * @brief Mock implementations for unit tests.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <gtest/gtest.h>

#include "dxl2_test_support.h"

/* ==============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

int capture_write(void* context, const uint8_t* data, size_t length)
{
    WriteCapture* capture = static_cast<WriteCapture*>(context);

    EXPECT_LE(length, sizeof(capture->data));
    std::memcpy(capture->data, data, length);
    capture->length = length;
    capture->writes++;
    return 0;
}

int failing_write(void* context, const uint8_t* data, size_t length)
{
    (void) context;
    (void) data;
    (void) length;
    return DXL2_TEST_WRITE_ERROR;
}

void record_packet(const struct dxl2_packet* packet, void* context)
{
    ReceivedPackets* received = static_cast<ReceivedPackets*>(context);

    ASSERT_LT(received->count, DXL2_TEST_MAX_RECORDED);
    received->ids[received->count] = packet->id;
    received->instructions[received->count] = packet->instruction;
    received->count++;
}

int feed_all(struct dxl2_parser* parser, const uint8_t* data, size_t length,
             struct dxl2_packet* packet)
{
    int result = 0;

    for (size_t i = 0U; i < length; ++i)
    {
        result = dxl2_parser_feed(parser, data[i], packet);
    }
    return result;
}
