# DXL2-C: Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library

[![Build Static Library](https://github.com/AntonVynohradov/dxl2-c/actions/workflows/build-static-lib.yml/badge.svg)](https://github.com/AntonVynohradov/dxl2-c/actions/workflows/build-static-lib.yml)
[![Build and Deploy Documentation](https://github.com/AntonVynohradov/dxl2-c/actions/workflows/documentation.yml/badge.svg)](https://github.com/AntonVynohradov/dxl2-c/actions/workflows/documentation.yml)

## Overview

DXL2-C is a C implementation of the
[ROBOTIS DYNAMIXEL Protocol 2.0](https://docs.robotis.com/docs/dxl/protocol/protocol2/)
wire format. It does not depend on hardware, an operating system or a heap, and
it is developed for embedded systems that must comply with **MISRA C:2012**.

### Project Status

🚧 **ARCHITECTURE REWORK IN PROGRESS** 🚧

The project structure, build, tests and CI are in place. The next stage is the
library architecture and MISRA C:2012 compliance.

---

## About DYNAMIXEL Protocol 2.0

Protocol 2.0 is the half-duplex serial protocol of ROBOTIS DYNAMIXEL
actuators. Every packet starts with the header `FF FF FD 00`, carries an ID,
a little-endian length, an instruction and parameters, and ends with a CRC-16.
A byte-stuffing rule keeps the header sequence out of the payload.

**Key Capabilities:**
- CRC-16 (polynomial `0x8005`), byte stuffing and unstuffing
- Incremental byte-stream parser that resynchronizes after noise
- Packet encoder with output-capacity checks
- Transport-independent link: bytes in, packets out, through user callbacks
- All instructions and status error codes of the specification
- Python twin of the codec, cross-checked against the C implementation

---

## Objectives

### 🎯 Primary Goals

1. **Embedded Systems Readiness**
   - No dynamic memory, no RTOS, no driver dependencies
   - All state in caller-owned structures, sizes fixed at build time
   - Deterministic behavior and documented stack usage

2. **Comprehensive Testing**
   - Unit test coverage: **100%** (GoogleTest + gcovr)
   - Integration tests and C/Python cross-check (pytest)
   - ROBOTIS reference packets as regression vectors

3. **Professional Documentation**
   - API reference (Doxygen via MkDocs)
   - Wire format and parser behavior
   - Integration guides and examples

4. **MISRA C:2012 Compliance**
   - Cppcheck MISRA addon in CI
   - Documented deviations
   - Cppcheck and CodeQL static analysis

---

## Roadmap

- [x] Project structure, CMake presets, CI workflows
- [x] Unit tests on GoogleTest, integration tests on pytest
- [ ] Library architecture rework
- [ ] MISRA C:2012 compliance (baseline: `misra-analyze` report)
- [ ] 100% unit test coverage
- [ ] Comprehensive API documentation
- [ ] Integration examples (bare metal, Zephyr, host serial port)

---

## Building & Usage

### Prerequisites
- C11 compiler (GCC, Clang, or an embedded toolchain)
- CMake 3.20+ (required)
- Optional: GoogleTest and gcovr (unit tests), Python 3 and pytest
  (integration tests), Cppcheck (static analysis), MkDocs (documentation)

### Build with presets

```bash
cmake --preset "DXL2[Release]"
cmake --build "build/DXL2[Release]"
```

| Preset | Purpose |
| --- | --- |
| `DXL2[Debug]` | Static library with debug information |
| `DXL2[Release]` | Optimized static library |
| `DXL2[UnitTests]` | GoogleTest suite; `--target coverage` for the report |
| `DXL2[IntegrationTests]` | pytest suite; `--target integration_tests` |

Static analysis targets: `cppcheck-analyze`, `misra-analyze`, `codeql-analyze`.
See [docs/guide.md](docs/guide.md) for details.

### Configuration Options
- `-DDXL2_UNIT_TESTS=ON` - Build the GoogleTest suite and the `coverage` target
- `-DDXL2_INTEGRATION_TESTS=ON` - Register the pytest suite with CTest
- `-DDXL2_MISRA_RULE_TEXTS=<file>` - MISRA rule texts for readable `misra-analyze` output
- `-DDXL2_MAX_PACKET_SIZE=<bytes>` (compiler definition) - Largest wire packet, default `256U`

### Using the library in another CMake project

```cmake
include(path/to/dxl2/dxl2.cmake)
target_link_libraries(app PRIVATE dxl2)
```

### Quick Start Example

```c
#include "dxl2.h"

static struct dxl2_link link;

static int uart_write(void *context, const uint8_t *data, size_t length)
{
    (void) context;
    return board_uart_write(data, length); /* 0 on success */
}

static void on_packet(const struct dxl2_packet *packet, void *context)
{
    (void) context;
    if (packet->instruction == DXL2_INST_PING)
    {
        static const uint8_t info[] = {0x06, 0x04, 0x26}; /* model 1030, firmware 38 */
        (void) dxl2_link_send_status(&link, 1U, DXL2_ERR_NONE, info, sizeof(info));
    }
}

int main(void)
{
    const struct dxl2_link_config config = {
        .write = uart_write,
        .handler = on_packet,
    };

    (void) dxl2_link_init(&link, &config);

    for (;;)
    {
        uint8_t bytes[32];
        size_t count = board_uart_read(bytes, sizeof(bytes));

        dxl2_link_receive(&link, bytes, count);
    }
}
```

---

## Project Structure

```
dxl2-c/
├── src/                    # Source implementation files
│   ├── dxl2_link.c
│   └── dxl2_protocol.c
├── inc/                    # Public header files
│   ├── dxl2_config.h
│   ├── dxl2_defs.h
│   ├── dxl2_link.h
│   └── dxl2_protocol.h
├── tests/
│   ├── unit/               # GoogleTest unit tests
│   └── integration/        # pytest suite, Python codec (dxl2/)
├── docs/                   # Documentation (MkDocs)
│   ├── api.md
│   ├── configuration.md
│   ├── guide.md
│   ├── index.md
│   ├── integration.md
│   ├── protocol.md
│   └── LICENSE.md
├── cmake/                  # CMake helpers
│   ├── integration_tests.cmake
│   ├── misra_check.cmake
│   ├── unit_tests.cmake
│   └── version.cmake
├── tools/
│   └── dxl2_gui/           # DXL2 Studio: GUI for simulation and serial-port debugging
├── .github/                # CI workflows, PR template, review guidelines
├── CMakeLists.txt
├── CMakePresets.json
├── cppcheck-excludes.txt
├── dxl2.cmake
├── dxl2.h
├── LICENSE
├── mkdocs.yml
└── README.md
```

---

## Tools

**DXL2 Studio** ([tools/dxl2_gui](tools/dxl2_gui/README.md)) is a desktop GUI built on the
static library. It runs a simulated device on the library or talks to a real device over a
serial port, and shows every packet with a field-by-field decode, latency and error statistics:

```bash
python -m pip install -r tools/dxl2_gui/requirements.txt
python tools/dxl2_gui/dxl2_gui.py
```

---

## Contributing

Contributions are welcome. Please ensure:

- Code follows MISRA C:2012; `misra-analyze` reports no new violations
- Changes include GoogleTest unit tests
- `clang-format` reports no changes
- Documentation is updated accordingly

See [.github/.copilot-instructions.md](.github/.copilot-instructions.md) for
the review guidelines.

---

## License

Licensed under the Apache License 2.0. See [LICENSE](LICENSE) for details.

---

## References

- [ROBOTIS DYNAMIXEL Protocol 2.0](https://docs.robotis.com/docs/dxl/protocol/protocol2/)
- [ROBOTIS DynamixelSDK](https://github.com/ROBOTIS-GIT/DynamixelSDK)
- MISRA C:2012 Guidelines for the use of the C language in critical systems

---

**Last Updated:** October 2026
**Status:** Active Development
**Target Release:** TBD
