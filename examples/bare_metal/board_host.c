/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Host simulation of the bare-metal board: drives the example\nas a self-checking test
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
 * @file board_host.c
 * @brief Host implementation of board.h that runs the bare-metal example as a test.
 * @details board_idle() plays the role of the host controller: it injects the
 *          next request through app_uart_rx_isr(), as the UART interrupt
 *          would, and checks the reply captured by board_uart_write(). The
 *          process exits with 0 when every reply matches.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "dxl2.h"

/* ==============================================================================
 * PRIVATE TYPE DEFINITIONS
 * =============================================================================*/

struct request
{
    const char* name;
    uint8_t id;
    uint8_t instruction;
    uint8_t parameters[8];
    uint16_t parameter_length;
    int expect_reply;
    uint8_t error;
    uint8_t data[4];
    uint16_t data_length;
};

/* ==============================================================================
 * PRIVATE VARIABLES (static)
 * =============================================================================*/

static const struct request requests[] = {
    {"PING", 1U, DXL2_INST_PING, {0}, 0U, 1, DXL2_ERR_NONE, {0x06, 0x04, 0x26}, 3U},
    {"broadcast PING",
     DXL2_BROADCAST_ID,
     DXL2_INST_PING,
     {0},
     0U,
     1,
     DXL2_ERR_NONE,
     {0x06, 0x04, 0x26},
     3U},
    {"PING to another ID", 2U, DXL2_INST_PING, {0}, 0U, 0, 0U, {0}, 0U},
    {"READ model number",
     1U,
     DXL2_INST_READ,
     {0x00, 0x00, 0x02, 0x00},
     4U,
     1,
     DXL2_ERR_NONE,
     {0x06, 0x04},
     2U},
    {"WRITE RAM", 1U, DXL2_INST_WRITE, {0x40, 0x00, 0xAB, 0xCD}, 4U, 1, DXL2_ERR_NONE, {0}, 0U},
    {"READ RAM",
     1U,
     DXL2_INST_READ,
     {0x40, 0x00, 0x02, 0x00},
     4U,
     1,
     DXL2_ERR_NONE,
     {0xAB, 0xCD},
     2U},
    {"broadcast WRITE", DXL2_BROADCAST_ID, DXL2_INST_WRITE, {0x42, 0x00, 0x11}, 3U, 0, 0U, {0}, 0U},
    {"READ after broadcast WRITE",
     1U,
     DXL2_INST_READ,
     {0x42, 0x00, 0x01, 0x00},
     4U,
     1,
     DXL2_ERR_NONE,
     {0x11},
     1U},
    {"WRITE read-only ID",
     1U,
     DXL2_INST_WRITE,
     {0x07, 0x00, 0x05},
     3U,
     1,
     DXL2_ERR_ACCESS,
     {0},
     0U},
    {"READ out of range",
     1U,
     DXL2_INST_READ,
     {0x7F, 0x00, 0x02, 0x00},
     4U,
     1,
     DXL2_ERR_DATA_RANGE,
     {0},
     0U},
    {"READ with bad length",
     1U,
     DXL2_INST_READ,
     {0x00, 0x00},
     2U,
     1,
     DXL2_ERR_DATA_LENGTH,
     {0},
     0U},
    {"unsupported instruction", 1U, DXL2_INST_REBOOT, {0}, 0U, 1, DXL2_ERR_INSTRUCTION, {0}, 0U},
};

static size_t next_request;
static int replies;
static struct dxl2_packet reply_packet;
static int failures;

/* ==============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

static void check_previous_reply(void)
{
    const struct request* request = &requests[next_request - 1U];
    int ok = (replies == request->expect_reply);

    if (ok && (request->expect_reply != 0))
    {
        ok = (reply_packet.id == 1U) && (reply_packet.instruction == DXL2_INST_STATUS)
             && (reply_packet.parameter_length == (uint16_t) (request->data_length + 1U))
             && (reply_packet.parameters[0] == request->error)
             && (memcmp(&reply_packet.parameters[1], request->data, request->data_length) == 0);
    }
    printf("%-28s %s\n", request->name, ok ? "ok" : "FAILED");
    failures += ok ? 0 : 1;
}

static void send_request(const struct request* request)
{
    static const uint8_t noise[] = {0x00, 0xFF, 0xFF, 0x13};
    uint8_t packet[DXL2_MAX_PACKET_SIZE];
    size_t length = 0U;

    if (dxl2_encode(request->id, request->instruction, request->parameters,
                    request->parameter_length, packet, sizeof(packet), &length)
        != 0)
    {
        printf("cannot encode %s\n", request->name);
        exit(EXIT_FAILURE);
    }

    /* Line noise before each request: the parser must resynchronize. */
    for (size_t i = 0U; i < sizeof(noise); ++i)
    {
        app_uart_rx_isr(noise[i]);
    }
    for (size_t i = 0U; i < length; ++i)
    {
        app_uart_rx_isr(packet[i]);
    }
    replies = 0;
}

/* ==============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

void board_init(uint32_t baud_rate)
{
    printf("bare-metal example on the host simulator, %u bit/s\n", (unsigned) baud_rate);
}

int board_uart_write(const uint8_t* data, size_t length)
{
    struct dxl2_parser parser;
    int result = 0;

    dxl2_parser_init(&parser);
    for (size_t i = 0U; i < length; ++i)
    {
        result = dxl2_parser_feed(&parser, data[i], &reply_packet);
    }
    replies += (result == 1) ? 1 : 0;
    return 0;
}

void board_idle(void)
{
    if (next_request > 0U)
    {
        check_previous_reply();
    }

    if (next_request < (sizeof(requests) / sizeof(requests[0])))
    {
        send_request(&requests[next_request]);
        next_request++;
    }
    else
    {
        printf("%d failure(s)\n", failures);
        exit((failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
    }
}
