# Physics Units Pilot

This example lane is the first public validation customer for the `fisiCs`
physics-units overlay.

It is intentionally small and deterministic:

- one valid ballistics example
- four invalid semantic examples
- no rendering, SDL, or external runtime framework

## Files

- `ballistics_valid.c`: legal timestep-based update using dimensions and units
- `ballistics_invalid_assignment.c`: invalid assignment (`length = time`)
- `ballistics_invalid_compare.c`: invalid comparison (`length < time`)
- `ballistics_invalid_position_update.c`: invalid motion update
  (`position += acceleration`)
- `ballistics_invalid_formula_chain.c`: invalid nested formula combine
  (`energy = mass + velocity * dt`)

## What The Valid Example Proves

The valid example demonstrates these legal formula paths:

- `velocity = velocity + acceleration * dt`
- `position = position + velocity * dt`
- `kinetic_energy = 0.5 * mass * velocity * velocity`

The declarations use readable physics-oriented overlay annotations:

- `position`: `length`, `meter`
- `velocity`: `speed`, `meter_per_second`
- `acceleration`: `acceleration`, `meter_per_second_squared`
- `dt`: `time`, `second`
- `gravity`: `acceleration`, `meter_per_second_squared`
- `mass`: `mass`, `kilogram`
- `kinetic_energy`: `energy`, `joule`

## Build The Valid Example

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
make examples-physics-units
./build/examples/ballistics_valid
```

## Inspect Semantic Units Data

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics --overlay=physics-units --dump-sema -c examples/physics_units/ballistics_valid.c -o build/examples/ballistics_valid.o
```

Useful things to look for in the dump:

- declaration/symbol unit attachments
- inferred expression dimensions for `acceleration * dt`
- inferred expression dimensions for `velocity * velocity`
- the final `energy`-dimension assignment
- unit identity preserved beside dimensions on the resolved declarations

## Inspect Intentional Failures

Invalid assignment:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics --overlay=physics-units --dump-sema -c examples/physics_units/ballistics_invalid_assignment.c -o /tmp/ballistics_invalid_assignment.o
```

Expected outcome:

- compile failure
- extension diagnostic describing a units assignment mismatch
- semantic dump showing `SemanticModel: 1 error(s)`
- LLVM code generation skipped because semantic errors were recorded

Invalid comparison:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics --overlay=physics-units --dump-sema -c examples/physics_units/ballistics_invalid_compare.c -o /tmp/ballistics_invalid_compare.o
```

Expected outcome:

- compile failure
- extension diagnostic describing a units comparison mismatch
- semantic dump showing `SemanticModel: 1 error(s)`
- LLVM code generation skipped because semantic errors were recorded

Invalid position update:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics --overlay=physics-units --dump-sema -c examples/physics_units/ballistics_invalid_position_update.c -o /tmp/ballistics_invalid_position_update.o
```

Expected outcome:

- compile failure
- extension diagnostic describing a units addition mismatch
- semantic dump showing the resolved `position` and `acceleration` dimensions
- LLVM code generation skipped because semantic errors were recorded

Invalid nested formula chain:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics --overlay=physics-units --dump-sema -c examples/physics_units/ballistics_invalid_formula_chain.c -o /tmp/ballistics_invalid_formula_chain.o
```

Expected outcome:

- compile failure
- extension diagnostic describing a units addition mismatch
- semantic dump still showing the legal intermediate `velocity * dt` inference
- LLVM code generation skipped because semantic errors were recorded

## Notes

- This pilot is dimension-first. It uses concrete units, but it does not depend
  on conversion helpers.
- The example is also intended to be a stable prompt/reference lane for human
  users and coding agents authoring overlay-aware C code.
- For stable current authoring guidance beyond this single pilot, see
  [helper_patterns.md](./helper_patterns.md).
