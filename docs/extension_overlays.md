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
- `memory-check`
- `semantic-metadata`
- `all`

`all` currently means the established public non-instrumenting overlay bundle.
It includes the existing IDE/physics overlay behavior, but it does not include
`memory-check` or experimental `semantic-metadata`. Both require explicit
selection during this stage.

CLI examples:

```bash
./fisics --overlay=physics-units sample.c
FISICS_OVERLAY=physics-units ./fisics sample.c
./fisics --overlay=memory-check sample.c -o sample
./fisics --overlay=semantic-metadata --dump-sema -c sample.c -o sample.o
```

For source shared with strict Clang builds, include
`<fisics/extensions.h>` and use the `FISICS_*` portability macros. Direct
`[[fisics::...]]` syntax remains the canonical native form. See
[Portable Extension Authoring](extension_portability.md). The public example
corpus is checked with `make examples-extension-portability`.

Frontend example:

```c
FisicsFrontendOptions opts = {
    .overlay_features = FISICS_OVERLAY_PHYSICS_UNITS,
};
```

If no overlay is enabled, the compiler stays on its normal core-C path and
overlay diagnostics/results are omitted.

## Semantic Metadata Extension

A typed `semantic-metadata` overlay implements the first additive declaration
metadata milestone. It is opt-in and intentionally remains outside
`overlay=all`.

It reuses the existing `[[fisics::...]]` attribute pipeline to attach authored
identities, canonical domains, and roles to functions and single-declarator
file-scope variables. The compiler validates and merges compatible
redeclarations, rejects translation-unit ID collisions, and prints a stable
record projection with `--dump-sema`.

```c
[[fisics::id("thermal.read_temperature")]]
[[fisics::domain("thermal")]]
[[fisics::role("sensor.read")]]
double read_temperature(void);
```

declaration relationship model also accepts repeatable relationships to distinct canonical targets:

```c
[[fisics::id("thermal.read_temperature")]]
[[fisics::produces("signal.cabin.temperature")]]
double read_temperature(void);

[[fisics::consumes("signal.cabin.temperature")]]
void update_controller(void);
```

Same-owner duplicate edges are diagnosed. Edges merge across compatible
redeclarations. A target matching a valid authored ID in the active translation
unit is `resolved`; every other valid isolated target is retained as `external`.
When the source has no authored ID, the dump uses a deterministic compiler
projection rather than a pointer-derived identity. Target prefixes have no
inferred category or behavior.

The translation-unit metadata export machine export contract is implemented. A successful single-TU
invocation can request an atomic deterministic artifact with:

```bash
./fisics --overlay=semantic-metadata \
  --emit-semantic-metadata-json metadata.json \
  -c sample.c -o sample.o
```

The option requires exactly one C input, explicit overlay selection, and a
filesystem destination. The additive `FisicsAnalysisResultEx` frontend result
exposes the same sorted record/edge projection and advertises it with
`FISICS_CONTRACT_CAP_EXTENSION_SEMANTIC_METADATA` only when populated.
The focused proof command is `make integration-cli-semantic-metadata`; the
extended frontend lifecycle remains part of `make frontend-contract-test`.

The semantic project graph assembles multiple fresh translation-unit metadata export artifacts through a dedicated graph-only
manifest and paired terminal CLI mode:

```bash
./fisics --semantic-project-manifest semantic-project.json \
  --emit-semantic-project-graph-json semantic-project-graph.json
```

This mode validates current source provenance, enforces project-global authored
IDs, resolves cross-TU targets, and permits only explicitly allowlisted external
targets. It does not enable an overlay, parse or compile C, link, or execute
runtime behavior. Its focused proof command is
`make integration-semantic-project-graph`.

Physics-units remains the authority for dimensions and units. physical composition graph can emit a
fresh deterministic units-evidence sidecar for one translation unit:

```bash
./fisics --overlay=physics-units,semantic-metadata \
  --emit-semantic-metadata-json values.semantic.json \
  --emit-units-metadata-json values.units.json \
  -c values.c -o values.o
```

