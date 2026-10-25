# Using the Library

This chapter shows how to use DXL2-C in the three typical roles: as a **device** that
answers instructions, as a **controller** that drives devices, and as a plain **codec**
without the link layer. All examples use only the public API from `dxl2.h`.

## Setting up a link

Every link needs two things from you: a function that writes bytes to the transport and a
function that handles received packets.

```c
#include "dxl2.h"

static struct dxl2_link bus;

/* Write one complete packet. Return 0 on success, a negative value on failure. */
static int bus_write(void* context, const uint8_t* data, size_t length)
{
    (void) context;
    return uart_write_blocking(data, length);
}

/* Called for every valid packet received on the bus. */
static void bus_handler(const struct dxl2_packet* packet, void* context)
{
    (void) context;
    /* see below */
}

void bus_init(void)
{
    const struct dxl2_link_config config = {
        .write = bus_write,
        .write_context = NULL,
        .handler = bus_handler,
        .handler_context = NULL,
    };

    (void) dxl2_link_init(&bus, &config);
}
```

- `dxl2_link_init()` copies the configuration, so `config` can be a local variable.
- The two context pointers are passed back unchanged to your callbacks. Use them to reach
  your driver or application object without global variables.
- A link without a write function is *receive-only*: sending returns `DXL2_PARSE_ERROR`.
  A link without a handler still parses and counts packets.

Then move received bytes into the link wherever your transport delivers them:

```c
void bus_poll(void)
{
    uint8_t bytes[64];
    const size_t count = uart_read_nonblocking(bytes, sizeof(bytes));

    dxl2_link_receive(&bus, bytes, count);
}
```

Chunk boundaries do not matter: one byte at a time, whole packets or several packets at
once all give the same result.

## Writing a device

A device answers Instruction Packets addressed to its ID. The handler receives each
decoded packet; `packet->parameters` holds the unstuffed parameters and
`packet->parameter_length` their number.

```c
#define MY_ID 1U

static uint8_t control_table[128];

static void bus_handler(const struct dxl2_packet* packet, void* context)
{
    const bool broadcast = (packet->id == DXL2_BROADCAST_ID);

    (void) context;
    if ((packet->id != MY_ID) && !broadcast)
    {
        return; /* addressed to another device */
    }

    switch (packet->instruction)
    {
        case DXL2_INST_PING:
            handle_ping();            /* every device answers a broadcast PING */
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
            break;                    /* a reply from another device on the bus */
        default:
            if (!broadcast)
            {
                (void) dxl2_link_send_status(&bus, MY_ID, DXL2_ERR_INSTRUCTION, NULL, 0U);
            }
            break;
    }
}
```

### Reading parameters

Instruction parameters are little endian. Use the helpers from `dxl2_defs.h` instead of
shifting bytes by hand:

```c
/* READ: address (2 bytes), length (2 bytes) */
static void handle_read(const struct dxl2_packet* packet)
{
    if (packet->parameter_length != 4U)
    {
        (void) dxl2_link_send_status(&bus, MY_ID, DXL2_ERR_DATA_LENGTH, NULL, 0U);
    }
    else
    {
        const uint16_t address = dxl2_get_u16(&packet->parameters[0]);
        const uint16_t length = dxl2_get_u16(&packet->parameters[2]);

        if (((uint32_t) address + length) > sizeof(control_table))
        {
            (void) dxl2_link_send_status(&bus, MY_ID, DXL2_ERR_DATA_RANGE, NULL, 0U);
        }
        else
        {
            (void) dxl2_link_send_status(&bus, MY_ID, DXL2_ERR_NONE,
                                         &control_table[address], length);
        }
    }
}
```

Always check `parameter_length` before reading parameters: the library guarantees that
the bytes up to `parameter_length` are valid, but it does not know the layout of each
instruction.

### Answering

`dxl2_link_send_status()` builds the Status Packet for you: instruction `0x55`, the error
byte, then your data.

