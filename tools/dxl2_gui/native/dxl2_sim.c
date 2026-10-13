/**
 * @file dxl2_sim.c
 * @brief Simulated DYNAMIXEL device built on the DXL2 link, for host tools.
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

#include "dxl2_sim.h"

/* ==============================================================================
 * DEFINES & MACROS
 * =============================================================================*/

/* Control Table addresses (XL430-W250) */
#define SIM_ADDR_MODEL_NUMBER 0U
#define SIM_ADDR_FIRMWARE_VERSION 6U
#define SIM_ADDR_ID 7U
#define SIM_ADDR_BAUD_RATE 8U
#define SIM_ADDR_RETURN_DELAY 9U
#define SIM_ADDR_OPERATING_MODE 11U
#define SIM_ADDR_PROTOCOL_TYPE 13U
#define SIM_ADDR_TEMPERATURE_LIMIT 31U
#define SIM_ADDR_MAX_VOLTAGE 32U
#define SIM_ADDR_MIN_VOLTAGE 34U
#define SIM_ADDR_PWM_LIMIT 36U
#define SIM_ADDR_VELOCITY_LIMIT 44U
#define SIM_ADDR_MAX_POSITION 48U
#define SIM_ADDR_MIN_POSITION 52U
#define SIM_ADDR_SHUTDOWN 63U
#define SIM_ADDR_TORQUE_ENABLE 64U
#define SIM_ADDR_LED 65U
#define SIM_ADDR_STATUS_RETURN 68U
#define SIM_ADDR_REGISTERED 69U
#define SIM_ADDR_HARDWARE_ERROR 70U
#define SIM_ADDR_VELOCITY_I 76U
#define SIM_ADDR_VELOCITY_P 78U
#define SIM_ADDR_POSITION_P 84U
#define SIM_ADDR_PROFILE_VELOCITY 112U
#define SIM_ADDR_GOAL_POSITION 116U
#define SIM_ADDR_REALTIME_TICK 120U
#define SIM_ADDR_MOVING 122U
#define SIM_ADDR_PRESENT_VELOCITY 128U
#define SIM_ADDR_PRESENT_POSITION 132U
#define SIM_ADDR_PRESENT_VOLTAGE 144U
#define SIM_ADDR_PRESENT_TEMPERATURE 146U

/** First RAM address; EEPROM items below it are locked while torque is on. */
#define SIM_RAM_START 64U

/** Position change per @ref dxl2_sim_tick when Profile Velocity is zero. */
#define SIM_DEFAULT_STEP 40

/** Parameter value of FACTORY_RESET / CLEAR selecting the full reset. */
#define SIM_RESET_ALL 0xFFU
#define SIM_RESET_EXCEPT_ID 0x01U
#define SIM_RESET_EXCEPT_ID_BAUD 0x02U

/* ==============================================================================
 * PRIVATE TYPE DEFINITIONS
 * =============================================================================*/

/** @brief One Control Table item description. */
struct sim_item
{
    uint16_t address; /**< Item address. */
    uint8_t size;     /**< Item size in bytes. */
    uint8_t writable; /**< 1 for read/write items. */
    const char* name; /**< Item name as in the e-Manual. */
};

/* ==============================================================================
 * PRIVATE VARIABLES (static)
 * =============================================================================*/