The implemented physical composition graph offline composer joins those receipts to a semantic project graph
and strict author-owned physical catalogs:

```bash
./fisics --physical-project-manifest physical-project.json \
  --emit-physical-project-graph-json physical-project-graph.json
```

The resulting extensible physical type keeps quantity kind, dimension,
concrete-unit policy, value form, reference semantics, and frame identity
separate. It reports only `source_compatible`, `source_unverified`,
`external_declared`, and uncovered state, with top-level
`physical_acceptance: not_assessed`.

The hardware compatibility graph consumes that graph with static component catalogs and authored topology:

```bash
./fisics --hardware-project-manifest hardware-project.json \
  --emit-hardware-project-graph-json hardware-project-graph.json
```

This exact paired terminal mode validates profile revisions, ownership,
capabilities, driver/model/transport compatibility, generic allocation spaces,
and physical-signal bindings. It does not compile C, generate or execute
drivers, contact hardware, run simulation, deploy firmware, or promote any
runtime or physical-acceptance state. The focused graph gates are
`make integration-physical-project-graph` and
`make integration-hardware-project-graph`.

See:

- [Extension Selection And Quickstart](extensions_guide.md)
- [Portable Extension Authoring](extension_portability.md)
- [Semantic Metadata Extension Contract](semantic_metadata_contract.md)
- [Semantic Project Graph Contract](semantic_metadata_project_graph_contract.md)
- [Physical Composition Graph Contract](semantic_metadata_physical_composition_contract.md)
- [Hardware Compatibility Graph Contract](semantic_metadata_hardware_contract.md)
- [Semantic Metadata Historical Implementation Record](semantic_metadata_implementation_plan.md)

## Memory-Check Overlay

`memory-check` is an opt-in runtime diagnostics overlay for allocation/free
behavior. It is meant for occasional diagnostic runs when maintainers want to
inspect whether a program has obvious allocation lifecycle problems without
changing default C semantics.

Enable it explicitly:

```bash
./fisics --overlay=memory-check app.c -o app
./app
```

The equivalent environment form follows the existing overlay path:

```bash
FISICS_OVERLAY=memory-check ./fisics app.c -o app
./app
```

When enabled, direct calls to the standard allocation family are rewritten to
the memory-check runtime wrappers:

| Source call | Lowered wrapper |
| --- | --- |
| `malloc(...)` | `__fisics_memcheck_malloc_site(...)` |
| `calloc(...)` | `__fisics_memcheck_calloc_site(...)` |
| `realloc(...)` | `__fisics_memcheck_realloc_site(...)` |
| `free(...)` | `__fisics_memcheck_free_site(...)` |

The older non-site wrapper names remain available for manual runtime tests and
external experiments, but compiler-generated memory-check calls use the
site-aware wrappers so diagnostics can include source labels.

For final executable builds, the driver auto-links the local runtime archive:

```text
build/unsanitized/libfisics_memcheck_runtime.a
```

Set `FISICS_MEMCHECK_RUNTIME_LIB=/path/to/libfisics_memcheck_runtime.a` only
when a custom runtime archive location is needed. Compile-only mode can emit
instrumented object code, but it does not auto-link the runtime.

### Reporting

The runtime automatically emits an exit report after the first memory-check
wrapper is used. If a program calls the report hook manually and no tracked
memory event happens afterward, the exit hook suppresses a duplicate summary.

```c
#include <stdlib.h>

void __fisics_memcheck_report(void);

int main(void) {
    void *p = malloc(32);
    free(p);
    __fisics_memcheck_report();
    return 0;
}
```

Example summary shape:

```text
[fisics:memory-check] summary: active=0 leaked_bytes=0 allocs=1 frees=1 double_free=0 unknown_free=0 tracker_failures=0
```

Leak reports include source labels for direct compiler-rewritten allocator
calls:

```text
[fisics:memory-check] leak: size=21 allocated_at=app.c:4
```