| Error value | Meaning |
| --- | --- |
| `DXL2_ERR_NONE` | Success |
| `DXL2_ERR_RESULT_FAIL` | The instruction could not be processed |
| `DXL2_ERR_INSTRUCTION` | Undefined instruction, or ACTION without REG_WRITE |
| `DXL2_ERR_CRC` | CRC mismatch (DXL2-C drops CRC-invalid packets before your handler sees them, so a device built on it normally never sends this) |
| `DXL2_ERR_DATA_RANGE` | Data outside the register range |
| `DXL2_ERR_DATA_LENGTH` | Data shorter than required |
| `DXL2_ERR_DATA_LIMIT` | Data outside the configured limit |
| `DXL2_ERR_ACCESS` | Writing a read-only register, reading a write-only one, or a locked EEPROM area |

OR the error with `DXL2_ERR_ALERT_BIT` when your device has an active hardware error, so
the controller knows to read its Hardware Error Status.

### Rules a well-behaved device follows

These rules come from the protocol specification. The library leaves them to you because
they depend on your Control Table:

- **Broadcast.** Execute broadcast instructions, but do not answer them — except PING,
  which every device answers. On a real bus, devices answer a broadcast PING in ID order,
  each after a delay, to avoid collisions.
- **Status Return Level.** If you emulate the Status Return Level register, answer only
  PING (level 0), only PING and READ (level 1), or everything (level 2).
- **Return Delay Time.** If you emulate it, wait that long before transmitting.
- **Half duplex.** Release the bus (switch the transceiver to receive) as soon as the last
  byte has left the UART. Your write function is the right place for that.

