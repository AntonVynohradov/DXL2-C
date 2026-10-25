# Introduction

DXL2-C is a C11 library that implements the wire format of the
[ROBOTIS DYNAMIXEL Protocol 2.0](https://docs.robotis.com/docs/dxl/protocol/protocol2/):
how packets are framed, protected with a CRC, byte-stuffed and parsed from a stream of
bytes. It is written for embedded systems that must be predictable and reviewable, and it
runs unchanged on microcontrollers, under an RTOS and on a PC.

## What the library does

- **Encodes packets.** Given an ID, an instruction and parameters, it builds the complete
  wire packet: header, Length field, byte stuffing and CRC-16.
- **Parses a byte stream.** It accepts bytes in any chunking, finds packet headers,
  validates Length and CRC, removes byte stuffing and hands you complete packets. It
  resynchronizes by itself after noise or lost bytes.
- **Connects the codec to a transport.** The *link* layer owns a parser, calls your
  handler for every valid packet and sends replies through a write function you supply.
- **Defines the protocol constants.** Every instruction and status error value of the
  specification, plus helpers for little-endian fields.

## What the library does not do

DXL2-C is deliberately limited to the wire format. These belong to your application:

| Concern | Why it is not in the library |
| --- | --- |
| UART, RS-485, USB or socket drivers | They are hardware- and OS-specific. You pass bytes in and provide a write function. |
| Threads, queues, interrupts, locking | Every system has its own. The [integration guide](integration.md) shows the usual patterns. |
| Control Tables and device models | Addresses and registers differ between models. Your handler interprets READ, WRITE and the other instructions. |
| Timeouts and retries | A controller decides how long to wait for a reply; the library has no clock. |
| Higher-level helpers (SYNC_WRITE builders, servo APIs) | They are easy to build on top of the codec; see [Extending the Library](extending.md). |

## Who it is for

- **Device firmware.** A board that should behave like a DYNAMIXEL device — a custom
  actuator, a sensor hub or a gripper — on an existing DYNAMIXEL bus.
- **Controller firmware.** A microcontroller that drives DYNAMIXEL servos directly.
- **Host tools.** Test benches, production tools and debuggers that need a codec which
  behaves exactly like the firmware. [DXL2 Studio](gui.md) is such a tool.

## Key properties

| Property | Value |
| --- | --- |
| Language | C11 (`-std=c11`), compiles without warnings with `-Wall -Wextra -Wpedantic -Wconversion` |
| Dependencies | `<stddef.h>`, `<stdint.h>`, `<stdbool.h>`, `<string.h>` only |
| Memory | No heap; all state in caller-owned structures; sizes fixed by `DXL2_MAX_PACKET_SIZE` |
| RAM per link | about `2 × DXL2_MAX_PACKET_SIZE` + 40 bytes (≈ 550 bytes with the default 256) |
| Stack | at most about 600 bytes on Cortex-M4 for the transmit path ([details](configuration.md#stack-usage)) |
| Coding standard | MISRA C:2012, 0 Cppcheck addon violations, [3 advisory deviations](misra.md) |
| Tests | 100 % line and branch coverage; C/Python cross-check; ROBOTIS reference packets |
| Thread safety | None built in, by design; see [Threading and reentrancy](architecture.md#threading-and-reentrancy) |
| License | [Apache License 2.0](LICENSE.md) |

## Terminology

Controller
:   The node that sends Instruction Packets and waits for replies. ROBOTIS documentation
    also calls it the *main controller* or *master*.

Device
:   A node with an ID (0–252) that executes instructions and answers with Status
    Packets: a servo, a sensor, or your firmware built with DXL2-C.

Instruction Packet
:   A packet sent by the controller. Its instruction byte says what to do (PING, READ,
    WRITE, …).

Status Packet
:   The reply of a device. Its instruction byte is `0x55` and its first parameter is the
    error byte.

Broadcast ID
:   ID `254` (`0xFE`). Every device executes the instruction; only some instructions are
    answered.

Control Table
:   The register map of a device. Not part of the protocol itself and not part of this
    library.

Byte stuffing
:   The rule that keeps the header sequence out of the packet body. See
    [Byte stuffing](protocol.md#byte-stuffing).

## Conventions in this documentation

- Hexadecimal bytes are written without a prefix inside packets (`FF FF FD 00`) and with
  `0x` in text and code (`0x55`).
- Multi-byte fields are little endian, as on the wire.
- `DXL2_MAX_PACKET_SIZE` is shown with its default value, 256 bytes.
- Code examples follow the library's own style: C11, MISRA-friendly, one `return` per
  function where it matters.