Report policy can be controlled with `FISICS_MEMCHECK_REPORT`:

| Value | Behavior |
| --- | --- |
| `always` / `auto` / unset | report at exit when tracked activity occurred |
| `errors` | report only double-free, unknown-pointer, or tracker failures |
| `leaks` | report leaks or errors |
| `never` / `manual` / `off` | suppress automatic exit reports |

Current diagnostics are written to `stderr` with the
`[fisics:memory-check]` prefix.

#### JSON report bridge

Memory-check can also write a default-off JSON sidecar report when
`FISICS_MEMCHECK_REPORT_JSON=/path/to/report.json` is set at runtime. This
sidecar uses the `memory_check_report_v1` profile and is a runtime report lane,
not a compiler diagnostic lane. It is not emitted by `--emit-diags-json`,
`--emit-diags-pack`, or the IDE analysis contract.

The report is written when the normal memory-check report hook emits a summary:
manual calls to `__fisics_memcheck_report()` use `"trigger": "manual"`, and
automatic exit reports use `"trigger": "atexit"`. Existing `stderr` output and
`FISICS_MEMCHECK_REPORT` policy behavior are unchanged.

Example shape:

```json
{
  "profile": "memory_check_report_v1",
  "schema_version": 1,
  "runtime": "fisics_memory_check",
  "trigger": "manual",
  "summary": {
    "active": 1,
    "leaked_bytes": 21,
    "allocs": 1,
    "frees": 0,
    "double_free": 0,
    "unknown_free": 0,
    "tracker_failures": 0
  },
  "leaks": [
    {
      "size": 21,
      "allocated_at": {
        "file": "app.c",
        "line": 4
      }
    }
  ]
}
```

### Current Checks

The current implementation detects or reports:

- successful `malloc`, `calloc`, and `realloc` allocations
- successful `free` releases
- `free(NULL)` as a normal no-op
- double free at free time
- unknown-pointer free at free time
- automatic exit summaries after tracked activity
- manual summaries through `__fisics_memcheck_report()`
- duplicate exit-report suppression after an unchanged manual report
- report policy control through `FISICS_MEMCHECK_REPORT`
- optional JSON sidecar reports through `FISICS_MEMCHECK_REPORT_JSON`
- allocation/free source labels for direct compiler-rewritten calls
- live allocation count and leaked-byte summary
- deterministic successful `realloc` accounting

### Current Limits

Memory-check is not an AddressSanitizer replacement.

Current limits:

- no default-mode behavior change
- not included in `--overlay=all`
- no general use-after-free detection for ordinary reads/writes after `free`
- no redzones or heap poisoning
- no bounds checking for `memcpy`, `memmove`, `strcpy`, or similar functions
- no stack allocation or `alloca` tracking
- no custom allocator tracking
- no allocator calls through function pointers
- no thread-safety guarantee for the runtime tracker
- no frontend/IDE exported memory diagnostics lane yet

The useful first interpretation is "allocation/free lifecycle diagnostics for
direct standard allocator calls." Stronger runtime safety work should be added
as later opt-in behavior, not by changing default C compilation.

### Validation Target

Maintainers touching this overlay should run:

```bash
make memory-check-test
```

That structured lane runs overlay parsing, runtime skeleton, direct-call
rewrite, driver auto-link, behavioral canaries, and the focused final manifest.
The canaries exercise the public `--overlay=memory-check` driver path and check
clean allocation/free, `free(NULL)`, leak summaries, automatic exit reports,
report policy controls, source labels, double-free diagnostics,
unknown-pointer free diagnostics, and successful `realloc` accounting.
`final-memory-check` remains available as the focused bucket-14 final manifest
target. Default examples remain separate:

```bash
make examples
```

To see the report shape in a small intentional-leak demo:

```bash
make examples-memory-check
```

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

