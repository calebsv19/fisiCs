# Physical authoring and verification path

Use this path for ordinary C with opt-in unit and physical checks. Start with
[extension selection](extensions_guide.md); detailed contracts linked below own
syntax and limits. A compiler check establishes source compatibility. Runtime
energy accounting, numerical convergence and physical acceptance need separate tests.

## 1. Establish dimensions and representation units

Include `<fisics/extensions.h>` in source intentionally shared with Clang:

```c
#include <fisics/extensions.h>
double distance FISICS_DIM(length) FISICS_UNIT(meter) = 2.0;
```

`dim` describes the physical dimension; `unit` describes concrete representation.
Use explicit `fisics_convert_unit(value, "meter")` at conversion boundaries.
There is no implicit mixing of different concrete units. Offset Celsius/Fahrenheit
conversion is not established by absolute/difference temperature arithmetic.
The conversion builtin itself is fisiCs-specific; annotation erasure does not
provide a Clang runtime conversion implementation. Shared conversion code needs
an explicitly tested ordinary-C adapter.

From the repository root, choose the source-built compiler and fresh output space:

```sh
COMPILER=/absolute/path/to/selected/fisics
OUT=$(mktemp -d)
"$COMPILER" --version
"$COMPILER" --overlay=physics-units -Iinclude \
  examples/physics_units/ballistics_valid.c -o "$OUT/ballistics"
"$OUT/ballistics"
clang -std=c99 -Wall -Wextra -Werror -pedantic -Iinclude \
  examples/physics_units/ballistics_valid.c -o "$OUT/ballistics-clang"
"$OUT/ballistics-clang"
```

The public [units examples](../examples/physics_units/README.md) explain intentional
failures. Clang erases these macros and checks ordinary C only. The header does
not enable any overlay. Keep output until reviewed; remove only your own output
when no consumer needs it. Avoid make defaults when protecting existing binaries.

## 2. Add meaning only where dimensions are insufficient

Energy and torque share dimensions; position and displacement need different
reference rules. Add explicit quantity kinds and references using
[physical semantics](physical_semantics.md) and the
[policy model](physical_policy_model.md). For example:

```c
double energy FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy");
```

A kind does not replace `dim`. Names do not infer kinds, frames or physical truth.
`physics-semantics` implies units checking but requires an explicit compatible
policy. Use one direct C source per invocation:

```sh
"$COMPILER" --overlay=physics-semantics \
  --physical-semantics-policy tests/fixtures/physical_semantics/policy.json \
  -Iinclude --dump-sema your_source.c
```

That fixture policy is a small finite example, not a universal solver policy.
Inspect its admitted kinds before using it. Separate single-TU objects can link
through the ordinary ABI; linking alone is not cross-TU physical-contract proof.
Use the receipt compatibility checks described in [physical coverage](physical_coverage.md).
The public frontend API does not acquire policy support merely by enabling a bit.

## 3. Admit operations, representations and frames explicitly

Use [operation contracts](physical_operations.md), [representations](physical_representations.md),
[frames](physical_frames.md), [frame propagation](physical_frame_propagation.md),
[rotations](physical_rotations.md) and [points](physical_points.md).

Force dot displacement and lever arm cross force have distinct meanings. Raw
multiplication, matching dimensions or a return annotation does not establish
those meanings. A declared helper is not generally proof of its implementation.
Only bounded admitted constructors check the specified actual body structure.
Raw mixed matrices, arbitrary aliases and unsupported storage restoration remain
explicit gaps. Do not annotate them merely to suppress a required unknown.

## 4. Distinguish energy accounts from conversions

[Energy forms](physical_energy_forms.md) qualify scalar energy contributions.
Same-form sums preserve the form; different known forms produce `energy.mixed`;
an unknown input does not become a known form. Narrowing mixed energy to thermal
energy is refused. Forms do not add runtime payload or change the C ABI.

A directed transfer must be declared with a compatible policy and signature.
Its output implementation remains declared trust. A process label does not prove
conservation, efficiency, nonnegativity or component existence. Power/time mapping,
intervals, component identity and sign conventions need an explicit solver interface.

## 5. Read coverage before accepting a function

Use [units reporting](units_coverage_reporting.md),
[strict units](units_coverage_strict.md) and [physical coverage](physical_coverage.md).
A small reproducible units report is:

```sh
"$COMPILER" --overlay=physics-units --units-coverage-strict \
  --units-coverage-function travel \
  --emit-units-coverage-json "$OUT/travel.json" \
  --emit-units-coverage-text "$OUT/travel.txt" \
  tests/fixtures/solver_physics/coverage_reporting.c
```

Inspect command exit and reports together. Required rows must be checked or in an
explicitly permitted trust category, with no blocked row and some actual checked
work. Ordinary controls remain separate. Removing annotations cannot shrink the
obligation denominator to manufacture success. Trust-only/empty scopes fail.

