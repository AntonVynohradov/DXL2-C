# Changelog

All notable changes to DXL2-C are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/): the public API in `dxl2.h` and
`inc/` stays compatible within a major version.

## [Unreleased]

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

[Unreleased]: https://github.com/AntonVynohradov/DXL2-C/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/AntonVynohradov/DXL2-C/releases/tag/v1.0.0