Canonical unit names in this overlay use singular lowercase `snake_case`
spellings such as `meter`, `kilowatt_hour`, and `pound_force`. Source aliases
such as `meters`, `metres`, or `kilowatts` are accepted where the registry
allows them, but canonical names are the preferred emitted and documented form.

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

- length: `meter`, `millimeter`, `centimeter`, `kilometer`, `foot`, `inch`, `yard`, `mile`
- mass: `kilogram`, `gram`, `milligram`, `metric_ton`, `pound`
- time: `second`, `millisecond`, `minute`, `hour`
- velocity: `meter_per_second`, `foot_per_second`
- acceleration: `meter_per_second_squared`
- force: `newton`, `kilonewton`, `pound_force`
- energy: `joule`, `kilojoule`, `watt_hour`, `kilowatt_hour`
- power: `watt`, `milliwatt`, `kilowatt`, `megawatt`
- pressure: `pascal`, `kilopascal`, `bar`, `psi`
- charge: `coulomb`, `ampere_hour`, `milliampere_hour`
- voltage: `volt`, `millivolt`, `kilovolt`
- resistance: `ohm`, `milliohm`, `kiloohm`, `megaohm`

The current registry is deliberately family-grouped and canonical-base driven:

- each family has one canonical base unit
- explicit conversions are computed through that canonical base
- source aliases are accepted for authoring, but canonical names remain the
  stable reference form for docs, dumps, and exported metadata

## Current Semantic Guarantees

The current public overlay behavior is still dimension-first, with one narrow
explicit conversion helper lane.

Implemented checks/inference:

- declaration and symbol attachments for resolved units annotations
- declaration-side concrete unit validation against the seeded unit registry
- expression-side dimension inference for identifiers, literals, casts, unary
  preservation, assignment, arithmetic, comparisons, and ternary expressions
- function arguments with resolved dimensions must match their known parameter
  contracts, for direct calls and supported typed indirect calls, even when the
  contract has no concrete unit; mismatches use diagnostic `4112`
- numeric literal-only arguments may take their interpretation from the declared
  parameter, following the initializer literal policy; known dimensionless values
  derived from variables are checked normally
- unannotated parameters, variadic arguments beyond the fixed signature, and
  unknown argument metadata remain unchecked; acceptance alone does not prove
  physical compatibility at those boundaries
- assignment requires exact dimension equality
- declaration initializers with resolved expression dimensions require the same
  dimension as their annotated owner, even when no concrete unit is specified;
  the same check applies to annotated record fields and array initializer entries
- numeric literal-only initializers (including signed literals, casts and
  arithmetic on literals) may take their physical interpretation from the owner;
  this exception does not apply to identifiers, calls, or dimensionless results
  derived from physical values
- unknown initializer metadata remains unchecked; acceptance alone does not prove
  physical compatibility across an unannotated or unsupported boundary
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

## Public Pilot Example

The first public end-to-end overlay example lives here:

- [../examples/physics_units/README.md](../examples/physics_units/README.md)
- [../examples/physics_units/helper_patterns.md](../examples/physics_units/helper_patterns.md)

That example lane is the recommended starting point for:

- human users learning the overlay surface
- coding agents generating overlay-aware C code
- maintainers verifying the intended compile / `--dump-sema` / diagnostic flows

## Physical square roots

The physics-units overlay derives result metadata for real-valued direct
`sqrt`, `sqrtf`, and `sqrtl` calls resolved to system-origin declarations
with `math.h` included, including declarations in nested system headers. The compiler-owned real `tgmath.h` square-root route is
also recognized. Identification happens while the actual lexical callee is live;
a matching identifier alone is insufficient. A local pointer, user definition,
or unrelated declaration does not acquire the intrinsic rule.

For a resolved argument dimension, every one of the eight signed exponents must
be even. The result halves each exponent exactly, including negative exponents.
For example, `sqrt(speed*speed)` has dimension speed and `sqrt(area)` has dimension
length. Dimensionless arguments produce dimensionless results. Odd exponents are
rejected with `4115`, `extension.units.sqrt_unsupported`; fractional exponents are
not introduced and Dim8 remains unchanged. The diagnostic carries context `sqrt`
and the input dimension vector/text.

