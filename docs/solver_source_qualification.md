# Actual solver source qualification and overlay cost

The compiler supports opt-in, finite selected-function physics gates. A passing
selected scope does not qualify its callers, raw matrix storage, every solver in a
library, runtime physical truth, or deployment. See [strict policy](units_coverage_strict.md).

## External structural source candidate

`tests/integration/run_units_structural_source_contracts.py` qualifies a hash-bound
candidate patch against an explicitly supplied actual `core_structural` source
owner. It copies only source/test/build-description inputs into a fresh output
root, verifies the patch input hashes, and applies the patch to that snapshot.
The live external source tree is read-only. This is source-candidate proof;
application of the patch to the shared owner and downstream adoption are separate.

```sh
python3 tests/integration/run_units_structural_source_contracts.py \
  --compiler ./fisics \
  --source-root /path/to/shared/core/core_structural \
  --base-root /path/to/shared/core/core_base \
  --out /tmp/structural-source-proof-new
```

The equivalent make target is `integration-units-structural-source-contracts`,
with explicit `STRUCTURAL_SOURCE_ROOT`, `STRUCTURAL_BASE_ROOT`,
`STRUCTURAL_SOURCE_OUT` and the selected `BIN`. Outputs must be fresh. Source
hash drift refuses patch application; review/rebase the candidate and its pins
explicitly before another qualification. Do not bypass pins to reuse old proof.

The patch in `tests/fixtures/solver_physics/structural_source/` puts typed internal
translation/rotation blocks into the actual planar element, loading and recovery
sources. It retains the public heterogeneous six-DOF dense ABI. The four selected
kernels construct the complete local stiffness, apply it, form uniform equivalent
loads and recover member end forces. Each implementation is strictly checked;
constructor/application literal zeros remain explicit permitted assumptions.
The dense SI-restoration adapter is deliberately rejected by strict checking.
Model admission, raw section/state/load imports, raw packing/restoration, releases,
coordinate transforms, global assembly/solve, stress extrema and energy integration
remain reviewed or numerically tested boundaries outside that strict scope.
There is no claim of end-to-end inferred provenance through the raw matrix.

The runner uses the actual module Makefile source list, compiling original Clang,
patched Clang and patched fisiCs variants. It runs eight existing library tests per
variant plus an independent nine-case cantilever probe: point, distributed and
combined loads across three lengths, including all 36 stiffness entries, reactions,
displacements, end forces, stress and energy/work. The stated absolute/relative
tolerances are both `1e-8`; numerical equivalence does not promise bitwise identity.
It compares the remaining reported chord values across variants and preserves
public model/result/member layout. Four legal-C wrong-quantity mutations must
reject directly and in strict reports. Missing coordinate metadata and raw
restoration must fail strict qualification. A dimensionally legal coefficient
mutation must compile and fail the existing numerical frame test.

The control compiler uses strict C11 warnings, `-O0` and FP contraction off;
fisiCs uses its default code-generation level and FP contraction policy. Both
produce objects for the same platform linker. This is not a claim that all Clang
Makefile flags are accepted by fisiCs. Backend `Error:` text in a supposedly
successful compile is treated as a harness failure as well as a nonzero exit.

## Maintained structural owner

After adopting the typed carriers into the actual owner, use
`--maintained-baseline /path/to/frozen-unannotated-owner` with the same command.
This mode never applies a candidate patch. It freezes the maintained source and
compares it against the supplied baseline: only the new private physical header
and the element, loads and recovery call sites may differ (README text may
also change). Other source, tests, public headers, Makefile and VERSION must
match. Preserve a baseline from immediately before adoption; do not substitute
an older module snapshot after unrelated development has advanced.

The receipt records the baseline and maintained-source hashes, four call-site
mappings, complete strict obligation enumeration and numerical/layout results.
Mutations run only on the disposable copy. The live owner and supplied baseline
must remain unchanged throughout qualification. Three builds compare unannotated
Clang, maintained annotated Clang and maintained fisiCs with physics-units.

