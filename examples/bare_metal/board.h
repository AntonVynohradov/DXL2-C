/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Board support interface of the bare-metal example
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
 * @file board.h
 * @brief Board support interface of the bare-metal example.
 * @details Implement these functions for the target MCU (UART driver, sleep).
 *          board_host.c implements them on a PC so the example runs in CI.
 */

#ifndef BOARD_H
#define BOARD_H

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <stddef.h>
#include <stdint.h>

/* ==============================================================================
 * PUBLIC FUNCTION DECLARATIONS
 * =============================================================================*/

/**
 * @brief   Configure the UART and enable its RX interrupt.
 * @param   baud_rate - UART bit rate
 */
void board_init(uint32_t baud_rate);

/**
 * @brief   Transmit bytes and wait until the last one has left the shift register.
 * @details On a half-duplex bus this is also where the driver switches the
 *          transceiver back to receive.
 * @param   data - Bytes to send
 * @param   length - Number of bytes
 * @return  0 on success, negative on error
 */
int board_uart_write(const uint8_t* data, size_t length);

/**
 * @brief   Wait for the next interrupt (for example WFI on Cortex-M).
 */
void board_idle(void);

/**
 * @brief   Called by the board's UART RX interrupt for every received byte.
 * @details Implemented by the application (main.c).
 * @param   byte - Received byte
 */
void app_uart_rx_isr(uint8_t byte);

#endif /* BOARD_H */
