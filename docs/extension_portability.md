# Portable fisiCs Physics-Units Authoring

Status: implemented

Header: `include/fisics/extensions.h`

Last verified: 2026-09-03

## Purpose

The public portability header lets one C source retain fisiCs physics-units
annotations when compiled by fisiCs and compile as ordinary C under a control
compiler. fisiCs defines `__FISICS__` during preprocessing; other compilers
erase the annotation macros.

## Public Macros

| Macro | fisiCs expansion | Intended placement |
| --- | --- | --- |
| `FISICS_DIM(value)` | `[[fisics::dim(value)]]` | after a variable declarator |
| `FISICS_UNIT(value)` | `[[fisics::unit(value)]]` | after a variable declarator |

Example:

```c
#include <fisics/extensions.h>

double distance FISICS_DIM(length) FISICS_UNIT(meter) = 2.0;
```

The macros do not enable the physics-units overlay and do not give Clang units
semantics. Use `--overlay=physics-units` with fisiCs for semantic validation.

## Permanent Gate

Run:

```bash
bash tests/integration/run_units_portability_macros.sh ./fisics
```

The gate proves strict Clang C99 compilation, macro erasure, exact Clang/fisiCs
runtime parity, and equivalent resolved dimensions and concrete units between
native-attribute and portable-macro fixtures.

Direct attributes remain appropriate for compiler fixtures and fisiCs-only
source. External program repositories are separate migration boundaries.
