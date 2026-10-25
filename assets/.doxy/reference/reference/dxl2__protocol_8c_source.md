

# File dxl2\_protocol.c

[**File List**](files.md) **>** [**src**](dir_68267d1309a1af8e8297ef4c3efbcdba.md) **>** [**dxl2\_protocol.c**](dxl2__protocol_8c.md)

[Go to the documentation of this file](dxl2__protocol_8c.md)


```C++
/*******************************************************************************
 * DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
 *
 * Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Description: Protocol 2.0 codec: CRC-16, incremental parser and encoder
 *              with byte stuffing
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

#include <stdbool.h>
#include <string.h>

#include "dxl2_defs.h"
#include "dxl2_protocol.h"

/* ==============================================================================
 * DEFINES & MACROS
 * =============================================================================*/

#define DXL2_HEADER_SIZE 4U
#define DXL2_MIN_LENGTH 3U
#define DXL2_LENGTH_OFFSET 5U
#define DXL2_BODY_OFFSET 7U
#define DXL2_CRC_SIZE 2U

/* ==============================================================================
 * PRIVATE TYPE DEFINITIONS
 * =============================================================================*/

/* ==============================================================================
 * PRIVATE VARIABLES (static)
 * =============================================================================*/

static const uint8_t dxl2_header[DXL2_HEADER_SIZE] = {0xFF, 0xFF, 0xFD, 0x00};

/* ==============================================================================
 * PRIVATE FUNCTION DECLARATIONS (static)
 * =============================================================================*/

static void dxl2_parser_reset(struct dxl2_parser* parser);

static int dxl2_decode_packet(const struct dxl2_parser* parser, struct dxl2_packet* packet,
                              size_t total_length);

/* ==============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

static void dxl2_parser_reset(struct dxl2_parser* parser)
{
    parser->length = 0U;
}

/*
 * CRC is checked over the stuffed wire bytes before byte stuffing is removed.
 * The parser accepts at most DXL2_MAX_PACKET_SIZE wire bytes, 10 of which are
 * header, ID, Length, instruction and CRC, and unstuffing only removes bytes,
 * so the decoded parameters always fit in DXL2_MAX_PARAMETER_SIZE.
 */
static int dxl2_decode_packet(const struct dxl2_parser* parser, struct dxl2_packet* packet,
                              size_t total_length)
{
    const size_t crc_offset = total_length - DXL2_CRC_SIZE;
    const uint16_t expected_crc = dxl2_crc16(parser->buffer, crc_offset);
    const uint16_t received_crc = dxl2_get_u16(&parser->buffer[crc_offset]);
    bool valid = (expected_crc == received_crc);
    size_t offset = DXL2_BODY_OFFSET;
    size_t body_length = 0U; /* Unstuffed instruction and parameter bytes. */

    if (valid)
    {
        packet->id = parser->buffer[4];
    }

    while (valid && (offset < crc_offset))
    {
        uint8_t decoded[3];
        size_t decoded_count = 1U;

        decoded[0] = parser->buffer[offset];
        if (((offset + 2U) < crc_offset) && (decoded[0] == 0xFFU)
            && (parser->buffer[offset + 1U] == 0xFFU) && (parser->buffer[offset + 2U] == 0xFDU))
        {
            if (((offset + 3U) < crc_offset) && (parser->buffer[offset + 3U] == 0xFDU))
            {
                decoded[1] = 0xFFU;
                decoded[2] = 0xFDU;
                decoded_count = 3U;
            }
            else
            {
                valid = false;
                decoded_count = 0U;
            }
            offset += 4U;
        }
        else
        {
            offset++;
        }

        for (size_t i = 0U; i < decoded_count; ++i)
        {
            if (body_length == 0U)
            {
                packet->instruction = decoded[i];
            }
            else
            {
                packet->parameters[body_length - 1U] = decoded[i];
            }
            body_length++;
        }
    }

    if (valid)
    {
        packet->parameter_length = (uint16_t) (body_length - 1U);
    }

    return valid ? 1 : DXL2_PARSE_ERROR;
}

/* ==============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * =============================================================================*/

uint16_t dxl2_crc16(const uint8_t* data, size_t length)
{
    uint16_t crc = 0U;

    if (data != NULL)
    {
        for (size_t i = 0U; i < length; ++i)
        {
            crc ^= (uint16_t) ((uint16_t) data[i] << 8);
            for (uint8_t bit = 0U; bit < 8U; ++bit)
            {
                if ((crc & 0x8000U) != 0U)
                {
                    crc = (uint16_t) ((uint16_t) (crc << 1) ^ 0x8005U);
                }
                else
                {
                    crc = (uint16_t) (crc << 1);
                }
            }
        }
    }

    return crc;
}

void dxl2_parser_init(struct dxl2_parser* parser)
{
    if (parser != NULL)
    {
        dxl2_parser_reset(parser);
    }
}

int dxl2_parser_feed(struct dxl2_parser* parser, uint8_t byte, struct dxl2_packet* packet)
{
    int result = 0;

    if ((parser == NULL) || (packet == NULL) || (parser->length >= DXL2_MAX_PACKET_SIZE))
    {
        if (parser != NULL)
        {
            dxl2_parser_reset(parser);
        }
        result = DXL2_PARSE_ERROR;
    }
    else
    {
        parser->buffer[parser->length] = byte;
        parser->length++;
        while ((parser->length >= DXL2_HEADER_SIZE)
               && (memcmp(parser->buffer, dxl2_header, DXL2_HEADER_SIZE) != 0))
        {
            parser->length--;
            (void) memmove(&parser->buffer[0], &parser->buffer[1], parser->length);
        }

        if (parser->length >= (DXL2_LENGTH_OFFSET + 2U))
        {
            const uint16_t length_field = dxl2_get_u16(&parser->buffer[DXL2_LENGTH_OFFSET]);

            if ((length_field < DXL2_MIN_LENGTH)
                || (length_field > (DXL2_MAX_PACKET_SIZE - DXL2_BODY_OFFSET)))
            {
                dxl2_parser_reset(parser);
                result = DXL2_PARSE_ERROR;
            }
            else if (parser->length >= ((size_t) length_field + DXL2_BODY_OFFSET))
            {
                result =
                    dxl2_decode_packet(parser, packet, (size_t) length_field + DXL2_BODY_OFFSET);
                dxl2_parser_reset(parser);
            }
            else
            {
                /* Packet incomplete: wait for more bytes. */
            }
        }
    }

    return result;
}

int dxl2_encode(uint8_t id, uint8_t instruction, const uint8_t* parameters,
                uint16_t parameter_length, uint8_t* output, size_t output_capacity,
                size_t* output_length)
{
    const size_t body_start = DXL2_BODY_OFFSET;
    const size_t body_length = (size_t) parameter_length + 1U;
    int result = DXL2_PARSE_ERROR;

    if ((output != NULL) && (output_length != NULL)
        && ((parameter_length == 0U) || (parameters != NULL))
        && (parameter_length <= DXL2_MAX_PARAMETER_SIZE) && (output_capacity >= 10U))
    {
        size_t write_offset = DXL2_BODY_OFFSET;
        bool fits = true;

        (void) memcpy(output, dxl2_header, sizeof(dxl2_header));
        output[4] = id;

        for (size_t i = 0U; fits && (i < body_length); ++i)
        {
            const uint8_t value = (i == 0U) ? instruction : parameters[i - 1U];

            if ((write_offset + 1U) > (output_capacity - DXL2_CRC_SIZE))
            {
                fits = false;
            }
            else
            {
                output[write_offset] = value;
                write_offset++;
                if (((write_offset - body_start) >= 3U) && (output[write_offset - 3U] == 0xFFU)
                    && (output[write_offset - 2U] == 0xFFU) && (output[write_offset - 1U] == 0xFDU))
                {
                    if ((write_offset + 1U) > (output_capacity - DXL2_CRC_SIZE))
                    {
                        fits = false;
                    }
                    else
                    {
                        output[write_offset] = 0xFDU;
                        write_offset++;
                    }
                }
            }
        }

        if (fits)
        {
            const uint16_t length_field = (uint16_t) ((write_offset - body_start) + DXL2_CRC_SIZE);

            dxl2_put_u16(&output[DXL2_LENGTH_OFFSET], length_field);
            dxl2_put_u16(&output[write_offset], dxl2_crc16(output, write_offset));
            *output_length = write_offset + DXL2_CRC_SIZE;
            result = 0;
        }
    }

    return result;
}
```


