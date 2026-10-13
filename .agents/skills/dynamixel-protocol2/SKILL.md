---
name: dynamixel-protocol2
description: "Use when working on the dxl2 DYNAMIXEL Protocol 2.0 library: the C codec (CRC-16, parser, byte stuffing, encoder), the transport-independent link, the Python codec, their tests, or integrating the library into firmware or host tools."
---

# dxl2: DYNAMIXEL Protocol 2.0 library

## Scope

This repository is a portable C11 static library plus a Python twin of the
codec. It implements the Protocol 2.0 **wire format** only. Device models,
Control Tables, motor control and transports (UART/USB drivers, threads,
queues) belong to the applications that use it, not here.

| Part | Files | Rule |
| --- | --- | --- |
| Umbrella | `dxl2.h` | Includes all public headers; fallback `DXL2_VERSION_*` |
| Configuration | `inc/dxl2_config.h` | Build-time sizes, each overridable with `#ifndef` |
| Codec | `inc/dxl2_protocol.h`, `src/dxl2_protocol.c` | CRC-16 (poly `0x8005`, init 0), incremental parser, encoder with stuffing |
| Constants | `inc/dxl2_defs.h` | Values only from the ROBOTIS specification |
| Link | `inc/dxl2_link.h`, `src/dxl2_link.c` | No OS calls; I/O only through `dxl2_write_fn`, dispatch through `dxl2_packet_handler_t` |
| Python | `tests/integration/dxl2/protocol.py` | Standard library only; must stay byte-identical to the C encoder |
| Build | `CMakeLists.txt`, `dxl2.cmake`, `cmake/`, `CMakePresets.json` | Library target in `dxl2.cmake`; tests and analysis in `cmake/` |

## Hard rules

- The library sources include only `<stddef.h>`, `<stdint.h>`, `<stdbool.h>`,
  `<string.h>` and DXL2 headers (`#include "dxl2_*.h"`). No heap, no RTOS, no driver headers, no
  global mutable state (all state lives in caller-owned structs).
- Public symbols use the `dxl2_` / `DXL2_` prefix. Public headers keep the
  `extern "C"` guards and Doxygen comments for every public item.
- Keep the public API stable within a major version. The version comes from
  the `vX.Y.Z` Git tag (`cmake/version.cmake`); keep the fallback
  `DXL2_VERSION_*` in `dxl2.h` equal to `DXL2_DEFAULT_VERSION` there.
- Target MISRA C:2012: `misra-analyze` must not report new violations without
  a documented deviation.
- Behavior is defined by the ROBOTIS specification
  (https://docs.robotis.com/docs/dxl/protocol/protocol2/). Do not invent
  instruction or error values. When the specification is unclear, record what
  the ROBOTIS DynamixelSDK does, with the commit and file.
- Code must compile without warnings with
  `-std=c11 -Wall -Wextra -Wconversion -Wpedantic`.
- Style: `.clang-format` (4 spaces, Allman braces, 100 columns); file banners
  and section comments as in the existing files (`.vscode/cpp.json.code-snippets`).

## Tests (definition of done)

Every codec or link behavior change needs a test. All of these must pass:

```sh
cmake --preset "DXL2[Debug]" && cmake --build "build/DXL2[Debug]"
cmake --preset "DXL2[UnitTests]" && cmake --build "build/DXL2[UnitTests]" --target coverage
cmake --preset "DXL2[IntegrationTests]" && cmake --build "build/DXL2[IntegrationTests]" --target integration_tests
cmake --build "build/DXL2[Debug]" --target misra-analyze
```

Unit tests are GoogleTest files `tests/unit/gtest_dxl2_*.cpp` (register new
files in `dxl2.cmake`). The pytest suite in `tests/integration/` includes a
cross-check that compiles `src/dxl2_protocol.c` with `gcc` and compares 300
random packets against the Python encoder and parser; it is skipped without
`gcc`, so say so when reporting results on such a machine.

Reference vectors (from the specification) that must keep passing:
PING `FF FF FD 00 01 03 00 01 19 4E`, status
`FF FF FD 00 01 07 00 55 00 06 04 26 65 5D`.

## Documentation

`README.md` (overview, build, API summary), `docs/api.md`,
`docs/configuration.md` (parameters, memory footprint), `docs/protocol.md`
(wire format and parser behavior), `docs/integration.md` (transport
integration). Update
them together with any API or behavior change.