The earlier P7.2 refined fixture mode remains available for its frozen inputs.
For the current maintained P7.3 header, also pass
`--structural-policy /path/to/core_structural/physics/frame_policy.json`.
This enables the explicit semantic source switch and physical policy for the
fisiCs numerical build. The model and catalog hashes are pinned for the run.
Use `run_physical_structural_contracts.py` separately for physical-strict scope
acceptance and semantic failure controls.

The four physical-strict scopes have reviewed checked/literal/control counts
61/7/15 (construction),26/2/6 (application),16/2/0 (equivalent loads) and28/0/6
(end recovery). Their enumeration is complete with no blocked obligations and
no declared-boundary permission. The source uses typed ratios and separate TR/RT
constructors. The consistent-load moment helper preserves the original left-to-
right multiplication order. Ordinary Clang needs no fisiCs installation.

The units-only engine sees helper interfaces rather than the physical engine's
exact-body constructor proof. Construction therefore has57checked/11trust
(seven literals and four named interface obligations), and loads13checked/5trust
(two literals and three named interface obligations). Declared interfaces are
permitted only in those selected dimensional scopes; helper bodies are separately
checked. The fixed counts prevent added boundary trust from silently passing.
Application and end recovery keep their previous dimensional counts. Stripped
metadata and raw SI restoration still refuse even with declared permission.

When another owner changes unrelated numerical sources during a run, stop live
whole-module acceptance. Preserve and identify that drift, bind the selected
maintained files to their qualified snapshot, and rerun numerical checks against
the exact frozen source and compiler. Do not silently replace old source pins or
claim the newer unrelated solver features were qualified. Whole-owner live
qualification requires a fresh stable baseline and its own readback.

## Matched compiler cost

`tests/performance/measure_overlay_cost.py` measures the same fisiCs executable,
target/default optimization, include/define arguments and fresh object outputs.
For each TU, one Clang `-std=c11 -fno-blocks -E -P -D__FISICS__=1` expansion
supplies both inputs. Only native `dim`/`unit` attributes are erased into whitespace
for the overlay-disabled baseline; all remaining numerical C text is identical.
Direct annotated-source compilation with the overlay disabled intentionally rejects
the attributes, so it cannot serve as a successful baseline. Preparation is outside
the sample time: these are matched preprocessed-input compiler costs, not full
raw-source preprocessing or end-to-end application build costs. It requires macOS `/usr/bin/time -l`; peak
RSS is bytes. A caller supplies a workload JSON manifest containing
`translation_units` (`id`, `source`, `args`), `strict_functions` (also `function`)
and an explicit `dependencies` path list. Reports retain dependency hashes, compiler
hash, CPU/platform/load, every preprocessing/sample command, prepared-input hashes and erased-attribute
counts, warmups and summaries. Paths are
caller supplied; no external workspace location is assumed.

```sh
python3 tests/performance/measure_overlay_cost.py \
  --compiler ./fisics --manifest /tmp/solver-cost-manifest.json \
  --budget tests/performance/solver_overlay_budget.json \
  --out /tmp/solver-cost-new
```

Unset `FISICS_OVERLAY`; inherited overlay settings are refused so the baseline
is controlled. The budget must be recorded before measurement. The initial solver budget uses
nine non-warmup samples per mode: median overlay wall time at most `1.5 * base +
0.05 seconds`, and median per-process peak RSS at most `1.5 * base + 8 MiB`.
Each measured strict/report sample must take at most one second and 256 MiB.
Strict parsing/report I/O is measured separately from object compilation, not
added to the overlay ratio. Both modes warm up, paired order alternates, and no
other qualification job is deliberately run alongside measurement. Fresh
processes/outputs do not imply a flushed OS cache or an idle machine. Reported
medians/ranges and the 10 ms wall-time resolution must be considered when interpreting
small ratios. Clang is a numerical control, never the overlay cost baseline.

The result is a finite host/workload qualification, not a universal performance
guarantee or a historical full-suite timing baseline. Successful disposable object,
archive, executable and copied-source trees should be removed after evidence is
read back and any measurement consumer finishes. Retain compact results/logs,
patches, input identities and useful failure reproducers.

