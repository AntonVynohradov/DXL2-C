# Configuration

Build-time parameters live in `inc/dxl2_config.h`. Each one is guarded by
`#ifndef`, so override it with a compiler definition instead of editing the
file:

```cmake
target_compile_definitions(dxl2 PUBLIC DXL2_MAX_PACKET_SIZE=1024U)
```

| Parameter | Default | Meaning |
| --- | --- | --- |
| `DXL2_MAX_PACKET_SIZE` | `256U` | Largest complete wire packet (header to CRC) the parser and encoder accept |

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

## CMake options

| Option | Default | Meaning |
| --- | --- | --- |
| `DXL2_UNIT_TESTS` | `OFF` | Build the GoogleTest suite and the `coverage` target |
| `DXL2_INTEGRATION_TESTS` | `OFF` | Register the pytest suite with CTest |
| `DXL2_MISRA_RULE_TEXTS` | empty | MISRA C:2012 rule texts file for the `misra-analyze` target |
