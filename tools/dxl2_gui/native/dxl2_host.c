/**
 * @file dxl2_host.c
 * @brief Host (controller) side of the DXL2 link, for the GUI tool.
 *
 * Project: DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Author: Anton Vynohradov
 * Email: avynohradov@systemfromscratch.com
 *
 * @copyright Copyright (c) 2026 Anton Vynohradov
 * @license Apache License 2.0
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
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <string.h>

#include "dxl2_host.h"

/* ==============================================================================
 * PRIVATE FUNCTION DECLARATIONS (static)
 * =============================================================================*/

static int host_write_bytes(void* context, const uint8_t* data, size_t length);
static void host_handle_packet(const struct dxl2_packet* packet, void* context);

/* ==============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

/** @brief Link writer: copies the encoded packet into the build buffer. */
static int host_write_bytes(void* context, const uint8_t* data, size_t length)
{
    struct dxl2_host* host = (struct dxl2_host*) context;

    if (host->tx == NULL || length > host->tx_capacity)
    {
        return DXL2_PARSE_ERROR;
    }

    memcpy(host->tx, data, length);
    host->tx_length = length;

    return 0;
}

/** @brief Link handler: appends a decoded packet to the queue. */
static void host_handle_packet(const struct dxl2_packet* packet, void* context)
{
    struct dxl2_host* host = (struct dxl2_host*) context;

    if (host->count == DXL2_HOST_RX_QUEUE)
    {
        host->head = (host->head + 1U) % DXL2_HOST_RX_QUEUE;
        host->count--;
        host->dropped++;
    }

    host->queue[(host->head + host->count) % DXL2_HOST_RX_QUEUE] = *packet;
    host->count++;
}

/* ==============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

size_t dxl2_host_size(void)
{
    return sizeof(struct dxl2_host);
}

size_t dxl2_host_max_packet_size(void)
{
    return DXL2_MAX_PACKET_SIZE;
}

int dxl2_host_init(struct dxl2_host* host)
{
    struct dxl2_link_config config;

    if (host == NULL)
    {
        return -1;
    }

    memset(host, 0, sizeof(*host));

    config.write = host_write_bytes;
    config.write_context = host;
    config.handler = host_handle_packet;
    config.handler_context = host;

    return dxl2_link_init(&host->link, &config) == 0 ? 0 : -1;
}

int dxl2_host_build(struct dxl2_host* host, uint8_t id, uint8_t instruction,
                    const uint8_t* parameters, uint16_t parameter_length, uint8_t* output,
                    size_t output_capacity)
{
    int result;

    if (host == NULL || output == NULL)
    {
        return -1;
    }

    host->tx = output;
    host->tx_capacity = output_capacity;
    host->tx_length = 0U;

    result = dxl2_link_send(&host->link, id, instruction, parameters, parameter_length);

    host->tx = NULL;
    host->tx_capacity = 0U;

    return result == 0 ? (int) host->tx_length : -1;
}

size_t dxl2_host_receive(struct dxl2_host* host, const uint8_t* data, size_t length)
{
    if (host == NULL)
    {
        return 0U;
    }

    if (data != NULL && length > 0U)
    {
        dxl2_link_receive(&host->link, data, length);
    }

    return host->count;
}

int dxl2_host_take(struct dxl2_host* host, uint8_t* id, uint8_t* instruction, uint8_t* parameters,
                   size_t capacity)
{
    const struct dxl2_packet* packet;
    size_t length;

    if (host == NULL || id == NULL || instruction == NULL || host->count == 0U)
    {
        return -1;
    }

    packet = &host->queue[host->head];
    *id = packet->id;
    *instruction = packet->instruction;

    length = packet->parameter_length < capacity ? packet->parameter_length : capacity;
    if (parameters != NULL && length > 0U)
    {
        memcpy(parameters, packet->parameters, length);
    }

    host->head = (host->head + 1U) % DXL2_HOST_RX_QUEUE;
    host->count--;

    return parameters != NULL ? (int) length : 0;
}

void dxl2_host_flush(struct dxl2_host* host)
{
    if (host != NULL)
    {
        host->head = 0U;
        host->count = 0U;
        dxl2_parser_init(&host->link.parser);
    }
}

uint32_t dxl2_host_rx_packets(const struct dxl2_host* host)
{
    return host != NULL ? dxl2_link_rx_packets(&host->link) : 0U;
}

uint32_t dxl2_host_invalid_packets(const struct dxl2_host* host)
{
    return host != NULL ? dxl2_link_invalid_packets(&host->link) : 0U;
}

uint16_t dxl2_host_crc16(const uint8_t* data, size_t length)
{
    return dxl2_crc16(data, length);
}
