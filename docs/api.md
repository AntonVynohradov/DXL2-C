# API Reference

This chapter documents every public function, type and macro of DXL2-C 1.0. Include the
umbrella header to use them:

```c
#include "dxl2.h"
```

| Header | Contents |
| --- | --- |
| `dxl2.h` | Includes all headers below; library version |
| `dxl2_config.h` | Build-time configuration |
| `dxl2_defs.h` | IDs, instructions, error codes, little-endian helpers |
| `dxl2_protocol.h` | CRC-16, stream parser, encoder |
| `dxl2_link.h` | Transport-independent link |

The reference generated from the source comments by Doxygen is available on the website
under *Reference → Doxygen*.

## Version — `dxl2.h`

| Macro | Value in 1.0.0 | Meaning |
| --- | --- | --- |
| `DXL2_VERSION_MAJOR` | `1` | Incompatible API changes |
| `DXL2_VERSION_MINOR` | `0` | Compatible additions |
| `DXL2_VERSION_PATCH` | `0` | Fixes |
| `DXL2_VERSION` | `"1.0.0"` | Version string |

CMake builds take the values from the `vX.Y.Z` Git tag; other builds use the fallbacks in
`dxl2.h`.

## Configuration — `dxl2_config.h`

| Macro | Default | Meaning |
| --- | --- | --- |
| `DXL2_MAX_PACKET_SIZE` | `256U` | Largest complete wire packet, header to CRC, that the parser accepts and the link sends. Range `10U..65542U`, checked at compile time. Override with a compiler definition. |

## Definitions — `dxl2_defs.h`

### Device IDs

| Macro | Value | Meaning |
| --- | --- | --- |
| `DXL2_MAX_ID` | `252U` | Highest ID a device may use |
| `DXL2_BROADCAST_ID` | `0xFEU` | Broadcast ID: every device executes the instruction |

### Instructions

| Macro | Value | Macro | Value |
| --- | --- | --- | --- |
| `DXL2_INST_PING` | `0x01U` | `DXL2_INST_STATUS` | `0x55U` |
| `DXL2_INST_READ` | `0x02U` | `DXL2_INST_SYNC_READ` | `0x82U` |
| `DXL2_INST_WRITE` | `0x03U` | `DXL2_INST_SYNC_WRITE` | `0x83U` |
| `DXL2_INST_REG_WRITE` | `0x04U` | `DXL2_INST_FAST_SYNC_READ` | `0x8AU` |
| `DXL2_INST_ACTION` | `0x05U` | `DXL2_INST_BULK_READ` | `0x92U` |
| `DXL2_INST_FACTORY_RESET` | `0x06U` | `DXL2_INST_BULK_WRITE` | `0x93U` |
| `DXL2_INST_REBOOT` | `0x08U` | `DXL2_INST_FAST_BULK_READ` | `0x9AU` |
| `DXL2_INST_CLEAR` | `0x10U` | | |
| `DXL2_INST_BACKUP` | `0x20U` | | |

### Status error byte

| Macro | Value | Meaning |
| --- | --- | --- |
| `DXL2_ERR_NONE` | `0x00U` | No error |
| `DXL2_ERR_RESULT_FAIL` | `0x01U` | The instruction could not be processed |
| `DXL2_ERR_INSTRUCTION` | `0x02U` | Undefined instruction, or ACTION without REG_WRITE |
| `DXL2_ERR_CRC` | `0x03U` | CRC mismatch |
| `DXL2_ERR_DATA_RANGE` | `0x04U` | Data outside the register range |
| `DXL2_ERR_DATA_LENGTH` | `0x05U` | Data shorter than the register |
| `DXL2_ERR_DATA_LIMIT` | `0x06U` | Data outside the configured limit |
| `DXL2_ERR_ACCESS` | `0x07U` | Access denied (read-only, write-only, locked) |
| `DXL2_ERR_ALERT_BIT` | `0x80U` | Flag ORed into the error byte: the device has a hardware error |

### Little-endian helpers

```c
static inline uint16_t dxl2_get_u16(const uint8_t* data);
static inline uint32_t dxl2_get_u32(const uint8_t* data);
static inline void dxl2_put_u16(uint8_t* data, uint16_t value);
static inline void dxl2_put_u32(uint8_t* data, uint32_t value);
```