Two separate permissions admit literal assumptions and declared boundaries.
Neither permission fixes missing metadata, unsupported operations or contradictions.
Review individual trust rows; accepted does not mean all rows were checked.
A literal zero can adopt an authored representation; it is an assumption, not an
independent measurement. A declared callee result is not body proof.

## 6. Diagnose the first broken boundary

| Observation | Inspect and act |
| --- | --- |
| Known dimension or concrete-unit mismatch | Find expected/actual units and the last legal intermediate; correct the formula or write an explicit conversion. |
| Same-dimension wrong kind/reference/frame | Check policy identities and the operation; do not cast or retag the destination. |
| Missing metadata / first loss location | Trace the source declaration or unsupported expression boundary; annotating only the destination does not prove its input. |
| Declared result trust | Review the external/helper implementation separately, or qualify its admitted body; trust remains visible. |
| Literal assumption | Confirm the constant's intended representation and review the permission separately. |
| Raw matrix/pointer/storage restoration | Keep the strict refusal; isolate and qualify an admitted interface rather than relabel raw slots. |
| Accepted source but incorrect runtime energy | Inspect numerical equations, sign, interval, reference and account boundaries with an independent runtime oracle. |

Reports retain source/loss locations and required/checked/trusted/blocked counts.
Use `--dump-sema` for resolved attachments and `--explain` with the reported code.
Do not infer physical coverage from annotation counts or successful code generation.

## 7. Exercise the existing teaching gates

These maintained runners reuse compiler fixtures and independent oracles. They
create the requested new output directories; do not pre-create those directories.
Build only isolated prerequisites if existing outputs must remain untouched:

```sh
make -j4 BIN="$OUT/fisics" BUILD_DIR="$OUT/build" \
  LIB_FRONTEND="$OUT/build/libfisics_frontend.a" \
  "$OUT/fisics" "$OUT/build/tests/physical_energy_contract" \
  "$OUT/build/tests/physical_frame_flow_contract"
COMPILER="$OUT/fisics"
python3 -B tests/integration/run_units_coverage_acceptance.py \
  --compiler "$COMPILER" --out "$OUT/units-acceptance"
python3 -B tests/integration/run_physical_affine_contracts.py \
  --compiler "$COMPILER" --out "$OUT/affine"
python3 -B tests/integration/run_physical_coverage_contracts.py \
  --compiler "$COMPILER" --out "$OUT/physical-coverage"
python3 -B tests/integration/run_physical_energy_contracts.py \
  --compiler "$COMPILER" --probe "$OUT/build/tests/physical_energy_contract" \
  --flow-probe "$OUT/build/tests/physical_frame_flow_contract" --out "$OUT/energy"
```

For the existing explicit conversion and registry diagnostic walkthrough, run:

```sh
bash tests/integration/run_overlay_units_scaffold.sh "$COMPILER"
```

This gate owns and removes its temporary sources and checks legal conversions
and illegal implicit unit mixes. It does not establish shared-Clang builtin support.

The units gate checks metadata removal, contradictions, unknown raw boundaries and
separate trust permissions. Affine/coverage gates exercise same-dimension meaning,
references and strict reporting. The energy gate covers aggregation, relabeling,
unknowns, transfer trust, cross-TU receipt controls and all four Clang/fisiCs calling
combinations. Its wrong transfer body compiles but fails the independent bookkeeping
oracle. That is deliberate evidence of the static/runtime distinction.

## 8. Adopt a bounded customer path

Inventory state and helper bodies; choose exact policies, source readsets and
accepted trust. Compile the actual source, run the strict gate, then independent
numerical/layout/ABI/consumer checks. Use [solver source qualification](solver_source_qualification.md)
for maintained subject routes and [confidence tiers](compiler_test_confidence_tiers.md)
for broader regressions. Select the relevant gate, not every solver for every edit.

Freeze FP mode, tolerances and cost budgets before comparisons. Unchanged runtime
implementation does not require new timing solely because prose changed. Changes
to codegen/ABI need runtime gates; new inference needs focused positive/negative/
unknown regressions. Canonical adoption, default selection, installation and release
are later decisions under the owning runbooks. Compilation and simulation do not
establish calibration or real-world physical acceptance.

For the finite general quantity/connection campaign, guards and known limitations,
see [Physical Compiler-flow Qualification](compiler_flow_qualification.md).

For bounded heterogeneous local arrays, see [immutable physical storage snapshots](physical_storage_snapshots.md). Dynamic raw matrix/DOF assembly remains outside that contract.

For explicit finite local array writes and accumulation, see [mutable storage provenance](physical_mutable_storage.md). This does not prove pointer-owned matrix assembly or dynamic DOF maps; immutable arrays retain their separate snapshot contract.


## Optional developer source kit

Released archives include the version-matched optional source kit under
`developer/`. Start at its `SOURCE_KIT.md` for the explicit LLVM22 analysis
preflight and clean valid/broken/unknown walkthrough. The bounded actual
matrix-flow checker additionally needs its separately supplied reviewed source
fixture. See [the tool's delivery instructions](../tools/matrix_flow/README.md).
Ordinary CLI compilation remains independent of this developer workflow.