static const struct sim_item sim_items[] = {
    {0U, 2U, 0U, "Model Number"},
    {2U, 4U, 0U, "Model Information"},
    {6U, 1U, 0U, "Firmware Version"},
    {7U, 1U, 1U, "ID"},
    {8U, 1U, 1U, "Baud Rate"},
    {9U, 1U, 1U, "Return Delay Time"},
    {10U, 1U, 1U, "Drive Mode"},
    {11U, 1U, 1U, "Operating Mode"},
    {12U, 1U, 1U, "Secondary ID"},
    {13U, 1U, 1U, "Protocol Type"},
    {20U, 4U, 1U, "Homing Offset"},
    {24U, 4U, 1U, "Moving Threshold"},
    {31U, 1U, 1U, "Temperature Limit"},
    {32U, 2U, 1U, "Max Voltage Limit"},
    {34U, 2U, 1U, "Min Voltage Limit"},
    {36U, 2U, 1U, "PWM Limit"},
    {44U, 4U, 1U, "Velocity Limit"},
    {48U, 4U, 1U, "Max Position Limit"},
    {52U, 4U, 1U, "Min Position Limit"},
    {63U, 1U, 1U, "Shutdown"},
    {64U, 1U, 1U, "Torque Enable"},
    {65U, 1U, 1U, "LED"},
    {68U, 1U, 1U, "Status Return Level"},
    {69U, 1U, 0U, "Registered Instruction"},
    {70U, 1U, 0U, "Hardware Error Status"},
    {76U, 2U, 1U, "Velocity I Gain"},
    {78U, 2U, 1U, "Velocity P Gain"},
    {80U, 2U, 1U, "Position D Gain"},
    {82U, 2U, 1U, "Position I Gain"},
    {84U, 2U, 1U, "Position P Gain"},
    {88U, 2U, 1U, "Feedforward 2nd Gain"},
    {90U, 2U, 1U, "Feedforward 1st Gain"},
    {98U, 1U, 1U, "Bus Watchdog"},
    {100U, 2U, 1U, "Goal PWM"},
    {104U, 4U, 1U, "Goal Velocity"},
    {108U, 4U, 1U, "Profile Acceleration"},
    {112U, 4U, 1U, "Profile Velocity"},
    {116U, 4U, 1U, "Goal Position"},
    {120U, 2U, 0U, "Realtime Tick"},
    {122U, 1U, 0U, "Moving"},
    {123U, 1U, 0U, "Moving Status"},
    {124U, 2U, 0U, "Present PWM"},
    {126U, 2U, 0U, "Present Load"},
    {128U, 4U, 0U, "Present Velocity"},
    {132U, 4U, 0U, "Present Position"},
    {136U, 4U, 0U, "Velocity Trajectory"},
    {140U, 4U, 0U, "Position Trajectory"},
    {144U, 2U, 0U, "Present Input Voltage"},
    {146U, 1U, 0U, "Present Temperature"},
};

#define SIM_ITEM_COUNT (sizeof(sim_items) / sizeof(sim_items[0]))

/* ==============================================================================
 * PRIVATE FUNCTION DECLARATIONS (static)
 * =============================================================================*/

static int sim_write_bytes(void* context, const uint8_t* data, size_t length);
static void sim_handle_packet(const struct dxl2_packet* packet, void* context);
static void sim_factory_defaults(struct dxl2_sim* sim, uint8_t id);
static int sim_find_item(uint16_t address);
static uint8_t sim_check_write(const struct dxl2_sim* sim, uint16_t address, uint16_t length,
                               const uint8_t* data);
static void sim_apply_write(struct dxl2_sim* sim, uint16_t address, uint16_t length,
                            const uint8_t* data);
static uint8_t sim_check_read(uint16_t address, uint16_t length);
static void sim_reply(struct dxl2_sim* sim, uint8_t instruction, uint8_t error, const uint8_t* data,
                      uint16_t length);

/* ==============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

/** @brief Link writer: collects the encoded Status Packets in the device buffer. */
static int sim_write_bytes(void* context, const uint8_t* data, size_t length)
{
    struct dxl2_sim* sim = (struct dxl2_sim*) context;

    if (length > sizeof(sim->tx) - sim->tx_length)
    {
        return DXL2_PARSE_ERROR;
    }

    memcpy(&sim->tx[sim->tx_length], data, length);
    sim->tx_length += length;

    return 0;
}

