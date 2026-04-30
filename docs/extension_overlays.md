# Extension Overlays

`fisiCs` keeps core C compilation behavior and extension-owned behavior in
separate lanes.

The core compiler remains responsible for normal C parsing, semantic analysis,
code generation, and diagnostics. Overlay lanes are opt-in and are intended for
additional metadata or stricter domain-specific checks that should not silently
change the base C compiler contract.

## Current Overlay Modes

CLI and frontend consumers can enable overlay behavior explicitly.

Supported overlay profiles:

- `none`
- `ide-metadata`
- `physics-units`
- `all`

CLI examples:

```bash
./fisics --overlay=physics-units sample.c
FISICS_OVERLAY=physics-units ./fisics sample.c
```

Frontend example:

```c
FisicsFrontendOptions opts = {
    .overlay_features = FISICS_OVERLAY_PHYSICS_UNITS,
};
```

If no overlay is enabled, the compiler stays on its normal core-C path and
overlay diagnostics/results are omitted.

## Physics Units Overlay

The first implemented overlay lane is a physics-units metadata/checking layer.
It uses declaration attributes:

```c
double distance [[fisics::dim(m)]];
double velocity [[fisics::dim(m/s)]];
double accel [[fisics::dim(acceleration)]];
```

This is an overlay annotation, not a replacement for the underlying C type.
`double`, `float`, structs, pointers, and other core C types still behave as
core compiler entities; the units overlay attaches extra semantic meaning beside
them.

### Dimension Model

The units lane uses a fixed 8-slot signed exponent vector:

1. length
2. mass
3. time
4. electric current
5. temperature
6. amount of substance
7. luminous intensity
8. custom/reserved domain slot

The implementation stores this as an `int8_t[8]` dimension vector.

### Supported Dimension Expressions

`fisics::dim(...)` currently accepts:

- base atoms: `m`, `kg`, `s`, `A`, `K`, `mol`, `cd`, `X`
- dimensionless aliases: `1`, `dimensionless`
- derived aliases:
  - `velocity`
  - `acceleration`
  - `force`
  - `energy`
  - `power`
  - `pressure`
  - `charge`
  - `voltage`
  - `resistance`
- expression operators:
  - `*`
  - `/`
  - integer exponents via `^`

Examples:

```c
[[fisics::dim(m)]]
[[fisics::dim(m/s)]]
[[fisics::dim(m/s^2)]]
[[fisics::dim(kg*m/s^2)]]
[[fisics::dim(energy)]]
```

## Current Semantic Guarantees

The current public overlay behavior is dimensions-only.

Implemented checks/inference:

- declaration and symbol attachments for resolved units annotations
- expression-side dimension inference for identifiers, literals, casts, unary
  preservation, assignment, arithmetic, comparisons, and ternary expressions
- assignment requires exact dimension equality
- `+` and `-` require exact dimension equality
- `*` adds dimension exponents
- `/` subtracts dimension exponents
- comparisons require exact dimension equality and produce a dimensionless
  result
- ternary branch values must have matching dimensions

When overlay violations occur, diagnostics are emitted in the extension
diagnostic namespace rather than core C diagnostic lanes.

## Frontend Contract

The reusable frontend library can export resolved declaration/symbol units
attachments through the optional `units_attachments` result lane.

Consumers must gate on:

- `FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS`

The contract details and compatibility policy live in:

- [compiler_ide_data_contract.md](./compiler_ide_data_contract.md)
- [frontend_api.md](./frontend_api.md)

## Current Limits

The current overlay is intentionally narrow:

- no concrete unit conversion system (`meter` vs `foot`, etc.)
- no unit literal syntax such as `10 m`
- no code generation changes based on units metadata
- no public expression-side units export lane yet

That keeps the overlay separated from the core compiler while the semantic model
hardens.
