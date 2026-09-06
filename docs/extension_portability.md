# Portable fisiCs Extension Authoring

Status: implemented and adopted by public extension examples

Header: `include/fisics/extensions.h`

Last verified: 2026-09-03

## Purpose

The portability header lets one C source retain fisiCs extension annotations
when compiled by fisiCs and compile as ordinary C under another compiler.

fisiCs already defines `__FISICS__` during preprocessing. The header expands
its macros to canonical `[[fisics::...]]` attributes only when that macro is
present. Otherwise every annotation macro expands to nothing.

The source's types, expressions, control flow, and runtime behavior must remain
valid ordinary C after those macros disappear.

## Public Macros

| Macro | fisiCs expansion | Intended placement |
| --- | --- | --- |
| `FISICS_DIM(value)` | `[[fisics::dim(value)]]` | after a variable declarator |
| `FISICS_UNIT(value)` | `[[fisics::unit(value)]]` | after a variable declarator |
| `FISICS_ID("name")` | `[[fisics::id("name")]]` | before a supported declaration |
| `FISICS_DOMAIN("name")` | `[[fisics::domain("name")]]` | before a supported declaration |
| `FISICS_ROLE("name")` | `[[fisics::role("name")]]` | before a supported declaration |
| `FISICS_PRODUCES("name")` | `[[fisics::produces("name")]]` | before a supported declaration |
| `FISICS_CONSUMES("name")` | `[[fisics::consumes("name")]]` | before a supported declaration |

Example:

```c
#include <fisics/extensions.h>

FISICS_ID("signal.room.temperature")
FISICS_DOMAIN("thermal")
FISICS_ROLE("data.read")
double room_temperature FISICS_DIM(temperature) FISICS_UNIT(kelvin) = 294.0;
```

## Compiler Commands

fisiCs validates and retains the annotations:

```bash
./fisics --overlay=physics-units,semantic-metadata \
  -Iinclude -c source.c -o source.fisics.o
```

Clang sees ordinary C:

```bash
clang -std=c99 -Wall -Wextra -Werror -pedantic \
  -Iinclude -c source.c -o source.clang.o
```

Run the permanent comparison:

```bash
make examples-semantic-metadata-compare
make examples-extension-portability
```

The broader portability target proves strict Clang C99 compilation across the
public units and semantic-metadata examples, exact Clang/fisiCs runtime parity
where execution is meaningful, all seven macro spellings, and normalized
native-attribute versus macro semantic/units parity.

## Adoption Boundary

All public C sources under `examples/` that use units or semantic metadata now
use `<fisics/extensions.h>` and the portability macros. This includes the
physics-units pilot, its intentional semantic failures, the units-bearing
curated projects, the basic semantic example, and the complete static hardware
example.

Direct attributes remain intentionally present in compiler source strings,
parser/API/integration fixtures, final probes, and the dedicated native parity
fixture. Those surfaces test native fisiCs syntax and must not be mechanically
rewritten.

External program repositories are separate customer-adoption boundaries. The
header contract does not claim that every existing CodeWork customer has
already migrated.

## Guarantees

- The macro spelling is a source-portability adapter only.
- Direct `[[fisics::...]]` syntax remains canonical and supported.
- fisiCs receives the same attribute payloads after preprocessing.
- Non-fisiCs compilers do not receive unknown fisiCs attributes.
- The header introduces no runtime declarations, storage, calls, or link
  dependency.
- The header does not enable an overlay; the fisiCs command must still select
  the required overlay explicitly.

## Non-Guarantees

- Clang does not validate units or semantic metadata and emits no fisiCs
  metadata artifacts.
- Removing annotations does not make extension-invalid calculations safe or
  correct; it merely produces the ordinary-C control build.
- This contract does not promise compatibility with every C compiler or every
  warning policy. The permanent public comparison currently proves local Clang
  in strict C99 mode.
- The macros do not represent semantic project graph, physical composition graph, or hardware compatibility graph manifests and schemas.
- No macro or static artifact establishes runtime or physical truth.

## Permanent Gate

```bash
make integration-extension-portability-macros
```

The gate verifies:

- macro erasure in Clang preprocessing output;
- strict `-std=c99 -Wall -Wextra -Werror -pedantic` compilation;
- exact runtime output parity for public executable examples;
- normalized translation-unit metadata export record/edge parity;
- normalized units attachment parity;
- ordinary-C compilation of intentionally invalid fisiCs units examples; and
- separate continued rejection of those examples by the units overlay's
  existing integration lane.
