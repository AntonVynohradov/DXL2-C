

# File dxl2\_link.h

[**File List**](files.md) **>** [**inc**](dir_bfccd401955b95cf8c75461437045ac0.md) **>** [**dxl2\_link.h**](dxl2__link_8h.md)

[Go to the documentation of this file](dxl2__link_8h.md)


```C++

#ifndef DXL2_LINK_H
#define DXL2_LINK_H

#include <stddef.h>
#include <stdint.h>

#include "dxl2_protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*dxl2_write_fn)(void* context, const uint8_t* data, size_t length);

typedef void (*dxl2_packet_handler_t)(const struct dxl2_packet* packet, void* context);

struct dxl2_link_config
{
    dxl2_write_fn write;           
    void* write_context;           
    dxl2_packet_handler_t handler; 
    void* handler_context;         
};

struct dxl2_link
{
    struct dxl2_link_config config; 
    struct dxl2_parser parser;      
    struct dxl2_packet packet;      
    uint32_t rx_packets;            
    uint32_t invalid_packets;       
};

int dxl2_link_init(struct dxl2_link* link, const struct dxl2_link_config* config);

void dxl2_link_receive(struct dxl2_link* link, const uint8_t* data, size_t length);

int dxl2_link_send(struct dxl2_link* link, uint8_t id, uint8_t instruction,
                   const uint8_t* parameters, uint16_t parameter_length);

int dxl2_link_send_status(struct dxl2_link* link, uint8_t id, uint8_t error, const uint8_t* data,
                          uint16_t data_length);

uint32_t dxl2_link_rx_packets(const struct dxl2_link* link);

uint32_t dxl2_link_invalid_packets(const struct dxl2_link* link);

#ifdef __cplusplus
}
#endif

#endif /* DXL2_LINK_H */

```