Known concrete units use these exact seeded mappings:

| Input units | Result units |
| --- | --- |
| square_meter, square_millimeter, square_centimeter | meter, millimeter, centimeter respectively |
| meter_to_fourth, millimeter_to_fourth, centimeter_to_fourth | square_meter, square_millimeter, square_centimeter respectively |

The mapping describes the numerical square root's representation. It does not
rescale the input or choose a canonical unit merely from the result dimension.
A known unit without a supported mapping is rejected. An input with known
dimensions but unknown concrete units produces a dimension-only result. Existing
arithmetic can lose concrete-unit information before the call; square-root
inference does not recover that information. Area-unit conversion support is
unchanged and remains separate from these unit-root mappings.

The result feeds normal initializer, assignment, argument and return checks.
Literal-only square-root calls remain calculated results, not contextual numeric
initializers. Unknown input metadata stays unknown. Indirect library pointers,
custom wrappers without declared result contracts, and complex math are outside
this checkpoint's intrinsic recognition. Existing declared wrapper contracts still
work. These checks do not prove value-domain safety (for example nonnegative real
inputs), numerical accuracy, pointer provenance or whole-model physical validity.
C numerical execution, ABI and precision/lowering policies are unchanged.

Use `make integration-units-sqrt-contracts` for the focused CLI gate.

## Physical magnitudes, norms and maxima

Real `fabs`, `hypot`, and `fmax` calls (including their `f`/`l` variants) derive
physical metadata when resolved to standard system math declarations. The
compiler-owned real `tgmath.h` routes and seeded `__builtin_fabs`,
`__builtin_fabsf`, `__builtin_fabsl` routes are recognized too. Lexical identity
and real arithmetic arguments are required; user declarations/definitions,
shadowed pointers, indirect calls and complex absolute values do not acquire
these rules.

- `fabs(x)` preserves the known dimension and optional concrete unit of `x`.
- `hypot(x,y)` and `fmax(x,y)` require matching known dimensions. A conflict
  reports `4116`, `extension.units.math_argument_dim_mismatch`, with operation
  context and both dimension vectors. This includes a bare dimensionless zero
  combined with a dimensioned value: declare a matching typed zero instead.
- Two known, different concrete units report `4107`. Convert explicitly before
  calling the helper. A binary result retains a unit only when both inputs
  have the same resolved concrete unit. If only dimensions are known, the
  result has dimensions only; no canonical unit is invented.
- If either binary input dimension is unknown, the result remains unknown.
  Known unary inputs propagate normally; unknown unary inputs stay unknown.
  Literal-only calls have dimensionless results, without contextual dimension
  adoption from their destination.

```c
#include <math.h>
#include <fisics/extensions.h>
double dx FISICS_DIM(length) FISICS_UNIT(meter) = 3;
double dy FISICS_DIM(length) FISICS_UNIT(meter) = 4;
double zero FISICS_DIM(length) FISICS_UNIT(meter) = 0;
double norm FISICS_DIM(length) FISICS_UNIT(meter) = hypot(dx, dy);
double bounded FISICS_DIM(length) FISICS_UNIT(meter) = fmax(fabs(dx), zero);
```

Results participate in initializer, assignment, argument, return and explicit
conversion checks. The permanent focused gate is
`make integration-units-math-contracts`; its runtime controls compare reduced
solver expressions against analytical values and ordinary Clang execution.
These rules leave C execution, ABI, numeric precision, NaN and domain behavior
unchanged. They do not infer physics through arbitrary wrappers or prove solver
accuracy. Other math functions remain outside this bounded rule set.

## Annotated member, array and pointer access

Resolved dimensions now propagate through C's actual member, element and
explicitly typed dereference expressions. This includes `state.field`,
`body->acceleration[i]`, nested records, arrays of records, multiple subscripts,
record-pointer aliases, and typed scalar pointers. Dimensions do not require a
concrete unit annotation. Optional concrete units retain the explicit-conversion
policy for same-dimension values.

