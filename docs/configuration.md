# Configuration

Build-time parameters live in `inc/dxl2_config.h`. Each one is guarded by
`#ifndef`, so override it with a compiler definition instead of editing the
file:

```cmake
target_compile_definitions(dxl2 PUBLIC DXL2_MAX_PACKET_SIZE=1024U)
```

| Parameter | Default | Meaning |
| --- | --- | --- |
| `DXL2_MAX_PACKET_SIZE` | `256U` | Largest complete wire packet (header to CRC) the parser and encoder accept; `10U..65542U`, checked at compile time |

Derived values:

| Macro | Value |
| --- | --- |
| `DXL2_MAX_PARAMETER_SIZE` | `DXL2_MAX_PACKET_SIZE - 10U` |

## Memory footprint

`DXL2_MAX_PACKET_SIZE` sizes three buffers:

| Object | Size |
| --- | --- |
| `struct dxl2_parser` | `DXL2_MAX_PACKET_SIZE` + `size_t` |
| `struct dxl2_packet` | `DXL2_MAX_PARAMETER_SIZE` + 4 bytes |
| `struct dxl2_link` | parser + packet + configuration + two counters |
| `dxl2_link_send()` stack | `DXL2_MAX_PACKET_SIZE` |
| `dxl2_link_send_status()` stack | `DXL2_MAX_PARAMETER_SIZE` + the `dxl2_link_send()` frame |

## Stack usage

The library does not recurse and does not use variable-length arrays, so the
stack depth is fixed. Measured with `-fstack-usage`, default
`DXL2_MAX_PACKET_SIZE` (256), bytes per function frame:

| Function | Cortex-M4 `-Os` | Cortex-M4 `-O0` | x86-64 `-O2` |
| --- | --- | --- | --- |
| `dxl2_link_send_status()` | 272 | 296 | 336 |
| `dxl2_link_send()` | 296 | 312 | 368 |
| `dxl2_encode()` | 24 | 56 | 32 |
| `dxl2_crc16()` | 8 | 32 | 8 |
| `dxl2_link_receive()` | 24 | 32 | 80 |
| `dxl2_parser_feed()` (with `dxl2_decode_packet()`) | 32 | 32 + 56 | 80 |
| `dxl2_link_init()` | 16 | 32 | 48 |
| other functions | 0 | 16 | 8 |

Toolchains: Arm GNU Toolchain 15.2 (`-mcpu=cortex-m4 -mthumb`), GCC 14.2.
At `-O0` the small static functions (`dxl2_get_u16()`, `dxl2_parser_reset()`)
add 16 bytes each where they are called.

Worst-case call chains on Cortex-M4 `-Os`, without the callbacks:

| Path | Chain | Stack |
| --- | --- | --- |
| Transmit | `dxl2_link_send_status` → `dxl2_link_send` → `dxl2_encode` → `dxl2_crc16` | 600 B + `write` callback |
| Receive | `dxl2_link_receive` → `dxl2_parser_feed` → `dxl2_crc16` | 64 B + packet handler |
| Receive and reply | `dxl2_link_receive` → handler → transmit chain | 624 B + handler + `write` |

The two large frames are the transmit buffers, so they scale with
`DXL2_MAX_PACKET_SIZE`: about `2 × DXL2_MAX_PACKET_SIZE + 90` bytes for the
transmit chain. Measure your own build with:

```bash
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -Os -std=c11 -fstack-usage -Iinc -I. -c src/dxl2_link.c src/dxl2_protocol.c
cat *.su
```

## CMake options

| Option | Default | Meaning |
| --- | --- | --- |
| `DXL2_UNIT_TESTS` | `OFF` | Build the GoogleTest suite and the `coverage` target |
| `DXL2_INTEGRATION_TESTS` | `OFF` | Register the pytest suite with CTest |
| `DXL2_MISRA_RULE_TEXTS` | empty | MISRA C:2012 rule texts file for the `misra-analyze` target |
| `DXL2_EXAMPLES` | `OFF` | Build the host examples (`examples/`) and register the bare-metal simulation with CTest |
| `DXL2_COVERAGE_MIN` | `100` | Line and branch coverage (%) below which the `coverage` target fails |
| `DXL2_MISRA_FAIL_ON_VIOLATION` | `ON` | Make `misra-analyze` fail when the report has a violation |
