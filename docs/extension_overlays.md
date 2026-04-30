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
double duration [[fisics::dim(duration)]];
double kinetic [[fisics::dim(work)]];
double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
double dt_s [[fisics::dim(time)]] [[fisics::unit(second)]];
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
- dimensionless aliases: `1`, `dimensionless`, `scalar`
- readable base-family aliases:
  - `length`, `distance`, `displacement`
  - `mass`
  - `time`, `duration`
  - `current`, `electric_current`
  - `temperature`
  - `amount`, `amount_of_substance`
  - `luminous`, `luminous_intensity`
  - `custom`, `custom_dimension`
- derived aliases:
  - `velocity`
  - `acceleration`
  - `force`
  - `energy`
  - `work`
  - `kinetic_energy`
  - `potential_energy`
  - `power`
  - `pressure`
  - `charge`
  - `voltage`
  - `electric_potential`
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
[[fisics::dim(distance/time)]]
[[fisics::dim(work)]]
```

Aliases are normalized to canonical dimension text in debug and export output.
For example, `[[fisics::dim(length)]]` and `[[fisics::dim(distance)]]` both
resolve to the same canonical length dimension as `[[fisics::dim(m)]]`.

### Supported Unit Attachments

`fisics::unit(...)` is now available as a declaration-side attachment on top of
`fisics::dim(...)`.

Current rules:

- `[[fisics::unit(...)]]` requires a matching `[[fisics::dim(...)]]`
- the unit name must exist in the seeded concrete-unit registry
- the unit's canonical dimension must match the resolved `dim(...)` exactly
- duplicate `unit(...)` annotations on the same declaration are rejected
- unit identity is validation-only for semantics in the current slice; it does
  not yet change arithmetic or conversion behavior
- resolved declaration/symbol unit identity is now exported as optional frontend
  metadata for IDE/agent consumers
- the public export preserves both canonical unit identity and original source
  spelling, while canonical scale/offset remain internal for Phase 8 conversion
  work

### Explicit Conversion Helper

The first concrete-unit conversion lane is now explicit-only.

Use:

- `fisics_convert_unit(value, "target_unit")`
- `__builtin_fisics_convert_unit(value, "target_unit")`

Current helper rules:

- the source expression must already carry a resolved concrete unit
- the target unit must resolve through the seeded concrete-unit registry
- source and target must share the same canonical dimension and family
- only conversion-supported families are accepted in the first slice
- the current helper lane is intentionally floating-scalar only
- unknown target unit text is rejected separately from known-but-incompatible
  targets such as `meter -> second`

Example:

```c
double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]] = 10.0;
double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]] =
    fisics_convert_unit(distance_m, "foot");
```

Seeded unit families currently include examples such as:

- length: `meter`, `foot`, `inch`
- mass: `kilogram`, `gram`
- time: `second`, `minute`, `hour`
- velocity: `meter_per_second`, `foot_per_second`
- acceleration: `meter_per_second_squared`
- force: `newton`
- energy: `joule`
- power: `watt`
- pressure: `pascal`
- charge: `coulomb`
- voltage: `volt`
- resistance: `ohm`

## Current Semantic Guarantees

The current public overlay behavior is still dimension-first, with one narrow
explicit conversion helper lane.

Implemented checks/inference:

- declaration and symbol attachments for resolved units annotations
- declaration-side concrete unit validation against the seeded unit registry
- expression-side dimension inference for identifiers, literals, casts, unary
  preservation, assignment, arithmetic, comparisons, and ternary expressions
- assignment requires exact dimension equality
- same-dimension assignment with different concrete units requires an explicit
  conversion helper
- `+` and `-` require exact dimension equality
- same-dimension `+` and `-` with different concrete units require an explicit
  conversion helper
- `*` adds dimension exponents
- `/` subtracts dimension exponents
- comparisons require exact dimension equality and produce a dimensionless
  result
- same-dimension comparisons with different concrete units require an explicit
  conversion helper
- ternary branch values must have matching dimensions
- same-dimension ternary branches with different concrete units require an
  explicit conversion helper
- direct declaration-owned dimensionless literals can still adopt the owning
  declaration dimension and concrete unit before these checks apply
- explicit helper conversions are currently limited to resolved floating-scalar
  source expressions in supported same-family conversion lanes

When overlay violations occur, diagnostics are emitted in the extension
diagnostic namespace rather than core C diagnostic lanes.

## Frontend Contract

The reusable frontend library can export resolved declaration/symbol units
attachments through the optional `units_attachments` result lane.

Consumers must gate on:

- `FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS`
- `FISICS_CONTRACT_CAP_EXTENSION_UNITS_CONCRETE`

The contract details and compatibility policy live in:

- [compiler_ide_data_contract.md](./compiler_ide_data_contract.md)
- [frontend_api.md](./frontend_api.md)

## Current Limits

The current overlay is intentionally narrow:

- no implicit conversion between concrete units
- no English concrete-unit words in `dim(...)`; those names belong in
  `[[fisics::unit(...)]]` instead
- no unit literal syntax such as `10 m`
- no public conversion scale/offset metadata
- no general result-unit propagation policy for mixed-unit arithmetic yet
- no code generation changes based on units metadata
- no public expression-side concrete-unit export lane yet
- no public expression-side units export lane yet

That keeps the overlay separated from the core compiler while the semantic model
hardens.