For example:

```c
#include <fisics/extensions.h>
typedef struct {
    double acceleration[2] FISICS_DIM(acceleration);
} Body;

void set_acceleration(Body *body, double value FISICS_DIM(acceleration)) {
    body->acceleration[0] = value;
}
```

Reads and writes use the same captured type facts. An incompatible value is
rejected on assignment (`4103`) or compound addition/subtraction (`4101`/`4102`).
A read also reaches initializer, arithmetic, argument and declared return checks.
The resolved C type is captured while its lexical scope is live, so two unrelated
fields with the same name cannot borrow each other's annotations. Missing field
annotations remain unknown, including when another annotated field shares that name.

These are static declared-type checks. A cast to an annotated record type asserts
that interface; the compiler does not prove the pointer's runtime target, bounds,
active union member or physical interpretation. A record-pointer alias retains
its declared field contracts. General scalar-pointer dataflow and storage-unit
inference through an unannotated alias are not guaranteed. Existing narrow legacy
address/alias assignment checks are not a complete alias analysis.

An annotated array is homogeneous: its element dimension applies to all indices.
The compiler does not infer per-slot dimensions for an unannotated heterogeneous
matrix. Runtime values, layout and C ABI remain unchanged. Ordinary C and the
opt-in overlay remain independent.

Use `make integration-units-access-contracts` for the focused driver checks.

## Declared function results

Use the existing dimension/unit attributes on a function's return type in its
shared header. For example:

```c
#include <fisics/extensions.h>

double FISICS_DIM(length) FISICS_UNIT(meter)
travel(double speed FISICS_DIM(speed), double dt FISICS_DIM(time));
```

A caller uses that declared result even when the definition is in another
translation unit or appears later. The definition should include the header;
it may repeat the result annotations or inherit omitted annotations from the
preceding declaration. Suffix attributes after the parameter list are also
accepted for direct function declarations/definitions. An omitted annotation
cannot erase an earlier result contract in the same scope.

With `--overlay=physics-units`, each resolved return expression must match the
effective declared result dimension (`4113`, `extension.units.return_dim_mismatch`).
Known same-dimension concrete-unit differences require explicit conversion
(`4107`). Conflicting known result dimensions on same-scope redeclarations report
`4114`, `extension.units.return_contract_mismatch`. The multi-TU driver also checks
known external result-contract contradictions in either input order, using the
existing cross-TU type-conflict diagnostic (`2000`). Its normal link path (`-o`)
must analyze all translation units; a frontend-only inspection is not that gate.

Typed callbacks preserve declared result metadata through their C signature.
Lexical binding controls legacy body inference, so a shadowing local callable
cannot acquire a global function's inferred result. Numeric literal-only returns
retain contextual interpretation, without changing their expression metadata.

A declaration is a trusted interface, not proof of an external implementation.
Unknown return expressions remain unchecked. A TU that defines a function without
including its annotated header cannot retroactively have its body checked from
another TU. Unannotated declarations assert no physical dimension; known contracts
are compared, but missing contracts do not become contradictions. The existing
limited same-TU body inference remains available for unannotated functions.
General alias dataflow, heterogeneous-slot contracts and checking-coverage
reporting are separate work. Dim8, numerical storage, C ABI and runtime execution are unchanged.

Run `make integration-units-return-contracts` for the focused driver checks.

## Current Limits

The current overlay is intentionally narrow:

- no implicit conversion between concrete units
- no English concrete-unit words in `dim(...)`; those names belong in
  `[[fisics::unit(...)]]` instead
- no unit literal syntax such as `10 m`
- no public conversion scale/offset metadata
- no offset-bearing public unit semantics yet; the current registry keeps
  offsets dormant at `0.0`
