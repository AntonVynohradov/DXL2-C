/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Zephyr example: DYNAMIXEL device on a UART with an RX\ninterrupt, message queue and
 *RX thread
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
 * @brief Zephyr DYNAMIXEL device: UART RX interrupt, byte queue and RX thread.
 * @details The ISR only queues bytes; the RX thread owns the link and runs
 *          the packet handler, which answers PING for DEVICE_ID. The UART is
 *          the devicetree chosen node dxl2,uart (see app.overlay.example).
 *          Disable console and logging on that UART: text corrupts the framing.
 */

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include <errno.h>

#include "dxl2.h"

/* ==============================================================================
 * DEFINES & MACROS
 * =============================================================================*/

#define DEVICE_ID 1U
#define MODEL_NUMBER 1030U
#define FIRMWARE_VERSION 38U

#define RX_QUEUE_SIZE 512
#define RX_STACK_SIZE 2048
#define RX_PRIORITY K_PRIO_PREEMPT(5)

/* ==============================================================================
 * PRIVATE VARIABLES (static)
 * =============================================================================*/

K_MSGQ_DEFINE(rx_queue, sizeof(uint8_t), RX_QUEUE_SIZE, 1);
K_MUTEX_DEFINE(tx_mutex);

static const struct device* const dxl2_uart = DEVICE_DT_GET(DT_CHOSEN(dxl2_uart));
static struct dxl2_link link;
static uint32_t rx_overruns;

/* ==============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

static void uart_rx_isr(const struct device* dev, void* user_data)
{
    ARG_UNUSED(user_data);

    while ((uart_irq_update(dev) > 0) && (uart_irq_rx_ready(dev) > 0))
    {
        uint8_t bytes[32];
        const int count = uart_fifo_read(dev, bytes, sizeof(bytes));

        if (count <= 0)
        {
            break;
        }
        for (int i = 0; i < count; ++i)
        {
            if (k_msgq_put(&rx_queue, &bytes[i], K_NO_WAIT) != 0)
            {
                rx_overruns++; /* The parser resynchronizes on the next header. */
            }
        }
    }
}

/* Blocking polled TX; at 57600 bit/s a 256-byte packet takes about 45 ms. */
static int uart_write(void* context, const uint8_t* data, size_t length)
{
    ARG_UNUSED(context);

    (void) k_mutex_lock(&tx_mutex, K_FOREVER);
    for (size_t i = 0U; i < length; ++i)
    {
        uart_poll_out(dxl2_uart, data[i]);
    }
    (void) k_mutex_unlock(&tx_mutex);
    return 0;
}

static void on_packet(const struct dxl2_packet* packet, void* context)
{
    ARG_UNUSED(context);

    if (((packet->id == DEVICE_ID) || (packet->id == DXL2_BROADCAST_ID))
        && (packet->instruction == DXL2_INST_PING))
    {
        uint8_t info[3];

        dxl2_put_u16(&info[0], MODEL_NUMBER);
        info[2] = FIRMWARE_VERSION;
        (void) dxl2_link_send_status(&link, DEVICE_ID, DXL2_ERR_NONE, info, sizeof(info));
    }
    /* Add READ / WRITE for the device's Control Table here (see examples/bare_metal). */
}

static void rx_thread(void* p1, void* p2, void* p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    for (;;)
    {
        uint8_t byte;

        if (k_msgq_get(&rx_queue, &byte, K_FOREVER) == 0)
        {
            dxl2_link_receive(&link, &byte, 1U);
        }
    }
}

K_THREAD_DEFINE(dxl2_rx, RX_STACK_SIZE, rx_thread, NULL, NULL, NULL, RX_PRIORITY, 0, 0);

/* ==============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

int main(void)
{
    const struct dxl2_link_config config = {
        .write = uart_write,
        .handler = on_packet,
    };

    if (!device_is_ready(dxl2_uart))
    {
        return -ENODEV;
    }
    (void) dxl2_link_init(&link, &config);
    if (uart_irq_callback_user_data_set(dxl2_uart, uart_rx_isr, NULL) != 0)
    {
        return -EIO;
    }
    uart_irq_rx_enable(dxl2_uart);
    return 0;
}
