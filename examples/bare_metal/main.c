/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Bare-metal example: DYNAMIXEL device on a UART with an RX\ninterrupt, ring buffer
 *and main loop
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
 * @brief Bare-metal DYNAMIXEL device: UART RX interrupt, ring buffer, main loop.
 * @details The RX interrupt only stores bytes in a single-producer,
 *          single-consumer ring buffer. The main loop drains it into the link,
 *          and the packet handler answers PING, READ and WRITE for a small
 *          Control Table. Addresses below CONTROL_TABLE_RAM_START are read-only.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "dxl2.h"

/* ==============================================================================
 * DEFINES & MACROS
 * =============================================================================*/

#define DEVICE_ID 1U
#define DEVICE_BAUD_RATE 57600U
#define MODEL_NUMBER 1030U
#define FIRMWARE_VERSION 38U

#define CONTROL_TABLE_SIZE 128U
#define CONTROL_TABLE_RAM_START 64U
#define ADDR_MODEL_NUMBER 0U
#define ADDR_FIRMWARE_VERSION 6U
#define ADDR_ID 7U

/** Ring buffer size; a power of two that holds at least one maximum packet. */
#define RX_QUEUE_SIZE 512U

/* ==============================================================================
 * PRIVATE VARIABLES (static)
 * =============================================================================*/

static struct dxl2_link link;
static uint8_t control_table[CONTROL_TABLE_SIZE];

/*
 * The ISR writes rx_head, the main loop writes rx_tail. This is safe without
 * locking on a single-core MCU where 16-bit loads and stores are atomic
 * (any 32-bit MCU); on an 8-bit MCU, disable the RX interrupt around the reads.
 */
static volatile uint8_t rx_queue[RX_QUEUE_SIZE];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;
static volatile uint32_t rx_overruns;

/* ==============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

static int uart_write(void* context, const uint8_t* data, size_t length)
{
    (void) context;
    return board_uart_write(data, length);
}

static void reply(uint8_t error, const uint8_t* data, uint16_t length)
{
    (void) dxl2_link_send_status(&link, DEVICE_ID, error, data, length);
}

/* PING reply data: model number (2), firmware version (1). */
static void handle_ping(void)
{
    uint8_t info[3];

    dxl2_put_u16(&info[0], dxl2_get_u16(&control_table[ADDR_MODEL_NUMBER]));
    info[2] = control_table[ADDR_FIRMWARE_VERSION];
    reply(DXL2_ERR_NONE, info, sizeof(info));
}

/* READ: address (2), length (2). */
static void handle_read(const struct dxl2_packet* packet)
{
    if (packet->parameter_length != 4U)
    {
        reply(DXL2_ERR_DATA_LENGTH, NULL, 0U);
    }
    else
    {
        const uint16_t address = dxl2_get_u16(&packet->parameters[0]);
        const uint16_t length = dxl2_get_u16(&packet->parameters[2]);

        if (((uint32_t) address + length) > CONTROL_TABLE_SIZE)
        {
            reply(DXL2_ERR_DATA_RANGE, NULL, 0U);
        }
        else
        {
            reply(DXL2_ERR_NONE, &control_table[address], length);
        }
    }
}

/* WRITE: address (2), data (n). Broadcast writes are executed without a reply. */
static void handle_write(const struct dxl2_packet* packet, bool broadcast)
{
    uint8_t error = DXL2_ERR_NONE;

    if (packet->parameter_length < 3U)
    {
        error = DXL2_ERR_DATA_LENGTH;
    }
    else
    {
        const uint16_t address = dxl2_get_u16(&packet->parameters[0]);
        const uint16_t length = (uint16_t) (packet->parameter_length - 2U);

        if (((uint32_t) address + length) > CONTROL_TABLE_SIZE)
        {
            error = DXL2_ERR_DATA_RANGE;
        }
        else if (address < CONTROL_TABLE_RAM_START)
        {
            error = DXL2_ERR_ACCESS;
        }
        else
        {
            (void) memcpy(&control_table[address], &packet->parameters[2], length);
        }
    }

    if (!broadcast)
    {
        reply(error, NULL, 0U);
    }
}

static void on_packet(const struct dxl2_packet* packet, void* context)
{
    const bool broadcast = (packet->id == DXL2_BROADCAST_ID);

    (void) context;
    if ((packet->id == DEVICE_ID) || broadcast)
    {
        switch (packet->instruction)
        {
            case DXL2_INST_PING:
                /* Every device answers a broadcast PING. */
                handle_ping();
                break;
            case DXL2_INST_READ:
                if (!broadcast)
                {
                    handle_read(packet);
                }
                break;
            case DXL2_INST_WRITE:
                handle_write(packet, broadcast);
                break;
            case DXL2_INST_STATUS:
                /* Another device's reply on the shared bus. */
                break;
            default:
                if (!broadcast)
                {
                    reply(DXL2_ERR_INSTRUCTION, NULL, 0U);
                }
                break;
        }
    }
}

static void control_table_init(void)
{
    (void) memset(control_table, 0, sizeof(control_table));
    dxl2_put_u16(&control_table[ADDR_MODEL_NUMBER], MODEL_NUMBER);
    control_table[ADDR_FIRMWARE_VERSION] = FIRMWARE_VERSION;
    control_table[ADDR_ID] = DEVICE_ID;
}

/* ==============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

void app_uart_rx_isr(uint8_t byte)
{
    const uint16_t next = (uint16_t) ((rx_head + 1U) & (RX_QUEUE_SIZE - 1U));

    if (next == rx_tail)
    {
        rx_overruns++; /* The parser resynchronizes on the next header. */
    }
    else
    {
        rx_queue[rx_head] = byte;
        rx_head = next;
    }
}

int main(void)
{
    const struct dxl2_link_config config = {
        .write = uart_write,
        .handler = on_packet,
    };

    control_table_init();
    (void) dxl2_link_init(&link, &config);
    board_init(DEVICE_BAUD_RATE);

    for (;;)
    {
        if (rx_tail != rx_head)
        {
            const uint8_t byte = rx_queue[rx_tail];

            rx_tail = (uint16_t) ((rx_tail + 1U) & (RX_QUEUE_SIZE - 1U));
            dxl2_link_receive(&link, &byte, 1U);
        }
        else
        {
            board_idle();
        }
    }
}
