

# File dxl2\_protocol.h

[**File List**](files.md) **>** [**inc**](dir_bfccd401955b95cf8c75461437045ac0.md) **>** [**dxl2\_protocol.h**](dxl2__protocol_8h.md)

[Go to the documentation of this file](dxl2__protocol_8h.md)


```C++

#ifndef DXL2_PROTOCOL_H
#define DXL2_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#include "dxl2_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DXL2_MAX_PARAMETER_SIZE (DXL2_MAX_PACKET_SIZE - 10U)

#define DXL2_PARSE_ERROR (-1)

struct dxl2_packet
{
    uint8_t id;                                  
    uint8_t instruction;                         
    uint16_t parameter_length;                   
    uint8_t parameters[DXL2_MAX_PARAMETER_SIZE]; 
};

struct dxl2_parser
{
    uint8_t buffer[DXL2_MAX_PACKET_SIZE]; 
    size_t length;                        
};

uint16_t dxl2_crc16(const uint8_t* data, size_t length);

void dxl2_parser_init(struct dxl2_parser* parser);

int dxl2_parser_feed(struct dxl2_parser* parser, uint8_t byte, struct dxl2_packet* packet);

int dxl2_encode(uint8_t id, uint8_t instruction, const uint8_t* parameters,
                uint16_t parameter_length, uint8_t* output, size_t output_capacity,
                size_t* output_length);

#ifdef __cplusplus
}
#endif

#endif /* DXL2_PROTOCOL_H */

```