## Structural-source C initializer regression

`make final-manifest MANIFEST=08-initializers-layout-structural-string-subobject.json`
checks string initialization of character-array fields with following scalar
fields, compound assignments, nested/brace-elided aggregates, designators and
zero-filled tails against an ordinary C control. The code generator recognizes a
string as one complete character-array subobject and builds its declared extent
before a generic aggregate copy, preventing subsequent field entries from being
consumed into the character array. This is a bounded initializer repair; it does
not enlarge the physics strict policy.


## Physical semantic solver acceptance (P5.6)

The physical overlay is qualified separately from dimensional strict coverage.
Use an explicit current structural owner, base owner and new output root:

```sh
make integration-physical-solver-acceptance BIN=./fisics \
  STRUCTURAL_SOURCE_ROOT=/path/to/core_structural \
  STRUCTURAL_BASE_ROOT=/path/to/core_base \
  PHYSICAL_SOLVER_OUT=/tmp/physical-solver-new
python3 tests/performance/measure_overlay_cost.py \
  --compiler ./fisics \
  --manifest /tmp/physical-solver-new/acceptance/cost-manifest.json \
  --budget tests/performance/solver_overlay_budget.json \
  --out /tmp/physical-cost-new
```

The `--physical` structural runner uses separate source-only patch pins under
`structural_physical/`; the historical units fixture stays unchanged. It captures
current owner hashes, applies the candidate only to a copy, then adds an explicit
nine-kind profile to its internal carriers. Erasing those semantic annotations
recovers exactly the units candidate's numerical text. A source pin mismatch
refuses; review/rebase explicitly rather than relaxing pins.

The gate runs original Clang, candidate Clang and physical fisiCs through eight
existing structural tests and the independent nine-case cantilever oracle. It
preserves public layouts and requires a dimensionally legal numerical mutant to
fail. The physical acceptance runner then uses the copied actual carrier header
for strict copy/relay projections, metadata-removal controls, same-dimension
rotational-stiffness versus torque rejection, scalar cross-TU checks, and six
numerical/layout/ABI combinations including both mixed Clang/fisiCs directions.
Those generated projections are not original solver bodies. Declared-call trust
stays separate from checked rows. Record/pointer receipt admission stays refused.

Under the legacy v1 profile, the four actual construction/application/load/recovery
kernels refuse physical strict because they lack operation contracts. Raw dense restoration also refuses and has incomplete enumeration.
A successful dimensional kernel report does not imply physical strict coverage.
No shared solver adoption or whole-solver semantic provenance follows from this gate.

Cost manifest `physical_policy` enables ordinary/units/physical modes. New cost
results use `fisics.overlay_cost.v2`; legacy two-mode manifests remain accepted.
Three-way inputs come from one preprocessing expansion, with only attributes erased
to whitespace for the relevant mode. Every mode gets a warmup and nine fresh process
samples in rotating order. Physical versus ordinary, units versus ordinary and
physical versus units use the frozen ratio-plus-allowance budget. Selected physical
strict and incomplete-report costs are separate; `selected_body_accepted` records
the semantic verdict independently from the cost-budget `accepted` field. A strict
item sets `physical:true`; `accepted:false` requests a deliberately incomplete
report workload. Policy/catalog and caller-listed source/header hashes, prepared
input hashes, compiler hash, host, commands and peak RSS are recorded. Unset both
`FISICS_OVERLAY` and `DISABLE_CODEGEN`. macOS resource collection needs permission
for the clock-rate sysctl used by `/usr/bin/time -l`.

The initial bounded P5.6 run passed all frozen budgets on Apple M2/macOS arm64:
ordinary/units/physical median compilation was60/70/70ms for element and loads,
and80/80/90ms for recovery. Physical peak RSS medians were40.19,40.00 and44.39MiB;
physical versus ordinary RSS increased2.1–3.4%. Wall measurements are coarse10ms
samples. These are prepared-input compiler costs, not runtime or full rebuild costs;
large-policy scaling and end-to-end solver semantic adoption remain separate.