/** @brief Load the factory defaults of every Control Table item. */
static void sim_factory_defaults(struct dxl2_sim* sim, uint8_t id)
{
    memset(sim->table, 0, sizeof(sim->table));

    dxl2_put_u16(&sim->table[SIM_ADDR_MODEL_NUMBER], DXL2_SIM_MODEL_NUMBER);
    sim->table[SIM_ADDR_FIRMWARE_VERSION] = DXL2_SIM_FIRMWARE_VERSION;
    sim->table[SIM_ADDR_ID] = id;
    sim->table[SIM_ADDR_BAUD_RATE] = 1U;
    sim->table[SIM_ADDR_RETURN_DELAY] = 250U;
    sim->table[SIM_ADDR_OPERATING_MODE] = 3U;
    sim->table[SIM_ADDR_PROTOCOL_TYPE] = 2U;
    sim->table[SIM_ADDR_TEMPERATURE_LIMIT] = 72U;
    dxl2_put_u16(&sim->table[SIM_ADDR_MAX_VOLTAGE], 140U);
    dxl2_put_u16(&sim->table[SIM_ADDR_MIN_VOLTAGE], 60U);
    dxl2_put_u16(&sim->table[SIM_ADDR_PWM_LIMIT], 885U);
    dxl2_put_u32(&sim->table[SIM_ADDR_VELOCITY_LIMIT], 230U);
    dxl2_put_u32(&sim->table[SIM_ADDR_MAX_POSITION], 4095U);
    dxl2_put_u32(&sim->table[SIM_ADDR_MIN_POSITION], 0U);
    sim->table[SIM_ADDR_SHUTDOWN] = 52U;
    sim->table[SIM_ADDR_STATUS_RETURN] = 2U;
    dxl2_put_u16(&sim->table[SIM_ADDR_VELOCITY_I], 1920U);
    dxl2_put_u16(&sim->table[SIM_ADDR_VELOCITY_P], 100U);
    dxl2_put_u16(&sim->table[SIM_ADDR_POSITION_P], 800U);
    dxl2_put_u32(&sim->table[SIM_ADDR_GOAL_POSITION], 2048U);
    dxl2_put_u32(&sim->table[SIM_ADDR_PRESENT_POSITION], 2048U);
    dxl2_put_u16(&sim->table[SIM_ADDR_PRESENT_VOLTAGE], 120U);
    sim->table[SIM_ADDR_PRESENT_TEMPERATURE] = 30U;

    sim->pending_valid = false;
}

/** @brief Index of the item containing @p address, or -1 for an unused byte. */
static int sim_find_item(uint16_t address)
{
    for (size_t i = 0U; i < SIM_ITEM_COUNT; ++i)
    {
        if (address >= sim_items[i].address
            && address < (uint16_t) (sim_items[i].address + sim_items[i].size))
        {
            return (int) i;
        }
    }

    return -1;
}

/** @brief Status error for a READ of @p length bytes at @p address. */
static uint8_t sim_check_read(uint16_t address, uint16_t length)
{
    if (length == 0U || length > DXL2_MAX_PARAMETER_SIZE - 1U)
    {
        return DXL2_ERR_DATA_LENGTH;
    }

    if ((uint32_t) address + length > DXL2_SIM_TABLE_SIZE)
    {
        return DXL2_ERR_ACCESS;
    }

    return DXL2_ERR_NONE;
}

