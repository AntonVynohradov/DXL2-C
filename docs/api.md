# API Reference

Include the umbrella header:

```c
#include "dxl2.h"
```

The full generated reference (Doxygen) is in the **API** section of this site.

## Codec (`dxl2_protocol.h`)

| Function | Purpose |
| --- | --- |
| `uint16_t dxl2_crc16(const uint8_t *data, size_t length)` | Protocol 2.0 CRC-16 |
| `void dxl2_parser_init(struct dxl2_parser *parser)` | Reset a stream parser |
| `int dxl2_parser_feed(struct dxl2_parser *parser, uint8_t byte, struct dxl2_packet *packet)` | Feed one byte; `1` = packet ready, `0` = need more, `DXL2_PARSE_ERROR` = rejected |
| `int dxl2_encode(uint8_t id, uint8_t instruction, const uint8_t *parameters, uint16_t parameter_length, uint8_t *output, size_t output_capacity, size_t *output_length)` | Build a wire packet with stuffing and CRC |

## Link (`dxl2_link.h`)

| Function | Purpose |
| --- | --- |
| `int dxl2_link_init(struct dxl2_link *link, const struct dxl2_link_config *config)` | Bind writer and handler, reset state |
| `void dxl2_link_receive(struct dxl2_link *link, const uint8_t *data, size_t length)` | Feed received bytes, dispatch valid packets |
| `int dxl2_link_send(struct dxl2_link *link, uint8_t id, uint8_t instruction, const uint8_t *parameters, uint16_t parameter_length)` | Encode and write a packet |
| `int dxl2_link_send_status(struct dxl2_link *link, uint8_t id, uint8_t error, const uint8_t *data, uint16_t data_length)` | Encode and write a Status Packet |
| `uint32_t dxl2_link_rx_packets(const struct dxl2_link *link)` | Valid packets received |
| `uint32_t dxl2_link_invalid_packets(const struct dxl2_link *link)` | Packets rejected |

Callbacks supplied in `struct dxl2_link_config`:

```c
typedef int (*dxl2_write_fn)(void *context, const uint8_t *data, size_t length);
typedef void (*dxl2_packet_handler_t)(const struct dxl2_packet *packet, void *context);
```

## Definitions (`dxl2_defs.h`)

- IDs: `DXL2_MAX_ID`, `DXL2_BROADCAST_ID`
- Instructions: `DXL2_INST_*`
- Status error byte: `DXL2_ERR_*`, `DXL2_ERR_ALERT_BIT`
- Little-endian helpers: `dxl2_get_u16`, `dxl2_get_u32`, `dxl2_put_u16`, `dxl2_put_u32`

## Return codes

| Value | Meaning |
| --- | --- |
| `0` | Success |
| `1` | `dxl2_parser_feed()` only: a complete valid packet is available |
| `DXL2_PARSE_ERROR` (`-1`) | Invalid argument, malformed packet, CRC mismatch, or insufficient capacity |
| other negative | `dxl2_link_send*()` only: value returned by the write callback |