## Bounded mechanical application checkpoint

`make physical-mechanical-contract-test` qualifies the copied structural header's
application and end-recovery helpers under the explicit v4 mechanical model.
`solver_mechanical_profile.py` is a separate refinement adapter; the old P5.6
profile remains unchanged. It distinguishes force-per-angle from moment-per-
displacement fields and locals. Coefficient construction and raw restoration stay
strict refusals. Kind erasure plus normalization of the coefficient split recovers
the previous header; three compiler/profile variants match the independent local
matrix/load oracle and record layouts. These checks do not run the complete
structural test suite or measure the joined solver cost. The P6.1.4 workflow below
provides that separate acceptance checkpoint. Optional `--source-root` on the focused runner verifies
that the external owner still matches the earlier candidate's patch input pins;
it neither copies nor modifies the owner tree.


## Joined mechanical solver acceptance (P6.1.4)

The refined v4 profile is now qualified in the actual-source candidate, not just
in the focused copied-header gate. The shared owner remains read-only:

```sh
make integration-mechanical-solver-acceptance BIN=./fisics \
  STRUCTURAL_SOURCE_ROOT=/path/to/core_structural \
  STRUCTURAL_BASE_ROOT=/path/to/core_base \
  PHYSICAL_SOLVER_OUT=/tmp/mechanical-solver-new
python3 tests/performance/measure_overlay_cost.py \
  --compiler ./fisics \
  --manifest /tmp/mechanical-solver-new/acceptance/cost-manifest.json \
  --budget tests/performance/solver_overlay_budget.json \
  --out /tmp/mechanical-cost-new
```

The equivalent scripts use `--mechanical` on both
`run_units_structural_source_contracts.py` (implies `--physical`) and
`run_physical_solver_acceptance.py`. Legacy `--physical`/P5.6 entry points retain
the original profile and behavior. Every output root must be fresh.

Qualification verifies current source pins, copies the owner sources, applies the
same candidate patch and the separate mechanical profile, then runs the eight
retained structural regression tests and the independent nine-case cantilever/matrix
probe in original Clang, refined Clang and physical-overlay fisiCs variants.
The probe checks 459 analytical values and 468 finite values per variant, including
324 matrix entries, deflections, reactions, recovered forces/stress and work/energy.
The existing absolute/relative tolerance is 1e-8. Public model/result/member sizes
and offsets match. A dimensionally legal stiffness coefficient mutant must fail
the actual numerical frame test. Numerical work/energy checks do not imply that
those formulas have physical semantic operation rules. Newer spatial, thermal and
fiber-specific test suites are outside this bounded planar qualification; the
eight-test set is the retained workload, not the entire current test catalog.

The v4 actual-source selected application/recovery bodies accept respectively
26 checked + two literal-zero assumptions, and 28 checked without trust permission.
Both retain six ordinary controls. Coupling swaps/collapse and metadata removal
refuse without shrinking the relevant obligation denominators. Construction,
equivalent line-load integration and raw dense restoration remain strict refusals;
the raw restoration report remains incomplete. This is bounded physical coverage.

Six ABI combinations separately test actual carrier records/pointers and scalar
quantity calls: all-Clang, all-physical, both mixed directions, ordinary fisiCs and
units fisiCs. They agree on a 48-byte load carrier, moment offset32 and numerical
output. Scalar receipt joins pass in both orders and kind conflicts reject; this
does not widen receipt admission for shared pointer/record signatures.

The mechanical source result binds the exact policy/catalog bytes. Joined
acceptance requires those bytes, the compiler hash, the complete copied candidate
hash map and the expected profile mode. Mutated candidate/model/compiler inputs
and mismatched legacy/mechanical modes refuse. These are consistency guards, not
cryptographic authentication of an untrusted producer's claims.