Read or write a 16- or 32-bit value in little-endian byte order, independent of the
CPU's byte order and alignment. `data` must point to 2 (or 4) accessible bytes; the
helpers do not check it, like `memcpy()`.

## Codec — `dxl2_protocol.h`

### Limits and errors

| Macro | Value | Meaning |
| --- | --- | --- |
| `DXL2_MAX_PARAMETER_SIZE` | `DXL2_MAX_PACKET_SIZE - 10U` | Largest unstuffed parameter block (246 by default) |
| `DXL2_PARSE_ERROR` | `-1` | Invalid argument, malformed packet, CRC mismatch or insufficient capacity |

### `struct dxl2_packet`

One decoded packet, with byte stuffing removed.

```c
struct dxl2_packet
{
    uint8_t id;                                  /* source or destination ID */
    uint8_t instruction;                         /* instruction, 0x55 for Status */
    uint16_t parameter_length;                   /* number of valid bytes in parameters */
    uint8_t parameters[DXL2_MAX_PARAMETER_SIZE]; /* unstuffed parameters */
};
```

For a Status Packet, `parameters[0]` is the error byte and the data follows from
`parameters[1]`.

### `struct dxl2_parser`

State of the incremental parser for one byte stream. Treat the members as private.
Initialize it with `dxl2_parser_init()` before use.

```c
struct dxl2_parser
{
    uint8_t buffer[DXL2_MAX_PACKET_SIZE]; /* packet in progress, header to CRC */
    size_t length;                        /* number of bytes in buffer */
};
```

### `dxl2_crc16()`

```c
uint16_t dxl2_crc16(const uint8_t* data, size_t length);
```

Computes the Protocol 2.0 CRC-16: polynomial `0x8005`, initial value `0`, not reflected,
no final XOR.

| Parameter | Description |
| --- | --- |
| `data` | Bytes covered by the CRC |
| `length` | Number of bytes |

**Returns** the CRC, or `0` (the initial value) when `data` is `NULL`.

### `dxl2_parser_init()`

```c
void dxl2_parser_init(struct dxl2_parser* parser);
```

Resets a parser before a new byte stream, or to discard a partial packet. `NULL` is
ignored.

### `dxl2_parser_feed()`

```c
int dxl2_parser_feed(struct dxl2_parser* parser, uint8_t byte, struct dxl2_packet* packet);
```

Feeds one byte to the parser. The parser searches for the header, checks the Length
field, waits for the complete packet, validates the CRC and removes byte stuffing. Keep
feeding bytes after an error: the parser resynchronizes on the next header.

| Parameter | Description |
| --- | --- |
| `parser` | Initialized parser state |
| `byte` | Next byte of the stream |
| `packet` | Output packet; its contents are valid only when the call returns `1` |

| Return | Meaning |
| --- | --- |
| `1` | A complete, valid packet is in `packet` |
| `0` | More bytes are needed |
| `DXL2_PARSE_ERROR` | `NULL` argument, invalid Length, CRC mismatch or invalid byte stuffing; the parser has been reset |

### `dxl2_encode()`

```c
int dxl2_encode(uint8_t id, uint8_t instruction, const uint8_t* parameters,
                uint16_t parameter_length, uint8_t* output, size_t output_capacity,
                size_t* output_length);
```

Encodes a complete wire packet: header, ID, Length, instruction, parameters with byte
stuffing, and CRC.

| Parameter | Description |
| --- | --- |
| `id` | Packet ID |
| `instruction` | Instruction byte |
| `parameters` | Unstuffed parameters; may be `NULL` when `parameter_length` is 0 |
| `parameter_length` | Number of parameter bytes, at most `DXL2_MAX_PARAMETER_SIZE` |
| `output` | Buffer that receives the packet |
| `output_capacity` | Size of `output`; at least 10 |
| `output_length` | Receives the packet length on success |

| Return | Meaning |
| --- | --- |
| `0` | Packet encoded; `*output_length` bytes of `output` are valid |
| `DXL2_PARSE_ERROR` | `NULL` argument, too many parameters, or the stuffed packet does not fit in `output_capacity` |

A buffer of `DXL2_MAX_PACKET_SIZE` bytes holds every packet the link can send. The
encoder never writes beyond `output_capacity`.

## Link — `dxl2_link.h`

### `dxl2_write_fn`