/** @brief Status error for a WRITE of @p data to @p address (no side effects). */
static uint8_t sim_check_write(const struct dxl2_sim* sim, uint16_t address, uint16_t length,
                               const uint8_t* data)
{
    if (length == 0U)
    {
        return DXL2_ERR_DATA_LENGTH;
    }

    if ((uint32_t) address + length > DXL2_SIM_TABLE_SIZE)
    {
        return DXL2_ERR_ACCESS;
    }

    for (uint16_t i = 0U; i < length; ++i)
    {
        uint16_t target = (uint16_t) (address + i);
        int item = sim_find_item(target);

        if (item < 0 || sim_items[item].writable == 0U)
        {
            return DXL2_ERR_ACCESS;
        }

        /* EEPROM area is locked while the torque is enabled */
        if (target < SIM_RAM_START && sim->table[SIM_ADDR_TORQUE_ENABLE] != 0U)
        {
            return DXL2_ERR_ACCESS;
        }

        if (target == SIM_ADDR_ID && data[i] > DXL2_MAX_ID)
        {
            return DXL2_ERR_DATA_RANGE;
        }

        if ((target == SIM_ADDR_TORQUE_ENABLE || target == SIM_ADDR_LED) && data[i] > 1U)
        {
            return DXL2_ERR_DATA_RANGE;
        }

        if (target == SIM_ADDR_STATUS_RETURN && data[i] > 2U)
        {
            return DXL2_ERR_DATA_RANGE;
        }
    }

    return DXL2_ERR_NONE;
}

/** @brief Copy @p data into the Control Table (already validated). */
static void sim_apply_write(struct dxl2_sim* sim, uint16_t address, uint16_t length,
                            const uint8_t* data)
{
    memcpy(&sim->table[address], data, length);
}

/**
 * @brief Send a Status Packet, honoring Status Return Level and broadcast rules.
 *
 * @details PING is always answered; READ-type instructions from level 1;
 *          everything else only at level 2.
 */
static void sim_reply(struct dxl2_sim* sim, uint8_t instruction, uint8_t error, const uint8_t* data,
                      uint16_t length)
{
    uint8_t level = sim->table[SIM_ADDR_STATUS_RETURN];
    bool is_read = instruction == DXL2_INST_READ || instruction == DXL2_INST_SYNC_READ
                   || instruction == DXL2_INST_BULK_READ;

    if (instruction != DXL2_INST_PING && (level == 0U || (level == 1U && !is_read)))
    {
        return;
    }

    if (sim->table[SIM_ADDR_HARDWARE_ERROR] != 0U)
    {
        error |= DXL2_ERR_ALERT_BIT;
    }

    (void) dxl2_link_send_status(&sim->link, sim->table[SIM_ADDR_ID], error, data, length);
}