The generated cost manifest measures three actual translation units across
ordinary/units/physical compilation, accepted carrier relay/application/recovery
strict scopes and a deliberately incomplete construction report. It uses the
unchanged nine-sample budgets above. `strict_functions` may specify physical
`permissions` (`literal-assumptions` and/or `declared-boundaries`); omitted permissions
retain the legacy defaults. The mechanical manifest grants literal permission only
where needed, and none for recovery/relay. Optional `expected_checked` and
`expected_permitted_trust` bind each sampled selection's meaning. Selection counts
are recorded separately from timing/RSS budget verdicts. The legacy units-only
cost-manifest path remains supported.

The bounded Apple M2/macOS arm64 P6.1.4 run passed all 13 comparisons/selection cost
gates: 130 processes including warmups. Median prepared-input object compilation:

| TU | Ordinary ms / MiB | Units ms / MiB | Physical ms / MiB |
|---|---:|---:|---:|
| element | 40 / 38.95 | 40 / 40.12 | 50 / 39.62 |
| loads | 40 / 38.33 | 40 / 38.69 | 50 / 40.11 |
| recovery | 50 / 42.81 | 60 / 44.34 | 60 / 43.91 |

Accepted strict application/recovery maxima were100/110ms and50.44/49.47MiB.
The incomplete construction report cost also passed; its physical coverage still
failed. OS cache/load remain uncontrolled, and wall times resolve only to10ms;
these results do not establish a general speed or memory improvement, end-to-end
build cost, runtime performance, large-model scaling or physical model acceptance.
Owner adoption, additional operation rules, representations, frames and energy
qualifiers remain separate work.

## P6.2 representation pilots

`tests/integration/run_physical_representation_pilots.py` takes explicit
`--math-root`, `--structural-root`, `--compiler`, `--out` roots. The permanent
`integration-representation-solver-acceptance` target accepts `MATH_SOURCE_ROOT`,
`STRUCTURAL_SOURCE_ROOT`, and fresh `REPRESENTATION_SOLVER_OUT`. A hash-bound
fixture pins the exact owner header/source bytes and refuses drift before copying.
No owner source or compiler default is adopted.

The pilot copies core_math Vec2f/Vec3f definitions and nine actual by-value helper
bodies unchanged, adding declaration metadata only. Six toolchain combinations
(Clang, physical fisiCs, both mixed directions, base and units controls) compare
120 independent numerical values each, plus C sizes/offsets and matrix copying.
Cross-sign and dot-term mutants still compile but the independent oracle refuses
them, preserving the difference between declared contracts and numerical proof.

The structural rank2 pilot copies the `axes[3][3]` member into a homogeneous
storage adapter. The original CoreStructuralSpatialBasis contains an origin too;
its whole record is not promoted. This establishes storage/copy, not a basis or
frame transformation. Selected matrix copy has two checked obligations and no
trust; selected dot caller has three checked and two declared-trust obligations.
Existing scalar structural acceptance remains a separate regression gate.

The cost manifest uses identical preprocessed numerical source in base/units/
physical modes, erasing the new metadata alongside earlier annotations. The
existing nine-sample wall/RSS budgets remain unchanged. Compiler descriptor and
fact sizes are reported separately from unchanged runtime C values. See
[the representation contract](physical_representations.md) for exclusions.

## Runtime planar-frame and raw numerical boundaries

The maintained structural path uses directional private C carriers in
`core_structural_frame_boundaries.h`: world/local displacement-and-rotation
coordinates, world/local force-and-moment loads, world/local line loads and
world/local mixed stiffness matrices. The six-DOF order remains
`[ux_a,uy_a,rz_a,ux_b,uy_b,rz_b]`. Directional parameter types reject local/world
or coordinate/load confusion in both ordinary C and fisiCs. An explicit cast or
a different runtime member cannot acquire frame identity from a C type name.

The basis is the member's existing runtime cosine/sine. The v7 registry's fixed
3D double helpers do not prove these runtime2D mixed-DOF transforms. No fake
single member-local frame ID is assigned. The new adapter scopes deliberately
refuse physical-strict acceptance even with both trust permissions. Their frame
identity, orthonormality and endpoint references remain declared numerical
premises, verified through independent orientation/release/rigid-motion tests.
Quantity-only P7.3 kernels keep their existing selected acceptance; that does not
qualify their enclosing adapters or all callers as frame checked.

