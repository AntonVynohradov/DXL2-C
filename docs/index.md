# DXL2-C: DYNAMIXEL Protocol 2.0 in C

[![Build Static Library](https://github.com/AntonVynohradov/dxl2-c/actions/workflows/build-static-lib.yml/badge.svg)](https://github.com/AntonVynohradov/dxl2-c/actions/workflows/build-static-lib.yml)
[![Build and Deploy Documentation](https://github.com/AntonVynohradov/dxl2-c/actions/workflows/documentation.yml/badge.svg)](https://github.com/AntonVynohradov/dxl2-c/actions/workflows/documentation.yml)

Hardware- and OS-agnostic implementation of the
[ROBOTIS DYNAMIXEL Protocol 2.0](https://docs.robotis.com/docs/dxl/protocol/protocol2/)
wire format in C, for embedded systems developed under MISRA C:2012.

## Key features

- CRC-16 (polynomial `0x8005`), byte stuffing, incremental byte-stream parser
- Transport-independent link: bytes in, packets out, through user callbacks
- No heap, no RTOS, no driver headers, no global mutable state
- All state in caller-owned structures; buffer sizes fixed at build time
- Python twin of the codec, cross-checked against the C encoder and parser

## Intended use

The library is intended for firmware that talks to DYNAMIXEL devices or
emulates one, and for host tools that need the same codec. It implements the
wire format only: device models, Control Tables and transports (UART, USB,
threads, queues) belong to the application.

## Quick start

```bash
cmake --preset "DXL2[Release]"
cmake --build "build/DXL2[Release]"
```

See [Getting Started](guide.md) for presets, tests and static analysis, and
[Integration](integration.md) for connecting the library to a UART.

## Documentation

- [Getting Started](guide.md)
- [Configuration](configuration.md)
- [API Reference](api.md)
- [Wire Format](protocol.md)
- [Integration](integration.md)
