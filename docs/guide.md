# Getting Started

## Prerequisites

- C11 compiler (GCC, Clang, or an embedded toolchain)
- CMake 3.20+
- Optional: GoogleTest and gcovr (unit tests, coverage), Python 3 and pytest
  (integration tests), Cppcheck (static analysis, MISRA C:2012 addon),
  clang-format, MkDocs with mkdoxy (documentation)

## Build with presets

| Preset | Purpose |
| --- | --- |
| `DXL2[Debug]` | Static library, `-g`, `DXL2_DEBUG` defined |
| `DXL2[Release]` | Static library, `-O2 -DNDEBUG` |
| `DXL2[UnitTests]` | GoogleTest executable with coverage instrumentation |
| `DXL2[IntegrationTests]` | pytest suite run through CTest |

```bash
cmake --preset "DXL2[Debug]"
cmake --build "build/DXL2[Debug]"
```

All builds use `-std=c11 -Wall -Wextra -Wpedantic -Wconversion`.

## Unit tests and coverage

```bash
cmake --preset "DXL2[UnitTests]"
cmake --build "build/DXL2[UnitTests]" --target coverage
```

The `coverage` target builds the tests, runs them through CTest and writes an
HTML report to `build/DXL2[UnitTests]/Coverage/coverage.html`.

## Integration tests

```bash
python -m pip install pytest
cmake --preset "DXL2[IntegrationTests]"
cmake --build "build/DXL2[IntegrationTests]" --target integration_tests
```

The suite in `tests/integration/` tests the Python codec and compiles
`src/dxl2_protocol.c` with `gcc` to compare 300 random packets between the C
and Python implementations. The cross-check is skipped when `gcc` is not on
the path.

## Static analysis

From any configured build directory:

| Target | Tool |
| --- | --- |
| `cppcheck-analyze` | Cppcheck, all checks, report in `cppcheck.txt` |
| `misra-analyze` | Cppcheck MISRA C:2012 addon on `src/`, report in `misra.txt` |
| `codeql-analyze` | CodeQL `cpp-queries`, report in `codeql-report.sarif` |

MISRA rule texts are not distributed with Cppcheck. If you have a licensed
copy, pass it with `-DDXL2_MISRA_RULE_TEXTS=<file>` to get rule headlines in
the report.

## Formatting

```bash
clang-format -i src/*.c inc/*.h dxl2.h tests/unit/*.cpp tests/unit/*.h
```

CI rejects pull requests with unformatted C/C++ files.

## Versioning

`cmake/version.cmake` takes the version from the latest `vX.Y.Z` Git tag and
passes it as `DXL2_VERSION_MAJOR/MINOR/PATCH`. Without a tag it uses the
default version and prints a warning; keep the fallback values in `dxl2.h`
in sync with that default.