The actual element, load and recovery paths call the named transforms, release
condensation/rotation helpers and local pack/restore boundaries. Proper planar
rotation leaves axial nodal moments unchanged and keeps their endpoint reference;
there is no moment rebasing or point/origin transformation. Stiffness uses
`T^t K_local T` in its original evaluation order. Global scatter/reduction uses
the actual compiled free-index map, including ties and supports.

`cs_frame_dense_scaled_solve_assumed_si` names the existing raw scaled numerical
kernel behind the unchanged internal `cs_frame_linear` entrypoint. RHS storage
changes from force/moment to normalized values and then displacement/rotation;
full/reduced/geometry/scale arrays retain mixed numerical meanings. Neither the
matrix nor those arrays are assigned a single dimension/kind/frame. Named raw
restoration copies the solved free coordinates back to world DOFs before the
existing publication/residual/energy checks. Joint assembly and whole global
assembly remain explicitly unqualified numerical boundaries. Geometry/index/rank
controls are not decorated with fabricated physical metadata.

For a fresh current pre-adoption snapshot that already contains P7.3 carriers,
use the existing numerical runner with `--boundary-adoption`,
`--maintained-baseline /path/to/pre-adoption-owner` and the explicit
`--structural-policy` argument. This mode allows only the six static call-site
files and the new private boundary header to differ. It retains the three-build,
eight-tests-per-build, analytical/layout and mutation checks. Earlier frozen
P7.2/P7.3 fixture modes and their pins remain separate.

Then qualify the adapters against that exact numerical output:

```sh
python3 tests/integration/run_structural_frame_boundary_contracts.py \
  --compiler /path/to/fisics \
  --source-root /path/to/shared/core/core_structural \
  --base-root /path/to/shared/core/core_base \
  --numerical-output /path/to/qualified-numerical-output \
  --out /tmp/structural-boundaries-new
```

The runner binds compiler, selected sources, model, numerical receipt and linked
libraries. It checks maintained call identities, directional C failures, explicit
static refusals and309independent values per Clang/units/semantic build. References
cover identity/vertical/reversed/oblique orientations, endpoint virtual-work
parity, rigid translation/rotation, full diagonal-matrix transformation, all four
release combinations, fixed-end loads, tied-DOF reduction and a2x2 scaled solve.
Eight legal-C mutations cover sign, transpose, DOF ordering, force/moment and
released-end mistakes, basis erasure and result restoration. Their numerical
oracle catches the body errors; static refusal is not advertised as body proof.
The fixture also checks nonfinite matrices/loads and invalid solver scales.
Public layouts and numerical admission tolerances retain their existing contract.
Actual energy/work diagnostic migration and combined consumer/resource acceptance
remain later checkpoints.

## Static elastic-energy and endpoint-work diagnostics

The maintained static planar-frame path connects the private
`core_structural_frame_energy.h` helpers to publication and recovery. It reuses
the existing force–displacement and torque–angle contraction kernels and the
force/area and torque/section-modulus stress contracts. The member and joint
accounts have the same `energy.elastic` form; their sum retains that form.
Component identity is an accounting obligation independent of energy form.

`external_work_j` and `algebraic_external_work_j` keep their existing meaning:
final-load endpoint contraction `F_final·u`, including `M_final*theta`. For an
admitted linear static ramp, `U = (F_final·u)/2`. The existing `2U-F_final·u`
acceptance test checks static consistency; it is not a general time-dependent
conservation law. There is no energy-transfer rule in this elastic path.

The selected `cs_frame_elastic_sum` body is physically checked without declared
trust. The two Gauss quadrature bodies have checked dimensions. Constitutive
classification into elastic energy, raw polynomial coefficients, runtime DOF
selection and SI restoration remain explicit numerical/trust boundaries. Those
scopes, and the enclosing publication/recovery functions, must refuse physical
strict acceptance even with both trust permissions. Specific forms cannot be
recovered by an annotation or a scalar cast. Thermal energy cannot be supplied
where elastic energy is required. No single quantity is assigned to the mixed
force/moment or displacement/rotation arrays; public C layouts stay unchanged.