The [bare-metal example](examples.md#bare-metal) implements PING, READ and WRITE with
range and access checks on a small Control Table.

## Writing a controller

A controller sends Instruction Packets and receives Status Packets through the same link.
Because the library has no clock, your application sends the request, then keeps feeding
received bytes until the reply arrives or its own timeout expires.

```c
struct pending_reply
{
    uint8_t id;          /* the device we expect an answer from */
    bool received;
    uint8_t error;
    uint8_t data[8];
    uint16_t data_length;
};

static void controller_handler(const struct dxl2_packet* packet, void* context)
{
    struct pending_reply* reply = context;

    /* Ignore our own Instruction Packets echoed by a single-wire adapter. */
    if ((packet->instruction == DXL2_INST_STATUS) && (packet->id == reply->id)
        && (packet->parameter_length >= 1U)
        && ((packet->parameter_length - 1U) <= sizeof(reply->data)))
    {
        reply->error = packet->parameters[0];
        reply->data_length = (uint16_t) (packet->parameter_length - 1U);
        (void) memcpy(reply->data, &packet->parameters[1], reply->data_length);
        reply->received = true;
    }
}

/* Read the 4-byte Present Position (address 132 on X-series servos). */
static int read_present_position(uint8_t id, int32_t* position)
{
    uint8_t parameters[4];
    int result = -1;

    pending.id = id;
    pending.received = false;
    dxl2_put_u16(&parameters[0], 132U); /* address */
    dxl2_put_u16(&parameters[2], 4U);   /* length */

    if (dxl2_link_send(&bus, id, DXL2_INST_READ, parameters, sizeof(parameters)) == 0)
    {
        const uint32_t start = millis();

        while (!pending.received && ((millis() - start) < 20U))
        {
            bus_poll(); /* feeds dxl2_link_receive() */
        }
        if (pending.received && (pending.error == DXL2_ERR_NONE) && (pending.data_length == 4U))
        {
            *position = (int32_t) dxl2_get_u32(pending.data);
            result = 0;
        }
    }
    return result;
}
```

Here `pending` is a `struct pending_reply` passed as `handler_context` when the link is
initialized, and `millis()` is your system tick.

Points to keep in mind:

- **Echo.** On a single-wire half-duplex bus without echo cancellation, your own
  Instruction Packet comes back to you. It is a valid packet with your instruction byte,
  so filter on `DXL2_INST_STATUS`.
- **Timeouts.** Choose a timeout that covers the device's Return Delay Time plus the
  transmission time of the reply. Retry on timeout: a corrupted reply is dropped silently
  by the parser.
- **Broadcast PING.** Several devices answer one after another. Keep feeding bytes for a
  while and collect every Status Packet; the [host serial example](examples.md#host-serial-port)
  does exactly that.
- **SYNC and BULK instructions.** Build their parameter blocks with `dxl2_put_u16()` and
  `dxl2_put_u32()` and send them with `dxl2_link_send()`; see
  [Extending the Library](extending.md#building-instruction-parameters).

## Using the codec without a link

The codec functions work on plain buffers. This is useful in tools, tests, gateways, or
firmware that already has its own message loop.

**Encoding** a packet into your own buffer:

```c
uint8_t packet[DXL2_MAX_PACKET_SIZE];
size_t packet_length = 0U;

if (dxl2_encode(1U, DXL2_INST_PING, NULL, 0U, packet, sizeof(packet), &packet_length) == 0)
{
    /* packet[0 .. packet_length - 1] = FF FF FD 00 01 03 00 01 19 4E */
}
```

**Parsing** a stream:

```c
static struct dxl2_parser parser;
static struct dxl2_packet packet;

void stream_init(void)
{
    dxl2_parser_init(&parser);
}

void stream_bytes(const uint8_t* data, size_t length)
{
    for (size_t i = 0U; i < length; ++i)
    {
        const int result = dxl2_parser_feed(&parser, data[i], &packet);

        if (result == 1)
        {
            process(&packet);      /* a complete, valid packet */
        }
        else if (result < 0)
        {
            count_bad_packet();    /* keep feeding: the parser resynchronizes */
        }
        else
        {
            /* 0: need more bytes */
        }
    }
}
```

`packet` is valid only after a call that returned `1`; after an error its contents are
undefined. Use one parser per byte stream.

**Computing a CRC** directly, for example to build a test vector:

```c
static const uint8_t ping[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x03, 0x00, 0x01};
const uint16_t crc = dxl2_crc16(ping, sizeof(ping)); /* 0x4E19, sent as 19 4E */
```

## Return values

| Function | Success | Failure |
| --- | --- | --- |
| `dxl2_link_init()` | `0` | `DXL2_PARSE_ERROR` (`NULL` argument) |
| `dxl2_link_send()`, `dxl2_link_send_status()` | `0` | `DXL2_PARSE_ERROR`, or the negative value of your write function |
| `dxl2_encode()` | `0` | `DXL2_PARSE_ERROR` (invalid arguments, packet does not fit) |
| `dxl2_parser_feed()` | `1` packet ready, `0` need more | `DXL2_PARSE_ERROR` |
| `dxl2_crc16()` | the CRC | `0` for a `NULL` buffer |
| `dxl2_link_rx_packets()`, `dxl2_link_invalid_packets()` | the counter | `0` for a `NULL` link |

`DXL2_PARSE_ERROR` is `-1`. Make your write function return other negative values for
its own errors so that you can tell them apart.

## Diagnostics

Each link counts the packets it accepted and rejected since `dxl2_link_init()`:

```c
const uint32_t good = dxl2_link_rx_packets(&bus);
const uint32_t bad = dxl2_link_invalid_packets(&bus);
```

A steadily rising `invalid_packets` count usually means a wrong baud rate, a baud rate
error above about 2 %, a missing termination, bus contention, or text (console output,
logging) on the same UART.

## Common mistakes

| Symptom | Likely cause |
| --- | --- |
| Nothing is ever received | `dxl2_link_init()` not called, or received bytes never passed to `dxl2_link_receive()` |
| Every packet is invalid | Wrong baud rate, inverted signal, or the transceiver direction is not switched |
| A controller receives its own commands | Single-wire bus echo: filter on `DXL2_INST_STATUS` |
| Replies corrupt each other | Two devices with the same ID, or a device answering a broadcast instruction |
| Crash or garbage in the handler after it returns | Keeping the `packet` pointer: copy the data instead |
| Random corruption under load | Two threads sending on one link without a lock, or `dxl2_link_receive()` called from an interrupt |
