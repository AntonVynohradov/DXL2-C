/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Host example: PING DYNAMIXEL devices over a serial port
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
 * @file main.c
 * @brief Host example: PING DYNAMIXEL devices over a serial port.
 * @details Usage: dxl2_ping PORT BAUD_RATE ID, where BAUD_RATE (default 57600)
 *          and ID (default 254) are optional.
 *
 *          With the broadcast ID (254) every device on the bus answers, so the
 *          tool lists all of them. Received bytes go to the link as the port
 *          returns them; the packet handler prints each Status Packet.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <stdio.h>
#include <stdlib.h>

#include "dxl2.h"
#include "serial_port.h"

/* ==============================================================================
 * DEFINES & MACROS
 * =============================================================================*/

#define DEFAULT_BAUD_RATE 57600UL
#define READ_TIMEOUT_MS 50U
/** Stop after this many consecutive empty reads (bus silent). */
#define IDLE_READS 4

/* ==============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

static int port_write(void* context, const uint8_t* data, size_t length)
{
    (void) context;
    return serial_write(data, length);
}

static void on_status(const struct dxl2_packet* packet, void* context)
{
    int* devices = context;

    if (packet->instruction != DXL2_INST_STATUS)
    {
        return; /* our own request echoed by a half-duplex adapter */
    }
    if ((packet->parameter_length == 4U) && (packet->parameters[0] == DXL2_ERR_NONE))
    {
        printf("ID %3u: model %u, firmware %u\n", packet->id, dxl2_get_u16(&packet->parameters[1]),
               packet->parameters[3]);
    }
    else
    {
        printf("ID %3u: status error 0x%02X\n", packet->id, packet->parameters[0]);
    }
    (*devices)++;
}

static int parse_number(const char* text, unsigned long max, unsigned long* value)
{
    char* end = NULL;

    *value = strtoul(text, &end, 0);
    return ((end != text) && (*end == '\0') && (*value <= max)) ? 0 : -1;
}

/* ==============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

int main(int argc, char** argv)
{
    struct dxl2_link link;
    struct dxl2_link_config config = {0};
    unsigned long baud_rate = DEFAULT_BAUD_RATE;
    unsigned long id = DXL2_BROADCAST_ID;
    int devices = 0;
    int idle = 0;

    if ((argc < 2) || (argc > 4)
        || ((argc > 2) && (parse_number(argv[2], 0xFFFFFFFFUL, &baud_rate) != 0))
        || ((argc > 3) && (parse_number(argv[3], DXL2_BROADCAST_ID, &id) != 0)))
    {
        fprintf(stderr, "usage: %s <port> [baud rate, default 57600] [ID, default 254]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (serial_open(argv[1], (uint32_t) baud_rate) != 0)
    {
        fprintf(stderr, "cannot open %s at %lu bit/s\n", argv[1], baud_rate);
        return EXIT_FAILURE;
    }

    config.write = port_write;
    config.handler = on_status;
    config.handler_context = &devices;
    (void) dxl2_link_init(&link, &config);

    if (dxl2_link_send(&link, (uint8_t) id, DXL2_INST_PING, NULL, 0U) != 0)
    {
        fprintf(stderr, "write failed\n");
        serial_close();
        return EXIT_FAILURE;
    }

    while (idle < IDLE_READS)
    {
        uint8_t bytes[64];
        const int count = serial_read(bytes, sizeof(bytes), READ_TIMEOUT_MS);

        if (count < 0)
        {
            fprintf(stderr, "read failed\n");
            break;
        }
        idle = (count == 0) ? (idle + 1) : 0;
        dxl2_link_receive(&link, bytes, (size_t) count);
    }
    serial_close();

    printf("%d device(s) answered, %lu invalid packet(s)\n", devices,
           (unsigned long) dxl2_link_invalid_packets(&link));
    return (devices > 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
