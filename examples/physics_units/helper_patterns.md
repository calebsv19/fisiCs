# Physics Units Helper Patterns

This note captures stable authoring patterns that work with the current
`fisiCs` physics-units overlay.

It is intentionally conservative:

- no syntax sugar beyond the current `dim(...)` / `unit(...)` attributes
- no custom macros required
- no implicit unit conversion assumptions
- no dependency on unimplemented runtime libraries

The goal is to give humans and coding agents a repeatable way to write
overlay-aware code that is easy to inspect with `--dump-sema`.

## Pattern 1: Declare Physical Constants Explicitly

Keep physical constants as normal C variables with explicit dimension and unit
attachments.

```c
static const double gravity
    [[fisics::dim(acceleration)]]
    [[fisics::unit(meter_per_second_squared)]] = -9.81;

static const double dt
    [[fisics::dim(time)]]
    [[fisics::unit(second)]] = 0.016;

static const double half
    [[fisics::dim(scalar)]] = 0.5;
```

Why this works well:

- the compiler can validate the declaration immediately
- the semantic dump shows both dimension and concrete unit identity
- agents do not need hidden tables or comments to know what the value means

## Pattern 2: Keep One Dimensional Step Per Statement

Prefer small formula stages instead of collapsing every physical update into one
long expression.

```c
double velocity_delta
    [[fisics::dim(speed)]]
    [[fisics::unit(meter_per_second)]] = acceleration * dt;

double velocity_next
    [[fisics::dim(speed)]]
    [[fisics::unit(meter_per_second)]] = velocity + velocity_delta;

double position_delta
    [[fisics::dim(length)]]
    [[fisics::unit(meter)]] = velocity_next * dt;

double position_next
    [[fisics::dim(length)]]
    [[fisics::unit(meter)]] = position + position_delta;
```

Why this works well:

- sema dumps stay readable
- failures point at a narrow formula step
- agents can reuse the same shape across many updates

## Pattern 3: Keep Formula Names Literal

Prefer direct names like:

- `position`
- `velocity`
- `acceleration`
- `gravity`
- `dt`
- `mass`
- `kinetic_energy`
- `velocity_delta`
- `position_next`

Avoid overly compressed names in the first pass of a physics-heavy function.

This keeps both human review and semantic dump inspection easier.

## Pattern 4: Use Typed Intermediates For Common Formulas

Common physics formulas should usually be written as a small typed chain rather
than as one opaque expression.

Kinetic energy:

```c
double velocity_squared
    [[fisics::dim(m^2/s^2)]] = velocity * velocity;

double kinetic_energy
    [[fisics::dim(energy)]]
    [[fisics::unit(joule)]] = half * mass * velocity_squared;
```

This pattern is useful because:

- the intermediate dimension is visible and reviewable
- the final energy assignment stays easy to read
- a future mismatch tends to fail at the exact combine step

## Pattern 5: Convert Units Only At Explicit Boundaries

Do not rely on same-dimension concrete-unit mixing to work implicitly. Use the
explicit helper at the boundary where unit representation actually changes.

```c
double launch_speed_fps
    [[fisics::dim(speed)]]
    [[fisics::unit(foot_per_second)]] = 60.0;

double launch_speed_mps
    [[fisics::dim(speed)]]
    [[fisics::unit(meter_per_second)]] =
        fisics_convert_unit(launch_speed_fps, "meter_per_second");
```

Good boundary choices:

- program input normalization
- file/data import
- UI display export
- interop with a subsystem that expects a different concrete unit

## Pattern 6: Inspect Failures With `--dump-sema`

When a formula fails, prefer:

```bash
./fisics --overlay=physics-units --dump-sema -c your_file.c -o /tmp/your_file.o
```

Look for:

- resolved declaration annotations
- the last legal intermediate expression
- `SemanticModel: N error(s)`
- skipped LLVM code generation after semantic failure

## Recommended First Style For Agents

If an agent is generating new overlay-aware code, the safest default style is:

1. declare every physical quantity with both `dim(...)` and `unit(...)`
2. keep constants explicit
3. use one physical step per statement
4. name intermediates literally
5. use `fisics_convert_unit(...)` only at explicit representation boundaries

That style is slightly verbose, but it is the most debuggable current surface.