- no general result-unit propagation policy for mixed-unit arithmetic yet
- no code generation changes based on units metadata
- no public expression-side concrete-unit export lane yet
- no public expression-side units export lane yet

That keeps the overlay separated from the core compiler while the semantic model
hardens.

## Mixed local frame quantities

A heterogeneous physical matrix can be authored as homogeneous annotated blocks
using the existing C record/array surface. The stable compiler fixture
[`frame_local.h`](../tests/fixtures/solver_physics/frame_local.h) demonstrates a
complete unreleased 2D frame element with separate translation/rotation vectors,
force/moment outputs and four stiffness blocks. Translation-to-force entries
have force/length dimensions; the two coupling blocks have force dimensions;
rotation-to-moment entries have force*length dimensions. Rotations use scalar
dimensions in this lane; no angle axis or new quantity-kind feature is introduced.

Construction and matrix-vector application use existing initializer, field,
array, assignment and arithmetic enforcement, including dynamic indices inside
homogeneous blocks. There is no compiler matrix-layout inference or new syntax.
The fixture's numerical inputs use an explicit SI convention. Known concrete
units on section/translation fields still participate in unit checks; dimensional
products do not establish a concrete-unit scale for every derived coefficient.

`make integration-units-frame-local-contracts` checks full coefficients and
applications against Clang and an independent rational integration oracle, exact
physical failures, a numerical sign mutation, and an explicit raw-matrix gap.
See the [fixture contract](../tests/fixtures/solver_physics/README.md#typed-unreleased-local-frame-boundary)
for mapping, counts, tolerances and scope. This does not validate arbitrary raw
matrix slots, distinguish same-dimension coupling roles, check general dense
factorization, or migrate the structural solver. Those remain separate work.

## Typed releases, transforms and explicit dense boundaries

The stable [`frame_boundary.h`](../tests/fixtures/solver_physics/frame_boundary.h)
fixture extends the local block authoring pattern with rotational releases,
local-to-global stiffness congruence, coordinate/load basis transforms and
explicit dense exchange. The operations reuse existing dimension enforcement;
there is no new compiler matrix syntax or frame-ID inference. Their preconditions
are admitted finite SI inputs, positive release pivots and normalized finite bases.

The row-major packer preserves the original six-coordinate order. The deliberately
named `frame_dense_restore_assumed_si` is a trust boundary: it assigns declared
physical fields from raw values under a caller-supplied SI/order assumption.
It does not recover or prove missing raw metadata. Known physical misuses after
restoration remain rejected, while raw slot swaps require separate numerical
validation. General pointer aliases and bulk copies gain no new inference rule.

`make integration-units-frame-boundary-contracts` qualifies 60 release/basis/section
combinations, exact physical diagnostics, covariance, in-place operation,
independent pack/import checks and numerical mutation controls. See the
[fixture contract](../tests/fixtures/solver_physics/README.md#local-releases-basis-transforms-and-dense-exchange)
for oracle, scope and tolerances. The shared structural solver is not migrated by
these fixtures; the original raw-matrix checking gap remains visible.

## Typed local load and recovery authoring

The stable [frame fixture contract](../tests/fixtures/solver_physics/README.md#typed-local-loads-joint-springs-recovery-and-work)
now demonstrates typed uniform/nodal load terms, translational/rotational elastic
springs, released member rotations, end-force and interior stress recovery, and
local work/energy identities. `frame_loading.h`, `frame_joint.h` and
`frame_recovery.h` reuse existing field/array, argument, return, arithmetic and
math-helper enforcement. No new compiler syntax, physical kinds or runtime
solver policy is introduced.

Keep polynomial coordinates dimensionless and each coefficient array homogeneous.
The fixture keeps separate declarations for readability. Independently annotated
comma-separated declarators are also supported (see below). Explicitly identify the coordinate basis used for each work pairing. Dense restoration and
unknown physical inputs retain their existing trust/coverage limitations.

The permanent gate `make integration-units-frame-recovery-contracts` checks typed
failures and independent numerical results, including stress stationary points,
released-load terms, joint masks and energy accounting. It preserves the original
stress oracle and adds a cantilever anchor. This is compiler-owned fixture
qualification, not shared structural-solver adoption, whole-model physical
acceptance, strength certification or global conservation qualification.


## Independent annotations in a declaration list

Physical `dim` and `unit` suffix attributes belong to the declarator that spells
them. Each variable has an independent compiler-side attachment, including in
local/global declarations, `for` initializers, fields, arrays and supported pointer
access. For example:

```c
double force FISICS_DIM(force) = 120,
       moment FISICS_DIM(force*length) = 200;
double copied_moment FISICS_DIM(force*length) = moment; /* accepted */
force = moment; /* 4103: incompatible dimensions */
```

An unannotated neighbor remains unknown. Different concrete units stay distinct:
`meter` and `foot` values still require explicit conversion. Shared attributes in
the declaration specifiers apply to every declarator; repeating a dimension/unit
on the same effective declarator is a duplicate, not an override. Duplicate or
invalid attributes on a later variable are diagnosed independently. Metadata JSON
exports the corresponding variable's own dimension, unit and source range.

`make integration-units-declarator-contracts` checks exact diagnostics, rejected
artifact absence, ordinary-C legality, runtime parity and metadata attachments.
This does not add physical quantity kinds, heterogeneous raw-slot inference,
general pointer dataflow or strict coverage enforcement.


## Internal coverage assessment contract

The [units coverage contract](units_coverage_contract.md) defines fact origins,
source provenance, checking/trust states, explicit information-loss reasons and a
finite selected-obligation denominator. `make units-coverage-contract-test`
qualifies the internal model against real frontend tables and solver fixtures.
This stage adds no CLI coverage report or strict enforcement; resolved dimensions
alone do not establish complete checking coverage.


## Selected-function coverage reporting

Use `--units-coverage-function <name>` with `--emit-units-coverage-json <path>`
and/or `--emit-units-coverage-text <path>` for an opt-in, frontend-only report.
The [reporting contract](units_coverage_reporting.md) defines scope selection,
checked/trusted/unknown states, source and operand provenance, finite counts,
unsupported enumeration and atomic output behavior. Unknown paths remain visible
without changing ordinary compilation acceptance. Run
`make integration-units-coverage-contracts` for the permanent report gate.

## Opt-in strict coverage

Add `--units-coverage-strict` to the selected-function report invocation to enforce
[policy selected_function_v1](units_coverage_strict.md). Required unknowns,
contradictions, incomplete enumeration and unpermitted trust fail the gate.
Ordinary loop controls and permitted trust retain separate visible counts.
`make integration-units-coverage-strict-contracts` qualifies this policy; default
compilation and report-only behavior are unchanged.

### Literal storage writes and real-C strict boundaries

Plain literal-only `=` writes adopt the declared lvalue representation, like
literal initialization. Variable/call values and compound assignments retain
physical compatibility checks. Strict coverage records the literal interpretation
as explicit assumption trust. The [strict-policy contract](units_coverage_strict.md)
also defines narrow pointer predicates, boolean status returns and source-defined
void/physical-record-pointer interface trust; those are not checked physical values.
Use `make integration-units-storage-contracts` for the focused contracts.


## Physical quantity kinds and references (P5.3/P5.4)

`--overlay=physics-semantics --physical-semantics-policy <path>` adds bounded
physical-kind compatibility to one direct C translation unit and implies
`physics-units`. It is deliberately absent from default/all selections. Portable
`FISICS_QUANTITY_KIND` and `FISICS_PHYSICAL_ORIGIN` spellings erase for Clang.
See [physical semantics](physical_semantics.md) for explicit policy loading,
admitted carriers, explicit point/difference arithmetic, known contradictions,
unknown/trust controls and the separate [physical strict/receipt contract](physical_coverage.md).
Multi-source physical compilation remains refused; exported receipt comparison is
a dedicated terminal command.
