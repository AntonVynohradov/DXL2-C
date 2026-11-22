# Changelog

All notable changes to DXL2-C are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/): the public API in `dxl2.h` and
`inc/` stays compatible within a major version.

## [Unreleased]

## [2.0.0] - 2026-11-22

### Changed

- **Breaking:** the parser removes byte stuffing in place, in its own buffer.
  `struct dxl2_packet` no longer holds a copy of the parameters:
  `parameters` is now `const uint8_t*` and points into the buffer of the
  parser that decoded the packet. The parameters stay valid until the next
  `dxl2_parser_feed()` or `dxl2_parser_init()` with that parser. Code that
  reads `packet->parameters[i]` in the packet handler or right after
  `dxl2_parser_feed()` returns 1 compiles and works unchanged; code that keeps
  a copy of the structure must copy the parameter bytes instead (as
  `tools/dxl2_gui/native/dxl2_host.c` now does).
- RAM: `struct dxl2_packet` shrinks from `DXL2_MAX_PARAMETER_SIZE` + 4 bytes to
  8 bytes on a 32-bit MCU (16 on a 64-bit host); `struct dxl2_link` on
  Cortex-M4 from 536 to 292 bytes with the default `DXL2_MAX_PACKET_SIZE`
  (256), from 1,560 to 804 bytes with 768. The stack frames are unchanged.

## [1.0.0] - 2026-10-25

First public release.

### Added

- DYNAMIXEL Protocol 2.0 codec: CRC-16 (polynomial `0x8005`), byte stuffing
  and unstuffing, incremental byte-stream parser that resynchronizes on the
  next header, encoder with output-capacity checks.
- Transport-independent link (`dxl2_link_*`): bytes in, packets out, through a
  user write callback and packet handler; valid and invalid packet counters.
- All instruction and status error values of the ROBOTIS specification, and
  little-endian helpers (`dxl2_get_u16/u32`, `dxl2_put_u16/u32`).
- Build-time configuration `DXL2_MAX_PACKET_SIZE` (default 256 bytes), checked
  at compile time.
- Examples: bare metal (UART interrupt, ring buffer, main loop; runs in CI on a
  simulated board), host serial port PING tool (Linux, macOS, Windows), Zephyr
  application.
- DXL2 Studio GUI (`tools/dxl2_gui`) for simulation and serial-port debugging.
- Documentation website (MkDocs Material): introduction, getting started,
  architecture, usage, extension guide, wire format with diagrams, integration
  guide, examples, DXL2 Studio, API reference (hand-written and Doxygen),
  configuration with measured stack usage, MISRA C:2012 compliance, author and
  license pages.
- Printable A4 PDF user manual with cover, table of contents and page numbers
  (`tools/docs/build_manual.py`), published with the website and every release.

### Quality

- MISRA C:2012: 0 violations reported by the Cppcheck addon; three advisory
  deviations recorded in `docs/misra.md`. CI fails on any violation.
- Unit tests (GoogleTest): 100 % line and branch coverage of `src/`, enforced
  in CI.
- Integration tests (pytest): Python codec and a C/Python cross-check on
  random packets; ROBOTIS reference packets as regression vectors.
- Cppcheck and CodeQL static analysis in CI.

[Unreleased]: https://github.com/AntonVynohradov/DXL2-C/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/AntonVynohradov/DXL2-C/compare/v1.0.0...v2.0.0
[1.0.0]: https://github.com/AntonVynohradov/DXL2-C/releases/tag/v1.0.0
