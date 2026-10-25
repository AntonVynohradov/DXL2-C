

# File dxl2\_link.c

[**File List**](files.md) **>** [**src**](dir_68267d1309a1af8e8297ef4c3efbcdba.md) **>** [**dxl2\_link.c**](dxl2__link_8c.md)

[Go to the documentation of this file](dxl2__link_8c.md)


```C++
/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Transport-independent link: receive stream dispatch and
 *              packet transmission through a user-supplied writer
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

/* ==============================================================================
 * INCLUDES
 * =============================================================================*/

#include <string.h>

#include "dxl2_defs.h"
#include "dxl2_link.h"

/* ==============================================================================
 * DEFINES & MACROS
 * =============================================================================*/

/* ==============================================================================
 * PRIVATE TYPE DEFINITIONS
 * =============================================================================*/

/* ==============================================================================
 * PRIVATE VARIABLES (static)
 * =============================================================================*/

/* ==============================================================================
 * PRIVATE FUNCTION DECLARATIONS (static)
 * =============================================================================*/

/* ==============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

/* ==============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

int dxl2_link_init(struct dxl2_link* link, const struct dxl2_link_config* config)
{
    int result = DXL2_PARSE_ERROR;

    if ((link != NULL) && (config != NULL))
    {
        (void) memset(link, 0, sizeof(*link));
        link->config = *config;
        dxl2_parser_init(&link->parser);
        result = 0;
    }

    return result;
}

void dxl2_link_receive(struct dxl2_link* link, const uint8_t* data, size_t length)
{
    if ((link != NULL) && (data != NULL))
    {
        for (size_t i = 0U; i < length; ++i)
        {
            const int result = dxl2_parser_feed(&link->parser, data[i], &link->packet);

            if (result > 0)
            {
                link->rx_packets++;
                if (link->config.handler != NULL)
                {
                    link->config.handler(&link->packet, link->config.handler_context);
                }
            }
            else if (result < 0)
            {
                link->invalid_packets++;
            }
            else
            {
                /* Packet incomplete: wait for more bytes. */
            }
        }
    }
}

int dxl2_link_send(struct dxl2_link* link, uint8_t id, uint8_t instruction,
                   const uint8_t* parameters, uint16_t parameter_length)
{
    int result = DXL2_PARSE_ERROR;

    if ((link != NULL) && (link->config.write != NULL))
    {
        uint8_t output[DXL2_MAX_PACKET_SIZE];
        size_t output_length = 0U;

        result = dxl2_encode(id, instruction, parameters, parameter_length, output, sizeof(output),
                             &output_length);
        if (result == 0)
        {
            result = link->config.write(link->config.write_context, output, output_length);
        }
    }

    return result;
}

int dxl2_link_send_status(struct dxl2_link* link, uint8_t id, uint8_t error, const uint8_t* data,
                          uint16_t data_length)
{
    uint8_t parameters[DXL2_MAX_PARAMETER_SIZE];
    int result = DXL2_PARSE_ERROR;

    if ((data_length <= (DXL2_MAX_PARAMETER_SIZE - 1U)) && ((data_length == 0U) || (data != NULL)))
    {
        parameters[0] = error;
        if (data_length > 0U)
        {
            (void) memcpy(&parameters[1], data, data_length);
        }

        result =
            dxl2_link_send(link, id, DXL2_INST_STATUS, parameters, (uint16_t) (data_length + 1U));
    }

    return result;
}

uint32_t dxl2_link_rx_packets(const struct dxl2_link* link)
{
    return (link != NULL) ? link->rx_packets : 0U;
}

uint32_t dxl2_link_invalid_packets(const struct dxl2_link* link)
{
    return (link != NULL) ? link->invalid_packets : 0U;
}
```