```c
typedef int (*dxl2_write_fn)(void* context, const uint8_t* data, size_t length);
```

Writes one complete wire packet to the transport. Called by `dxl2_link_send()` and
`dxl2_link_send_status()`.

| Parameter | Description |
| --- | --- |
| `context` | `write_context` from the link configuration |
| `data` | The encoded packet; valid only during the call |
| `length` | Number of bytes |

**Returns** `0` on success or a negative value on failure; the value is passed back to
the caller of the send function.

### `dxl2_packet_handler_t`

```c
typedef void (*dxl2_packet_handler_t)(const struct dxl2_packet* packet, void* context);
```

Called by `dxl2_link_receive()` for every valid packet. `packet` is valid only until the
handler returns; copy what you need. The handler may send on the same link.

| Parameter | Description |
| --- | --- |
| `packet` | The decoded packet |
| `context` | `handler_context` from the link configuration |

### `struct dxl2_link_config`

```c
struct dxl2_link_config
{
    dxl2_write_fn write;           /* NULL makes the link receive-only */
    void* write_context;           /* passed to write */
    dxl2_packet_handler_t handler; /* NULL discards packets (they are still counted) */
    void* handler_context;         /* passed to handler */
};
```

### `struct dxl2_link`

The state of one link: a copy of the configuration, a parser, the last packet and two
counters. Allocate it yourself and treat its members as private.

### `dxl2_link_init()`

```c
int dxl2_link_init(struct dxl2_link* link, const struct dxl2_link_config* config);
```

Initializes a link: copies the configuration, resets the parser and the counters.

| Return | Meaning |
| --- | --- |
| `0` | Link ready |
| `DXL2_PARSE_ERROR` | `link` or `config` is `NULL` |

### `dxl2_link_receive()`

```c
void dxl2_link_receive(struct dxl2_link* link, const uint8_t* data, size_t length);
```

Feeds received bytes, in any chunking, to the link's parser. For each valid packet it
increments the receive counter and calls the handler; each rejected packet increments
the invalid-packet counter. Does nothing if `link` or `data` is `NULL`.

Call it for a given link from one context only, and not from an interrupt handler.

### `dxl2_link_send()`

```c
int dxl2_link_send(struct dxl2_link* link, uint8_t id, uint8_t instruction,
                   const uint8_t* parameters, uint16_t parameter_length);
```

Encodes a packet into a stack buffer of `DXL2_MAX_PACKET_SIZE` bytes and passes it to the
write function.

| Return | Meaning |
| --- | --- |
| `0` | Packet written |
| `DXL2_PARSE_ERROR` | `NULL` link, no write function, invalid parameters, or the packet does not fit |
| other negative value | Returned by the write function |

### `dxl2_link_send_status()`

```c
int dxl2_link_send_status(struct dxl2_link* link, uint8_t id, uint8_t error,
                          const uint8_t* data, uint16_t data_length);
```

Sends a Status Packet with instruction `0x55`: the error byte followed by `data`.

| Parameter | Description |
| --- | --- |
| `id` | ID of the answering device |
| `error` | `DXL2_ERR_*`, optionally ORed with `DXL2_ERR_ALERT_BIT` |
| `data` | Status data; may be `NULL` when `data_length` is 0 |
| `data_length` | Number of data bytes, at most `DXL2_MAX_PARAMETER_SIZE - 1` |

**Returns** the same values as `dxl2_link_send()`. Invalid `data` or `data_length` returns
`DXL2_PARSE_ERROR` before anything is written.

### `dxl2_link_rx_packets()`, `dxl2_link_invalid_packets()`

```c
uint32_t dxl2_link_rx_packets(const struct dxl2_link* link);
uint32_t dxl2_link_invalid_packets(const struct dxl2_link* link);
```

Return the number of valid and of rejected packets since `dxl2_link_init()`, or `0` for a
`NULL` link. The counters wrap around at 2^32.

## Return codes summary

| Value | Meaning |
| --- | --- |
| `0` | Success (`dxl2_parser_feed()`: need more bytes) |
| `1` | `dxl2_parser_feed()` only: a complete valid packet is available |
| `DXL2_PARSE_ERROR` (`-1`) | Invalid argument, malformed packet, CRC mismatch, or insufficient capacity |
| other negative | `dxl2_link_send*()` only: value returned by the write function |
