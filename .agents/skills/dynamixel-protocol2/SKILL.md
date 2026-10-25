---
name: dynamixel-protocol2
description: "Use when working on the dxl2 DYNAMIXEL Protocol 2.0 library: the C codec (CRC-16, parser, byte stuffing, encoder), the transport-independent link, the Python codec, their tests, the examples (bare metal, Zephyr, host serial), MISRA C:2012 compliance, releases, or integrating the library into firmware or host tools."
---

# dxl2: DYNAMIXEL Protocol 2.0 library

## Scope

This repository is a portable C11 static library plus a Python twin of the
codec. Version 1.0.0 is released: the public API in `dxl2.h` and `inc/` is
stable and follows Semantic Versioning. It implements the Protocol 2.0 **wire format** only. Device models,
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
| Examples | `examples/bare_metal`, `examples/host_serial`, `examples/zephyr` | Not library code; built with `DXL2_EXAMPLES=ON` (Zephyr with `west`) |
| Tools | `tools/dxl2_gui` | DXL2 Studio GUI; its native bridge links the library |

## Hard rules

- The library sources include only `<stddef.h>`, `<stdint.h>`, `<stdbool.h>`,
  `<string.h>` and DXL2 headers (`#include "dxl2_*.h"`). No heap, no RTOS, no driver headers, no
  global mutable state (all state lives in caller-owned structs).
- Public symbols use the `dxl2_` / `DXL2_` prefix. Public headers keep the
  `extern "C"` guards and Doxygen comments for every public item.
- Keep the public API stable within a major version: no removed or changed
  public symbols, struct members or return values without a major version
  bump. Record every user-visible change under `[Unreleased]` in
  `CHANGELOG.md`.
- The version comes from the `vX.Y.Z` Git tag (`cmake/version.cmake`); keep
  the fallbacks `DXL2_VERSION_*` and `DXL2_VERSION` in `dxl2.h` equal to
  `DXL2_DEFAULT_VERSION` there.
- MISRA C:2012: `misra-analyze` must report 0 violations (it fails the build
  otherwise). An unavoidable violation needs an entry in the deviation
  register in `docs/misra.md` (rule, location, rationale). Library functions
  have a single `return` at the end, parenthesize mixed-operator expressions,
  cast ignored return values to `(void)`, keep `++`/`--` as separate
  statements, and index arrays instead of doing pointer arithmetic.
- Every public function validates its pointer arguments and lengths
  (Dir 4.11). Do not add code that cannot be reached; prove bounds from the
  `DXL2_MAX_PACKET_SIZE` invariant instead (see `dxl2_decode_packet()`).
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
cmake --preset "DXL2[Examples]" && cmake --build "build/DXL2[Examples]" && ctest --test-dir "build/DXL2[Examples]" -L example
```

The `coverage` target fails below 100 % line or branch coverage of `src/`
(`DXL2_COVERAGE_MIN`); `misra-analyze` fails on any violation. The
bare-metal example runs against a simulated board (`board_host.c`) that
checks every reply, so keep it passing when the link behavior changes.

Unit tests are GoogleTest files `tests/unit/gtest_dxl2_*.cpp` (register new
files in `dxl2.cmake`). The pytest suite in `tests/integration/` includes a
cross-check that compiles `src/dxl2_protocol.c` with `gcc` and compares 300
random packets against the Python encoder and parser; it is skipped without
`gcc`, so say so when reporting results on such a machine.

Reference vectors (from the specification) that must keep passing:
PING `FF FF FD 00 01 03 00 01 19 4E`, status
`FF FF FD 00 01 07 00 55 00 06 04 26 65 5D`.

## Documentation

The documentation is a MkDocs Material website (`mkdocs.yml`, pages in
`docs/`) and a printable PDF manual built from the same pages
(`python tools/docs/build_manual.py`; chapter order = `mkdocs.yml` nav,
`index.md` and the Doxygen `reference/` pages are website-only). All
documentation is in English.

| Page | Contents |
| --- | --- |
| `overview.md`, `guide.md` | Introduction, getting started, build, docs tooling |
| `architecture.md`, `usage.md`, `extending.md` | How it works, how to use it, how to extend it |
| `protocol.md`, `integration.md`, `examples.md`, `gui.md` | Wire format, transports, examples, DXL2 Studio |
| `api.md`, `configuration.md`, `misra.md` | API reference, configuration and stack usage, MISRA |
| `about.md`, `LICENSE.md`, `changelog.md` | Author and company, license, changelog (included from `CHANGELOG.md`) |

Update the affected pages, `README.md` and `CHANGELOG.md` together with any
API or behavior change; re-measure the stack table with `-fstack-usage` when a
function's frame changes. The documentation tooling is the shared base of the personal
`sfs-protocol-library` skill (SystemFromScratch® protocol libraries); keep it
identical to the skill's templates and change project data, not structure:
the landing page `overrides/home.html` is driven by `mkdocs.yml → extra.landing`
(hero, problem cards, offer cards, stats, portfolio), the PDF builder
`tools/docs/build_manual.py` by `extra.manual`, and `docs/index.md` only holds
the Quick start Markdown. Diagrams, including the animated isometric hero, are
generated by `tools/docs/diagrams.py` with `tools/docs/svgkit.py`: edit the
script and run it, never edit the SVG files. Links between pages are relative `.md`
links (the PDF builder turns them into page references). Avoid Material-only
markup (grid cards, icons, tabs) outside `index.md` and `about.md`, because the
PDF does not render it. Author: Anton Vynohradov
(github.com/AntonVynohradov, linkedin.com/in/vynohradov), company System From
Scratch (systemfromscratch.com).

## Releases

1. Set `DXL2_DEFAULT_VERSION` in `cmake/version.cmake` and the fallbacks in
   `dxl2.h` to the new version.
2. Move the `[Unreleased]` entries in `CHANGELOG.md` to `[X.Y.Z] - date`.
3. Merge to `main`, then push the tag `vX.Y.Z`. The Release workflow
   (`.github/workflows/release.yml`) refuses a tag that does not match these
   files, runs all quality gates and publishes the library, headers and MISRA
   report as a GitHub Release.

Never push a tag or publish a release without the maintainer's explicit
request.
