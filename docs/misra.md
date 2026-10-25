# MISRA C:2012 compliance

This page records how the library is checked against MISRA C:2012 and every
deviation from it. It covers the library code only: `src/`, `inc/` and
`dxl2.h`. Tests, examples and tools are not in scope.

## Checking

| Check | Where | Gate |
| --- | --- | --- |
| Cppcheck MISRA C:2012 addon | `misra-analyze` target, CI job *MISRA C:2012 Analysis* | Fails on any violation |
| Cppcheck, all checks | `cppcheck-analyze` target, CI job *Cppcheck Analysis* | Report |
| CodeQL `cpp-queries` | `codeql-analyze` target, CI job *CodeQL Analysis* | Report |
| Compiler | `-std=c11 -Wall -Wextra -Wpedantic -Wconversion` | No warnings |
| Manual review | Pull request review, see `.github/.copilot-instructions.md` | Reviewer |

`misra-analyze` fails when the report contains a violation. Set
`-DDXL2_MISRA_FAIL_ON_VIOLATION=OFF` to get the report without failing the
build. A violation that cannot be fixed must be added to the deviation
register below before the pull request is merged.

The Cppcheck addon checks most decidable rules, but not the directives or
rules that need knowledge of the whole program or the intent of the code.
Those are covered by the manual review notes below.

## Current status

| Item | Result |
| --- | --- |
| Cppcheck MISRA addon (2.13 in CI, 2.16) | 0 violations |
| Unit test coverage of `src/` | 100 % lines, 100 % branches |
| Deviations | 3, all advisory, listed below |

## Manual review notes

| Guideline | How the library complies |
| --- | --- |
| Dir 4.1 (run-time failures) | Every array index is bounded by a length check or by the `DXL2_MAX_PACKET_SIZE` invariant described in `dxl2_decode_packet()`. `inc/dxl2_config.h` rejects a `DXL2_MAX_PACKET_SIZE` outside `10..65542` at compile time. |
| Dir 4.11 (validity of arguments) | Every public function checks its pointer arguments and lengths and returns `DXL2_PARSE_ERROR` (or `0` for `dxl2_crc16()` and the counters). The inline little-endian helpers in `dxl2_defs.h` do not check: they take a pointer to 2 or 4 readable bytes, like `memcpy()`. |
| Dir 4.12, Rule 21.3 (dynamic memory) | No heap. All state is in caller-owned structures. |
| Rule 15.5 (single point of exit) | Every function has one `return` at the end. |
| Rule 17.2 (recursion) | No recursion; the stack depth is fixed, see [Configuration](configuration.md#stack-usage). |
| Rule 21.6 (standard I/O) | No `<stdio.h>`. The library includes only `<stddef.h>`, `<stdint.h>`, `<stdbool.h>` and `<string.h>`. |

## Deviation register

### DEV-01: `int` as the status type (Dir 4.6, advisory)

- **Location:** return type of `dxl2_parser_feed()`, `dxl2_encode()`,
  `dxl2_link_init()`, `dxl2_link_send()`, `dxl2_link_send_status()` and of
  the `dxl2_write_fn` callback.
- **Rationale:** the value is a status code (`-1`, `0`, `1`, or the code the
  write callback returned), not a quantity whose size matters. `int` lets an
  integrator pass an existing driver `write()` function as `dxl2_write_fn`
  without a wrapper. `int` is at least 16 bits on every conforming compiler,
  which holds every value the library returns.
- **Risk control:** the library never does arithmetic on these values; it only
  compares them with `0` or `DXL2_PARSE_ERROR`.

### DEV-02: macros not used inside the library (Rule 2.5, advisory)

- **Location:** `DXL2_INST_*` (except `DXL2_INST_STATUS`), `DXL2_ERR_*`,
  `DXL2_MAX_ID`, `DXL2_BROADCAST_ID` in `inc/dxl2_defs.h`;
  `DXL2_VERSION_*` in `dxl2.h`.
- **Rationale:** these are the public names of the protocol constants. They
  are used by applications, not by the codec, which handles every instruction
  the same way.

### DEV-03: external functions referenced in one translation unit (Rule 8.7, advisory)

- **Location:** the public API functions that the library itself does not
  call, for example `dxl2_link_rx_packets()` and `dxl2_link_invalid_packets()`.
- **Rationale:** they are the library interface and are called by
  applications. Giving them internal linkage would remove them from the API.
