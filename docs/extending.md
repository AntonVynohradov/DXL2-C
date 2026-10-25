# Extending the Library

DXL2-C is intentionally small: it implements the wire format and stops there. This
chapter shows where the extension points are, how to build the common higher-level
pieces on top of the public API, and how to change the library itself when that is the
right thing to do.

## Where to extend

| You want to… | Do it in | Library change needed? |
| --- | --- | --- |
| Support another transport (RS-485, USB CDC, TCP, CAN gateway) | your write function and receive loop | No |
| Emulate a device model | your packet handler and Control Table | No |
| Send SYNC, BULK or other multi-device instructions | parameter builders on top of `dxl2_link_send()` | No |
| Use a private, vendor-specific instruction | your handler; the codec passes any instruction byte through | No |
| Handle bigger packets | `DXL2_MAX_PACKET_SIZE` | No, configuration only |
| Run several buses | one `struct dxl2_link` per bus | No |
| Fix a bug or change wire-format behavior | the library sources | Yes — see [Contributing](#contributing-to-the-library) |

Almost everything is done *on top of* the library. Keep it that way: your extensions stay
portable, and you can update DXL2-C without merging changes.

## Adding a transport

A transport needs two pieces of code.

**Receive.** Deliver received bytes to `dxl2_link_receive()` from one context. Any
chunking works. Typical shapes:

| Transport | Receive loop |
| --- | --- |
| UART with RX interrupt | ISR writes to a ring buffer; main loop or thread drains it |
| UART with DMA | Idle-line or half/full-transfer interrupt signals a thread; the thread passes the new region |
| USB CDC / TCP socket | Pass each received block as it arrives |
| Log file or capture | Read the file in blocks and pass them in order |

**Transmit.** Implement `dxl2_write_fn`:

```c
typedef int (*dxl2_write_fn)(void* context, const uint8_t* data, size_t length);
```

The contract:

- `data` is one complete packet, header to CRC. Send all `length` bytes in order, without
  interleaving other output.
- Return `0` on success or a negative value on failure; the value is returned by
  `dxl2_link_send*()`. Avoid `-1`, which is `DXL2_PARSE_ERROR`.
- The buffer is valid only during the call. A buffered or DMA transmitter must copy it.
- On a half-duplex bus, enable the driver before the first byte and switch back to
  receive when the last byte has left the shift register — not when the FIFO is empty.
- If several contexts can send, serialize the calls with a mutex.

Use `write_context` to pass your driver instance, so one write function serves several
buses:

```c
struct uart_port; /* your driver */
int uart_port_write(struct uart_port* port, const uint8_t* data, size_t length);

static int link_write(void* context, const uint8_t* data, size_t length)
{
    return uart_port_write((struct uart_port*) context, data, length);
}

static struct dxl2_link bus_a;
static struct dxl2_link bus_b;

void buses_init(struct uart_port* port_a, struct uart_port* port_b)
{
    const struct dxl2_link_config config_a = {.write = link_write, .write_context = port_a};
    const struct dxl2_link_config config_b = {.write = link_write, .write_context = port_b};

    (void) dxl2_link_init(&bus_a, &config_a);
    (void) dxl2_link_init(&bus_b, &config_b);
}
```

## Building a Control Table

A device is mostly a register map. A table-driven description keeps the READ and WRITE
handlers generic and makes range and access checks automatic:

```c
enum access
{
    ACCESS_R,  /* read-only */
    ACCESS_RW, /* read-write, RAM */
    ACCESS_RW_EEPROM, /* read-write only while torque is off */
};

struct item
{
    uint16_t address;
    uint8_t size;
    enum access access;
};

static const struct item items[] = {
    {0U, 2U, ACCESS_R},           /* Model Number */
    {6U, 1U, ACCESS_R},           /* Firmware Version */
    {7U, 1U, ACCESS_RW_EEPROM},   /* ID */
    {64U, 1U, ACCESS_RW},         /* Torque Enable */
    {65U, 1U, ACCESS_RW},         /* LED */
    {116U, 4U, ACCESS_RW},        /* Goal Position */
    {132U, 4U, ACCESS_R},         /* Present Position */
};

static uint8_t table[CONTROL_TABLE_SIZE];

/* DXL2_ERR_NONE if [address, address + length) may be written now. */
static uint8_t check_write(uint16_t address, uint16_t length, bool torque_on)
{
    uint8_t error = DXL2_ERR_NONE;

    for (size_t i = 0U; i < (sizeof(items) / sizeof(items[0])); ++i)
    {
        const struct item* item = &items[i];
        const bool overlaps = (item->address < (address + length))
                              && (address < (item->address + item->size));

        if (overlaps && ((item->access == ACCESS_R)
                         || ((item->access == ACCESS_RW_EEPROM) && torque_on)))
        {
            error = DXL2_ERR_ACCESS;
        }
    }
    return error;
}
```

Apply side effects after the write — for example, a new ID takes effect after the Status
Packet has been sent with the old one. The simulator of [DXL2 Studio](gui.md)
(`tools/dxl2_gui/native/dxl2_sim.c`) is a complete reference implementation of an X-series
Control Table, including REG_WRITE/ACTION, FACTORY_RESET, SYNC and BULK instructions.

## Building instruction parameters

The codec encodes any instruction; it does not need to know its parameter layout. Build
the parameters with the little-endian helpers and send them with `dxl2_link_send()`.

| Instruction | Parameters |
| --- | --- |
| READ | address (2), length (2) |
| WRITE, REG_WRITE | address (2), data (n) |
| SYNC_READ, FAST_SYNC_READ | address (2), length (2), ID (1) × devices |
| SYNC_WRITE | address (2), length (2), then per device: ID (1), data (length) |
| BULK_READ, FAST_BULK_READ | per device: ID (1), address (2), length (2) |
| BULK_WRITE | per device: ID (1), address (2), length (2), data (length) |
| FACTORY_RESET, CLEAR, BACKUP | one or more option bytes, see the ROBOTIS specification |

SYNC and BULK instructions are sent to the broadcast ID. A SYNC_WRITE builder that sets
the 4-byte Goal Position (address 116 on X-series servos) of several servos at once:

```c
/* Returns 0 on success, DXL2_PARSE_ERROR if the packet does not fit. */
int sync_write_u32(struct dxl2_link* link, uint16_t address, const uint8_t* ids,
                   const uint32_t* values, size_t count)
{
    uint8_t parameters[DXL2_MAX_PARAMETER_SIZE];
    size_t length = 4U;
    int result = DXL2_PARSE_ERROR;

    if ((4U + (count * 5U)) <= sizeof(parameters))
    {
        dxl2_put_u16(&parameters[0], address);
        dxl2_put_u16(&parameters[2], 4U); /* bytes per device */
        for (size_t i = 0U; i < count; ++i)
        {
            parameters[length] = ids[i];
            dxl2_put_u32(&parameters[length + 1U], values[i]);
            length += 5U;
        }
        result = dxl2_link_send(link, DXL2_BROADCAST_ID, DXL2_INST_SYNC_WRITE, parameters,
                                (uint16_t) length);
    }
    return result;
}
```

`dxl2_link_send()` also returns `DXL2_PARSE_ERROR` when byte stuffing makes the packet
larger than `DXL2_MAX_PACKET_SIZE`, so the size check above is a fast path, not the only
guard.

For SYNC_READ and BULK_READ, every addressed device answers with its own Status Packet,
in the order of the request; collect them in your handler as shown for
[broadcast PING](usage.md#writing-a-controller). FAST_SYNC_READ and FAST_BULK_READ
return one combined Status Packet whose parameter layout is described in the ROBOTIS
specification; parse it in your handler.

## Vendor-specific instructions

The parser and the encoder accept any instruction byte. If you control every node on a
bus, you can define private instructions, for example to update firmware. Choose values
that the specification does not use, keep them out of public buses, and document them —
a future revision of the protocol may assign them. Do not add them to `dxl2_defs.h`,
which only contains values from the specification.

## Bridging two buses

A gateway that forwards packets between two buses needs no special support: each bus
gets its own link, and each handler re-sends what it receives on the other link.

```c
static void forward_a_to_b(const struct dxl2_packet* packet, void* context)
{
    (void) context;
    (void) dxl2_link_send(&bus_b, packet->id, packet->instruction, packet->parameters,
                          packet->parameter_length);
}
```

Re-encoding regenerates the stuffing and the CRC, so packets that arrived corrupted are
never forwarded — the parser has already dropped them.

## Changing the configuration

`DXL2_MAX_PACKET_SIZE` is the only build-time parameter. Raise it if your devices use
large READ, SYNC or BULK transfers; lower it to save RAM on small MCUs. Define it for the
whole build, so that the library and the application agree on the structure sizes:

```cmake
target_compile_definitions(dxl2 PUBLIC DXL2_MAX_PACKET_SIZE=1024U)
```

The value must be between 10 and 65542; the build stops with an error otherwise. See
[Configuration](configuration.md) for its effect on RAM and stack.

## Contributing to the library

Changes to DXL2-C itself are welcome when they fix a bug, follow the specification more
closely, or add something every user needs. The rules keep the library certifiable:

1. **Code.** C11, no new dependencies, no heap, no global mutable state. Public symbols
   use the `dxl2_` / `DXL2_` prefix and have Doxygen comments. Follow the MISRA rules
   used in the code: one `return` per function, parentheses around mixed operators,
   `(void)` for ignored results, `++`/`--` as separate statements, array indexing instead
   of pointer arithmetic.
2. **Tests.** Every behavior change comes with a GoogleTest test in
   `tests/unit/gtest_dxl2_*.cpp` (register new files in `dxl2.cmake`). Line and branch
   coverage must stay at 100 %. When the codec changes, update the Python twin in
   `tests/integration/dxl2/protocol.py`; the cross-check compares both.
3. **Analysis.** `misra-analyze` must report 0 violations. An unavoidable deviation is
   recorded in the [deviation register](misra.md#deviation-register).
4. **Format.** `clang-format` with the repository `.clang-format` must report no changes.
5. **Documentation.** Update the affected pages in `docs/`, the stack table in
   [Configuration](configuration.md#stack-usage) when a function's frame changes, the
   diagrams (`python tools/docs/diagrams.py`) when the structure changes, and add an
   entry under `[Unreleased]` in `CHANGELOG.md`.
6. **Compatibility.** The public API is stable within a major version: do not remove or
   change public functions, structure members or return values without a major release.

Run the whole definition of done locally before opening a pull request:

```bash
cmake --preset "DXL2[UnitTests]" && cmake --build "build/DXL2[UnitTests]" --target coverage
cmake --preset "DXL2[IntegrationTests]" && cmake --build "build/DXL2[IntegrationTests]" --target integration_tests
cmake --preset "DXL2[Debug]" && cmake --build "build/DXL2[Debug]" --target misra-analyze
cmake --preset "DXL2[Examples]" && cmake --build "build/DXL2[Examples]" && ctest --test-dir "build/DXL2[Examples]" -L example
```

Pull requests target the `development` branch. CI runs the same checks and blocks the
merge if any of them fails.
