# Getting Started

This chapter shows how to get DXL2-C, add it to your project, and build and test it.

## Get the library

Clone the repository, or download a release package from the
[GitHub releases](https://github.com/AntonVynohradov/DXL2-C/releases) page:

```bash
git clone https://github.com/AntonVynohradov/DXL2-C.git
```

Each release provides the source code, a prebuilt static library for Linux x86-64 with
its headers, and the MISRA report.

## Add it to your project

The library is two C files and five headers. Choose whichever fits your build:

**CMake.** Include `dxl2.cmake`; it defines the static library target `dxl2` with its
include directories and version definitions:

```cmake
include(path/to/DXL2-C/dxl2.cmake)
target_link_libraries(app PRIVATE dxl2)
```

**Any other build system** (Make, IDE projects, vendor tools). Compile
`src/dxl2_protocol.c` and `src/dxl2_link.c` with your firmware and add the repository
root and `inc/` to the include path. No other files and no special flags are needed.

**Prebuilt package.** Link `lib/libdxl2.a` and add `include/` to the include path. Use
it only for the platform it was built for.

Then include the umbrella header and continue with [Using the Library](usage.md):

```c
#include "dxl2.h"
```

## Prerequisites for development

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
| `DXL2[Examples]` | Host examples and the bare-metal simulation test |

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
HTML report to `build/DXL2[UnitTests]/Coverage/coverage.html`. It fails when
line or branch coverage of `src/` is below `DXL2_COVERAGE_MIN` (100 %).

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

`misra-analyze` fails when the report contains a violation; see
[MISRA C:2012](misra.md) for the deviation register.

MISRA rule texts are not distributed with Cppcheck. If you have a licensed
copy, pass it with `-DDXL2_MISRA_RULE_TEXTS=<file>` to get rule headlines in
the report.

## Formatting

```bash
clang-format -i src/*.c inc/*.h dxl2.h tests/unit/*.cpp tests/unit/*.h examples/*/*.[ch] examples/zephyr/src/*.c
```

CI rejects pull requests with unformatted C/C++ files.

## Versioning and releases

`cmake/version.cmake` takes the version from the latest `vX.Y.Z` Git tag and
passes it as `DXL2_VERSION_MAJOR/MINOR/PATCH` and the string `DXL2_VERSION`.
Without a tag it uses `DXL2_DEFAULT_VERSION` and prints a warning; keep the
fallback values in `dxl2.h` equal to that default (a unit test checks that
the string matches the numbers).

To release version X.Y.Z:

1. Set `DXL2_DEFAULT_VERSION` in `cmake/version.cmake` and the fallbacks in
   `dxl2.h` to X.Y.Z.
2. Move the `[Unreleased]` entries in `CHANGELOG.md` to a `[X.Y.Z] - date`
   section.
3. Merge to `main`, then tag and push: `git tag vX.Y.Z && git push origin vX.Y.Z`.

The Release workflow refuses a tag that does not match these files, runs all
quality gates and publishes the GitHub Release.

## Documentation

The documentation is written in Markdown in `docs/` and published with MkDocs (Material
theme); the API pages are generated from the source comments by Doxygen through mkdoxy.
Install the tools into the project virtual environment (VS Code task
*Python: Setup venv*, or `python -m pip install -r requirements-dev.txt`) and Doxygen.

| Task | Command | VS Code task |
| --- | --- | --- |
| Live preview | `mkdocs serve` | *Docs: Serve on localhost* |
| Build the website into `site/` | `mkdocs build` | *Docs: Build site* |
| Build the printable PDF manual | `python tools/docs/build_manual.py` | *Docs: Build PDF manual* |

The PDF manual is written to `build/manual/DXL2-C_User_Manual_v<version>.pdf`. It is an
A4 document with a cover page, an imprint, a table of contents with page numbers,
numbered chapters, running headers, page numbers and PDF bookmarks. It needs WeasyPrint,
which uses the Pango library: on Linux install `libpango-1.0-0` and `libpangoft2-1.0-0`
(Debian, Ubuntu); on Windows install the MSYS2 package `mingw-w64-x86_64-pango` — the
script finds `C:\msys64\mingw64\bin` by itself, or set `WEASYPRINT_DLL_DIRECTORIES`.

Diagrams are SVG files generated by `python tools/docs/diagrams.py`; edit the script, not
the SVG files.