The exact old multiplication/grouping order, member and joint contributions,
`force_on_a=k*(q_b-q_a)` sign, equivalent-load subtraction, reaction/stress
calculations, finite gates, tolerances and atomic publication remain the numerical
contract. The reusable numerical runner's `--energy-adoption` mode additionally
requires `--boundary-adoption`, a frozen P7.4 `--maintained-baseline`, and
`--structural-policy`. It permits only the new private energy header and the
recovery/publication source files to differ. Unrelated owner changes must be
mirrored identically into both frozen inputs and disclosed separately.

After that numerical gate completes, run:

```sh
python3 tests/integration/run_structural_energy_contracts.py \
  --compiler /path/to/fisics \
  --source-root /path/to/qualified/core_structural \
  --base-root /path/to/core_base \
  --numerical-output /path/to/qualified-numerical-output \
  --out /tmp/structural-energy-new
```

The runner binds source/model/compiler and the numerical receipt, verifies live
helper call sites, tests strict acceptance/refusal and same-dimension wrong forms,
and compares independent energy/work/stress references across three toolchains.
A beam with nonzero translational and rotational springs independently checks
member/joint/algebraic energies, endpoint torque work, signs and reactions across
the three numerical libraries. Missing components, duplicate components, sign and
half-factor errors remain legal C but must fail the numerical oracle. Keep this
proof separate from whole-solver physical strictness, general physical validity,
consumer adoption and installed-app qualification.

## Combined maintained static-owner acceptance

`tests/integration/run_structural_owner_acceptance.py` joins the bounded static
structural qualification in one opt-in entrypoint. It freezes current shared and
Structural Sim CLI inputs, records the current source/function inventory, and
runs the existing numerical, physical, energy and frame/raw-boundary gates.
Sibling modules keep their existing responsibilities; no solver library, runtime
scheduler or app dependency is added by this test orchestration.

The input baseline is the pre-energy-connection owner snapshot; the reference is
the already qualified energy-connection snapshot. Only their selected numerical
source delta is used. All unrelated dependencies are mirrored from the current
owner into both comparison builds. The runner refuses an unexpected energy-source
delta and checks all frozen shared/consumer source identities again at closeout.
This preserves a numerical comparator while qualifying current dependencies.

From the compiler checkout, choose fresh build and proof roots:

```sh
make -j4 BUILD_DIR=/tmp/structural-proof-build \
  LIB_FRONTEND=/tmp/structural-proof-build/libfisics_frontend.a \
  BIN=/tmp/structural-proof-fisics \
  /tmp/structural-proof-fisics \
  /tmp/structural-proof-build/tests/physical_energy_contract

python3 -B tests/integration/run_structural_owner_acceptance.py \
  --compiler /tmp/structural-proof-fisics \
  --shared-root /path/to/shared \
  --consumer-root /path/to/structural_sim \
  --p75-baseline /path/to/pre-energy-connection-snapshot \
  --p75-reference /path/to/energy-qualified-snapshot \
  --model-probe /tmp/structural-proof-build/tests/physical_energy_contract \
  --out /tmp/structural-proof-new
```

The consumer input is the actual supported Structural Sim source tree, with its
Make, identity, CLI and example contracts. The runner selects the headless binary
and identity targets, rather than unrelated aggregate GUI/history targets. It
checks a normal Clang build and two builds linked against the qualified Clang and
semantic fisiCs numerical archives. Archive hashes supplement the consumer's
ordinary build identity. CLI codec/refusal/lifecycle checks and frame-result
schema/value comparisons run against each executable; JSON numbers use the
existing numerical tolerance, while keys, strings and booleans compare exactly.
This is CLI source qualification, not installed or GUI adoption.

Six ABI projections cover Clang/Clang, physical/physical, both mixed directions,
base/base and units/units. They execute the actual public sizes/offsets, physical
carrier pointer relay and scalar torque call. Scalar physical receipts are also
checked in both orders. Shared pointer/record signatures remain refused by the
cross-TU receipt checker; their runtime ABI evidence is separate. This does not
extend receipt support or establish arbitrary aggregate/callback ABI support.