/** @brief Link handler: executes one Instruction Packet addressed to the device. */
static void sim_handle_packet(const struct dxl2_packet* packet, void* context)
{
    struct dxl2_sim* sim = (struct dxl2_sim*) context;
    const uint8_t* p = packet->parameters;
    uint16_t n = packet->parameter_length;
    uint8_t own_id = sim->table[SIM_ADDR_ID];
    bool broadcast = packet->id == DXL2_BROADCAST_ID;
    bool addressed = broadcast || packet->id == own_id;
    uint8_t data[DXL2_MAX_PARAMETER_SIZE];
    uint8_t error;

    if (packet->instruction == DXL2_INST_STATUS)
    {
        return; /* Status Packets of other devices on the bus */
    }

    /* Sync/Bulk instructions are sent to the broadcast ID and name devices inside */
    switch (packet->instruction)
    {
        case DXL2_INST_SYNC_READ:
            if (broadcast && n >= 4U)
            {
                uint16_t address = dxl2_get_u16(&p[0]);
                uint16_t length = dxl2_get_u16(&p[2]);

                for (uint16_t i = 4U; i < n; ++i)
                {
                    if (p[i] == own_id)
                    {
                        error = sim_check_read(address, length);
                        if (error == DXL2_ERR_NONE)
                        {
                            sim_reply(sim, packet->instruction, error, &sim->table[address],
                                      length);
                        }
                        else
                        {
                            sim_reply(sim, packet->instruction, error, NULL, 0U);
                        }
                    }
                }
            }
            return;

        case DXL2_INST_SYNC_WRITE:
            if (broadcast && n >= 4U)
            {
                uint16_t address = dxl2_get_u16(&p[0]);
                uint16_t length = dxl2_get_u16(&p[2]);
                uint16_t i = 4U;

                while (length > 0U && (uint32_t) i + 1U + length <= n)
                {
                    if (p[i] == own_id
                        && sim_check_write(sim, address, length, &p[i + 1U]) == DXL2_ERR_NONE)
                    {
                        sim_apply_write(sim, address, length, &p[i + 1U]);
                    }
                    i = (uint16_t) (i + 1U + length);
                }
            }
            return;

        case DXL2_INST_BULK_READ:
            if (broadcast)
            {
                for (uint16_t i = 0U; (uint32_t) i + 5U <= n; i = (uint16_t) (i + 5U))
                {
                    if (p[i] == own_id)
                    {
                        uint16_t address = dxl2_get_u16(&p[i + 1U]);
                        uint16_t length = dxl2_get_u16(&p[i + 3U]);

                        error = sim_check_read(address, length);
                        sim_reply(sim, packet->instruction, error,
                                  error == DXL2_ERR_NONE ? &sim->table[address] : NULL,
                                  error == DXL2_ERR_NONE ? length : 0U);
                    }
                }
            }
            return;

        case DXL2_INST_BULK_WRITE:
            if (broadcast)
            {
                uint16_t i = 0U;

                while ((uint32_t) i + 5U <= n)
                {
                    uint16_t address = dxl2_get_u16(&p[i + 1U]);
                    uint16_t length = dxl2_get_u16(&p[i + 3U]);

                    if (length == 0U || (uint32_t) i + 5U + length > n)
                    {
                        break;
                    }
                    if (p[i] == own_id
                        && sim_check_write(sim, address, length, &p[i + 5U]) == DXL2_ERR_NONE)
                    {
                        sim_apply_write(sim, address, length, &p[i + 5U]);
                    }
                    i = (uint16_t) (i + 5U + length);
                }
            }
            return;

        default:
            break;
    }

    if (!addressed)
    {
        return;
    }

    switch (packet->instruction)
    {
        case DXL2_INST_PING:
            dxl2_put_u16(&data[0], dxl2_get_u16(&sim->table[SIM_ADDR_MODEL_NUMBER]));
            data[2] = sim->table[SIM_ADDR_FIRMWARE_VERSION];
            sim_reply(sim, packet->instruction, DXL2_ERR_NONE, data, 3U);
            break;

        case DXL2_INST_READ:
            if (broadcast)
            {
                break; /* READ to the broadcast ID is not answered */
            }
            if (n != 4U)
            {
                sim_reply(sim, packet->instruction, DXL2_ERR_DATA_LENGTH, NULL, 0U);
                break;
            }
            {
                uint16_t address = dxl2_get_u16(&p[0]);
                uint16_t length = dxl2_get_u16(&p[2]);

                error = sim_check_read(address, length);
                sim_reply(sim, packet->instruction, error,
                          error == DXL2_ERR_NONE ? &sim->table[address] : NULL,
                          error == DXL2_ERR_NONE ? length : 0U);
            }
            break;

        case DXL2_INST_WRITE:
        case DXL2_INST_REG_WRITE:
            if (n < 3U)
            {
                error = DXL2_ERR_DATA_LENGTH;
            }
            else
            {
                uint16_t address = dxl2_get_u16(&p[0]);
                uint16_t length = (uint16_t) (n - 2U);

                error = sim_check_write(sim, address, length, &p[2]);
                if (error == DXL2_ERR_NONE && packet->instruction == DXL2_INST_WRITE)
                {
                    sim_apply_write(sim, address, length, &p[2]);
                }
                else if (error == DXL2_ERR_NONE)
                {
                    memcpy(sim->pending_data, &p[2], length);
                    sim->pending_address = address;
                    sim->pending_length = length;
                    sim->pending_valid = true;
                    sim->table[SIM_ADDR_REGISTERED] = 1U;
                }
                else
                {
                    /* invalid write, nothing stored */
                }
            }
            if (!broadcast)
            {
                sim_reply(sim, packet->instruction, error, NULL, 0U);
            }
            break;

        case DXL2_INST_ACTION:
            if (sim->pending_valid)
            {
                if (sim_check_write(sim, sim->pending_address, sim->pending_length,
                                    sim->pending_data)
                    == DXL2_ERR_NONE)
                {
                    sim_apply_write(sim, sim->pending_address, sim->pending_length,
                                    sim->pending_data);
                }
                sim->pending_valid = false;
                sim->table[SIM_ADDR_REGISTERED] = 0U;
                error = DXL2_ERR_NONE;
            }
            else
            {
                error = DXL2_ERR_RESULT_FAIL;
            }
            if (!broadcast)
            {
                sim_reply(sim, packet->instruction, error, NULL, 0U);
            }
            break;

        case DXL2_INST_FACTORY_RESET:
        {
            uint8_t mode = n >= 1U ? p[0] : SIM_RESET_ALL;
            uint8_t keep_id = own_id;
            uint8_t keep_baud = sim->table[SIM_ADDR_BAUD_RATE];

            if (mode != SIM_RESET_ALL && mode != SIM_RESET_EXCEPT_ID
                && mode != SIM_RESET_EXCEPT_ID_BAUD)
            {
                sim_reply(sim, packet->instruction, DXL2_ERR_DATA_RANGE, NULL, 0U);
                break;
            }
            if (!broadcast)
            {
                sim_reply(sim, packet->instruction, DXL2_ERR_NONE, NULL, 0U);
            }
            sim_factory_defaults(sim, mode == SIM_RESET_ALL ? 1U : keep_id);
            if (mode == SIM_RESET_EXCEPT_ID_BAUD)
            {
                sim->table[SIM_ADDR_BAUD_RATE] = keep_baud;
            }
            break;
        }

        case DXL2_INST_REBOOT:
            if (!broadcast)
            {
                sim_reply(sim, packet->instruction, DXL2_ERR_NONE, NULL, 0U);
            }
            /* RAM area returns to its power-on state */
            sim->table[SIM_ADDR_TORQUE_ENABLE] = 0U;
            sim->table[SIM_ADDR_LED] = 0U;
            sim->table[SIM_ADDR_HARDWARE_ERROR] = 0U;
            sim->table[SIM_ADDR_REGISTERED] = 0U;
            sim->pending_valid = false;
            memcpy(&sim->table[SIM_ADDR_GOAL_POSITION], &sim->table[SIM_ADDR_PRESENT_POSITION], 4U);
            break;

        case DXL2_INST_CLEAR:
        case DXL2_INST_BACKUP:
            if (!broadcast)
            {
                sim_reply(sim, packet->instruction, DXL2_ERR_NONE, NULL, 0U);
            }
            break;

        default:
            if (!broadcast)
            {
                sim_reply(sim, packet->instruction, DXL2_ERR_INSTRUCTION, NULL, 0U);
            }
            break;
    }
}

/* ==============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

size_t dxl2_sim_size(void)
{
    return sizeof(struct dxl2_sim);
}

int dxl2_sim_init(struct dxl2_sim* sim, uint8_t id)
{
    struct dxl2_link_config config;

    if (sim == NULL || id > DXL2_MAX_ID)
    {
        return -1;
    }

    memset(sim, 0, sizeof(*sim));

    config.write = sim_write_bytes;
    config.write_context = sim;
    config.handler = sim_handle_packet;
    config.handler_context = sim;

    if (dxl2_link_init(&sim->link, &config) != 0)
    {
        return -1;
    }

    sim_factory_defaults(sim, id);

    return 0;
}

size_t dxl2_sim_process(struct dxl2_sim* sim, const uint8_t* rx, size_t rx_length, uint8_t* tx,
                        size_t tx_capacity)
{
    size_t count;

    if (sim == NULL)
    {
        return 0U;
    }

    sim->tx_length = 0U;
    if (rx != NULL && rx_length > 0U)
    {
        dxl2_link_receive(&sim->link, rx, rx_length);
    }

    count = sim->tx_length < tx_capacity ? sim->tx_length : tx_capacity;
    if (tx != NULL && count > 0U)
    {
        memcpy(tx, sim->tx, count);
    }

    return tx != NULL ? count : 0U;
}

void dxl2_sim_tick(struct dxl2_sim* sim)
{
    int32_t present;
    int32_t goal;
    int32_t step;
    uint16_t tick;

    if (sim == NULL)
    {
        return;
    }

    tick = dxl2_get_u16(&sim->table[SIM_ADDR_REALTIME_TICK]);
    dxl2_put_u16(&sim->table[SIM_ADDR_REALTIME_TICK], (uint16_t) ((tick + 1U) % 32768U));

    present = (int32_t) dxl2_get_u32(&sim->table[SIM_ADDR_PRESENT_POSITION]);
    goal = (int32_t) dxl2_get_u32(&sim->table[SIM_ADDR_GOAL_POSITION]);
    step = (int32_t) dxl2_get_u32(&sim->table[SIM_ADDR_PROFILE_VELOCITY]);
    if (step <= 0)
    {
        step = SIM_DEFAULT_STEP;
    }

    if (sim->table[SIM_ADDR_TORQUE_ENABLE] == 0U || sim->table[SIM_ADDR_HARDWARE_ERROR] != 0U
        || present == goal)
    {
        sim->table[SIM_ADDR_MOVING] = 0U;
        dxl2_put_u32(&sim->table[SIM_ADDR_PRESENT_VELOCITY], 0U);
        return;
    }

    if (goal > present)
    {
        present = (goal - present > step) ? present + step : goal;
        dxl2_put_u32(&sim->table[SIM_ADDR_PRESENT_VELOCITY], (uint32_t) step);
    }
    else
    {
        present = (present - goal > step) ? present - step : goal;
        dxl2_put_u32(&sim->table[SIM_ADDR_PRESENT_VELOCITY], (uint32_t) -step);
    }

    sim->table[SIM_ADDR_MOVING] = present != goal ? 1U : 0U;
    dxl2_put_u32(&sim->table[SIM_ADDR_PRESENT_POSITION], (uint32_t) present);
}

uint8_t dxl2_sim_id(const struct dxl2_sim* sim)
{
    return sim != NULL ? sim->table[SIM_ADDR_ID] : 0U;
}

int dxl2_sim_read_table(const struct dxl2_sim* sim, uint16_t address, uint8_t* out, uint16_t length)
{
    if (sim == NULL || out == NULL || (uint32_t) address + length > DXL2_SIM_TABLE_SIZE)
    {
        return -1;
    }

    memcpy(out, &sim->table[address], length);

    return 0;
}

void dxl2_sim_set_hardware_error(struct dxl2_sim* sim, uint8_t value)
{
    if (sim != NULL)
    {
        sim->table[SIM_ADDR_HARDWARE_ERROR] = value;
    }
}

uint32_t dxl2_sim_invalid_packets(const struct dxl2_sim* sim)
{
    return sim != NULL ? dxl2_link_invalid_packets(&sim->link) : 0U;
}

size_t dxl2_sim_item_count(void)
{
    return SIM_ITEM_COUNT;
}

int dxl2_sim_item_info(size_t index, uint16_t* address, uint8_t* size, uint8_t* writable,
                       const char** name)
{
    if (index >= SIM_ITEM_COUNT || address == NULL || size == NULL || writable == NULL
        || name == NULL)
    {
        return -1;
    }

    *address = sim_items[index].address;
    *size = sim_items[index].size;
    *writable = sim_items[index].writable;
    *name = sim_items[index].name;

    return 0;
}