The Clang AST inventory includes each function's source/body hash, signature,
calls, category and selected report. A report-mode result never becomes a strict
claim: physical-kernel acceptance is joined to the separate passing strict gate.
Dimension-only quadratures, raw numerical/context boundaries, controls, shared
dispatch and deferred inertia functions stay explicitly classified. Enclosing
raw solver functions are not advertised as physically strict.

Cost qualification requires macOS `/usr/bin/time -l` resource statistics. The
unchanged `tests/performance/solver_overlay_budget.json` uses nine samples plus
warmup, rotating base/units/physical order, identical preprocessed numerical
source outside erased attributes, and separate selected strict/report workloads.
It measures four actual translation units, including diagnostic publication,
only after the runner's other compilation finishes. Compiler fact/context sizes
and allocation-failure model reload are checked separately. Peak compiler RSS is
not a runtime solver heap/stack or physical-accuracy measurement.

`--resume` reuses completed source/receipt identities and verifies numerical
archives, ABI projection sources and consumer binary identities before reuse.
A failed measurement can use a fresh `--cost-output` without changing budgets.
Failed stages remain failures until qualified. Successful disposable builds may
be retired after delivery and an active-consumer check; keep compact receipts,
source pins and needed replay baselines. No compiler default, VERSION, installed
app, canonical merge or release is changed by this qualification entrypoint.

## Connected prescribed-temperature assemblies

`tests/integration/run_connected_assembly_contracts.py` is an opt-in current-source
qualification runner. It reuses the prescribed-temperature scalar contracts and
links the actual thermal translation unit into the existing Structural native
host. It adds no solver equations, physical policy rules or default build cutover.

Run from this compiler checkout with explicit compatible inputs:

```sh
python3 -B tests/integration/run_connected_assembly_contracts.py \
  --compiler /path/to/fisics \
  --core-root /path/to/shared/core/core_structural \
  --core-build /path/to/fresh/clang-core-build \
  --structural-root /path/to/structural-source \
  --native /path/to/fresh/clang-native \
  --thermal-native /path/to/pinned/thermal_assembly_agent \
  --out /tmp/connected-proof-new
```

The core archive and objects must come from the same explicit fresh build.
The output must not exist. The selected compiler uses its matching checkout's
extension headers. Inputs are hash-bound and source drift refuses acceptance.

The gate checks portable source contracts, actual mixed-host execution, an
independent planar frame reference, exact multi-source stored-time joins,
mechanical-only availability, rigid shared-node restraint, spatial rotation,
fixed-field subdivision, both ABI directions, native CLI, fresh external
transports, solver-free reopening and inspection. It compares 18 recorded case
snapshots across matched Clang, ordinary fisiCs, units and physical profiles.
The Clang exact comparator uses O0 and contraction off. The original optimized
native baseline has a separate physical-reference gate; cross-policy bitwise
identity is not required. Existing numerical reference tolerances are unchanged.

Physical strict coverage remains selected scalar arithmetic. Bending energy is
units-only; raw mixed matrices, local-basis/SI and constitutive restoration and
Python/runtime admission remain explicit external boundaries. Neutral codec rows
are zero eigenstrain, not observed zero Kelvin. Shared structural joints do not
couple heat transport. Per-derived-element deflection limits do not certify an
authored parent span. These checks do not establish whole-solver physical
acceptance, arbitrary spatial accuracy, Thermal PDE convergence, installation or
canonical adoption. Higher-order thermal laws and dynamic feedback stay separate.

## Elastic dynamic energy

See [selected dynamic energy contracts](physical_dynamic_energy_contracts.md) for the opt-in actual-source gate, typed scalar/units-only/raw boundaries, explicit policy and v2 diagnostic account. This adds no default compiler selection or whole-solver physical claim.

## Bounded axial contact energy

See [contact energy contracts](physical_contact_energy_contracts.md) for the opt-in immutable-donor/Particle contained-candidate route, source checks and explicit runtime/ownership limits.
