# Bounded thermal compiler contracts

The opt-in `physics-semantics` overlay supports a finite v7 thermal scalar
vocabulary alongside the existing dimensional and affine-temperature analysis.
These are compiler contracts, not thermal solver adoption or runtime diagnostics.
Ordinary C storage and the eight-exponent dimensional representation are unchanged.

The compiler-owned example policy is
[`tests/fixtures/physical_semantics/thermal/policy.json`](../tests/fixtures/physical_semantics/thermal/policy.json),
with a reusable catalog and portable source macros in the same directory.
Only explicitly declared operation rules are active. This adds operation choices
within the existing v7 envelope; a compiler without these choices rejects the
new policy rules. Existing policies retain their operation fingerprints.

## Finite arithmetic

| Expression | Result kind | Dimensions |
| --- | --- | --- |
| power × duration | energy | J |
| energy / duration | power | W |
| heat_capacity × temperature.difference | energy | J |
| energy / heat_capacity | temperature.difference | K interval |
| thermal_conductance × temperature.difference | power | W |
| power / heat_capacity | temperature_rate | K/s |
| thermal_conductance / heat_capacity | inverse_duration | 1/s |
| inverse_duration × duration | ratio | 1 |
| heat_capacity / heat_capacity | ratio | 1 |

Multiplication supports either order when the ordered rule is present. Existing
ratio scaling remains available. Canonical `quantity.*` IDs, exact Dim8 vectors,
linear/difference roles and seven-field operation contracts are validated at
load time. The temperature difference must have the difference role; absolute
temperature is excluded from these scalar tuples. Heat capacity is J/K;
conductance is W/K. Neither is conductivity or volumetric heat capacity.

`energy / heat_capacity` does not reconstruct absolute temperature or select a
storage zero. Zero-at0K storage and material-relative enthalpy require their own
explicit model/reference boundary. `power * duration` yields generic energy:
it does not infer `energy.thermal`, dissipation, a heat-transfer process or a
reference origin. Metadata absence is not dimensionless, and a cast cannot
relabel torque as energy. Correctly typed scalar array elements and harmless
casts retain existing metadata.

## Exact standard math helpers

`expm1`, `log` and `fmin`, including their `f` and `l` suffixes, use resolved direct
standard `math.h` declarations with exact real floating prototypes. User-defined
implementations, function-pointer calls and unseeded builtin lookalikes do not
receive these contracts. New type-generic wrappers are not admitted here.

- Units analysis requires dimensionless inputs for `expm1` and `log` and returns
  a dimensionless result. Physical analysis additionally requires the canonical
  linear `quantity.ratio` kind and an explicit unary rule with origin policy
  `none`. A dimensionless angle is not a ratio.
- `fmin` follows the bounded `fmax` contract: matching dimensions, compatible
  concrete units and, physically, the same quantity kind and origin. Its rule
  preserves the result kind with origin policy `same`. Energy-form and frame
  propagation continue through the existing conservative helper paths.
- Exact helper recognition does not prove runtime domains: callers still own
  positivity for `log`, finite values, range, material bounds and error handling.
  The standard numerical operations are not rewritten. In particular, `expm1`
  retains its small-argument behavior.

Recognized helpers with unknown or contradictory inputs stay blocked in strict
coverage. Literal assumptions and declared boundaries remain separate opt-in
permissions; neither permits a known kind or reference contradiction.

## Qualification

```sh
make physical-thermal-contract-test
```

For a retained fresh result directory and an isolated selected compiler:

```sh
python3 -B tests/integration/run_physical_thermal_contracts.py \
  --compiler /path/to/fisics \
  --probe /path/to/build/tests/physical_operations_contract \
  --out /tmp/thermal-contracts-new
```

The six compiler-only gates cover finite arithmetic, exact helper identities,
semantic failures, model/allocation compatibility, receipt integrity and
executable numerical/layout parity. The executable fixture uses Clang with
macros disabled/enabled, ordinary fisiCs, units, physical and reordered-policy
configurations. These are fixture configurations, not a six-variant whole-solver
ABI qualification. An independent80-digit Decimal oracle checks the tiny
exponential difference and logarithm within one double ULP.

A passing fixture does not establish coverage of a thermal owner's implementation,
checkpoint compatibility, actual host interfaces, conservation, hardware or
physical model validity. Those remain source-adoption and numerical-owner gates.

## Actual owner acceptance

The finite v7 rows additionally cover area/length/volume, volumetric capacity,
conductivity, area-specific resistance, volumetric enthalpy and the intermediate
capacity-per-length, conductance-length and inverse-capacity quantities used by
finite-volume thermal arithmetic. These require exact canonical dimensions and
explicit policy rows. Absolute storage restoration remains a declared boundary.

`make integration-thermal-owner-acceptance` is an optional external-owner gate.
Supply `THERMAL_CORE_ROOT`, `THERMAL_HOST_ROOT`, `THERMAL_BASELINE_CORE`,
`THERMAL_BASELINE_BIN`, `THERMAL_PHYSICS_ROOT` and a fresh
`THERMAL_ACCEPTANCE_OUT`. The baseline is a frozen pre-adoption core tree and its
exact native host binary, retained for legacy replay. Select isolated `BIN`,
`BUILD_DIR` and `LIB_FRONTEND` using the normal Make overrides. The direct Python
entrypoint accepts the corresponding hyphenated options; `--probe` selects the
native allocation-failure driver. A `--skip-host` diagnostic run never reports
full acceptance.

The gate checks the actual two-body, constant rod, material rod, participant,
cycle and bounded receiver sources. It inventories functions and verifies every
selected arithmetic kernel is used by the real source. Strict kernel validation
is separate from measured enclosing raw admission/control and reference
restoration boundaries. Private matrix arrays distinguish J/K coefficients,
J right-hand sides and K increments; real call-site swapping controls must fail.
It does not claim whole-owner strict coverage, Python producer validity or new
inter-domain feedback.

Six ordinary/portable/compiler/overlay numerical and layout configurations,
old C consumers, faint-power high-precision oracles, numerical mutations, actual
host consumers, receiver checks and matched compile-time/RSS budgets must pass.
New source gets a new engine identity; old checkpoints must refuse unchanged in
the new engine and replay with the exact old helper. This gate never updates a
checkpoint, installed app, source hash expectation or release.

## Fixed-resistor producer/receiver contract binding

`make integration-electrothermal-contracts` is an opt-in source-bound gate for
modeled, memoryless fixed-resistor loss into the existing Thermal assembly.
It changes no C storage, solver law, runtime schema, package or default compiler.
Set `ELECTROTHERMAL_COMPILER`, `THERMAL_CORE_ROOT`, `THERMAL_HOST_ROOT`,
`ELECTROTHERMAL_ESP_ROOT`, `ELECTROTHERMAL_NATIVE`,
`ELECTROTHERMAL_NATIVE_IDENTITY`, `ELECTROTHERMAL_LEGACY_NATIVE` and a fresh
`ELECTROTHERMAL_OUT`, plus `THERMAL_PHYSICS_ROOT` for the existing receiver
inventory header dependency. That root is not gas-transfer numerical acceptance. This target consumes explicit already-built tools; it never
rebuilds an owner's default product. The native identity is the owning build's
generated `thermal_semantic_identity.json`; all its live input hashes, complete
input set and engine digest must match. The distinct legacy helper is retained
only to qualify original-engine replay/restart behavior.

The direct CLI uses the corresponding `--compiler`, `--core-root`, `--host-root`,
`--esp-root`, `--native`, `--native-identity`, `--legacy-native`, `--physics-root`,
and `--out` options.
`--binding` defaults to the finite
`tests/fixtures/physical_semantics/electrothermal/fixed_resistor_boundary.json`.
With the same explicit tools/owners, `--verify-receipt /path/to/results.json`
rechecks input/artifact bytes and joins without rewriting the receipt. A complete
`results.json` is published atomically only after the gate and its refusal
controls pass. Failed runs retain `failure.json`; preflight failures may occur
before creating output. Existing output directories are never reused.

The gate reuses the actual thermal owner's C inventory/strict checker, verifies
`assembly_cycle_power` calls `ct_share_power` and `ct_remaining_power`, and
`assembly_cycle_integral` calls `ct_cycle_energy`. It binds those real selected
kernel receipts to current modeled producer schemas, component/layer allocations,
source/circuit/plan fingerprints, mapped scenario, native engine and immutable
runtime transfer receipts. The existing 51-kernel manifest, raw controller
inventory, form and typed-storage failure controls remain separate evidence.
Python producer/reference tests and runtime admission are not C compiler proof.

The finite binding distinguishes W, J, duration and absolute temperature.
`energy.thermal` is an explicit validated-ingress interpretation for dissipated
energy; it is not a form on watts or a compiler inference across Python/JSON.
Imported heat is `source_j`, not the participant packet `cell_exchange_j` route.
Unallocated loss is reported rather than deposited, and authored powers must be
zero. Opaque runtime IDs are receipt identities, never global physical types.

Four current authored/raw/scheduled/film-boundary-loss cases, same-engine split/restart and
observation replay, old-engine restart refusal and exact old-tool continuation,
wrong W/J/kind/raw C fixtures, source/profile/identity/allocation and rehashed
wrong-meaning controls qualify this bounded join. All negative C fixtures remain
valid ordinary Clang C. A digest binds bytes; these receipts are not signed
attestations or authentication. Whole-owner strictness, Python dataflow inference,
hardware, RC/storage profiles, signed gas packets and coupled feedback are excluded.
The maintained prefix diagnostics are described below. Expanded transactional controls are described below. Final ABI/resource/broad
acceptance remains a separate qualification step.

## Accepted-prefix diagnostic reports

The same opt-in acceptance gate now publishes `runtime/<case>/diagnostics.json`,
`partial-diagnostics.json`, `zero-diagnostics.json` and `diagnostics/summary.md`.
Every accepted sample, including sample zero, reports component/layer allocated J
through that sample, pending allocated J after it, exact rational sample time,
native source/sink, material-relative storage change, inward signed boundary
energy, closure residual and absolute temperatures. The source material reference
and nonzero initial stored energy remain separate from imported heat.

The three insulated controls require stored change = accepted allocated J and,
for constant capacity, capacity-weighted temperature change = accepted J / C.
The fourth case uses the existing film-boundary configuration (ambient293.15K,
left/right h5/10W/m²/K) with a current constant modeled resistor export.
Boundary energy is `left_face_j - right_face_j`, positive into the solid;
boundary loss is negative and does not become `sink_j`. Source supply and
unallocated dissipation are explicitly full-electrical-horizon accounts, not
accepted thermal heat. Pending is a historical value at each sample, not a claim
that a completed experiment still has queued heat.

Reports use the saved, native-validated hexadecimal samples to preserve binary64
values. Ordinary live inspection JSON may round a number or scenario power by an
ULP. Energy/temperature observations are cross-checked using the existing 1e-10 J
and 1e-8 K limits. Scenario identity uses the already validated frozen scenario,
rather than dictionary equality with rounded live JSON. No production serializer
or solver tolerance is changed. Material metadata
and the signed boundary convention follow native and heated-member inspection.
The Structural-dependent heated-member inspector is not instantiated by this gate.
No new thermal equations or Structural solve are introduced. Nonconstant heat
capacity keeps its native storage diagnostics but has no constant-C prediction.

A separate read-only CLI can inspect an existing complete or partial frozen bundle:

```sh
python3 -B tests/integration/run_electrothermal_diagnostics.py \
  --host-root /path/to/thermal-owner --native /path/to/thermal_assembly_agent \
  --bundle /path/to/thermal-electrical-bundle --through 37 \
  --output /path/to/new-prefix-diagnostics.json
```

Omit `--through` for all available accepted samples; `--through 0` selects the
initial sample. Historical `sample` differs from the source trace's global
`completed_steps`: a completed100-step record may be viewed through37, while an
actual37-step checkpoint refuses a38th sample. The report names available and
reported steps separately. Partial checkpoints are validated in a private native
process solely for inspection: no original source-file read, resume, step, save
or engine relabel occurs. A native engine mismatch still refuses checkpoint load.

Use `--verify-report /path/to/report.json` instead of `--output` to independently
rebuild and compare a report. Creation validates first and atomically links a new
file; existing outputs survive, failed/future selection publishes nothing, and
output inside the immutable input bundle is refused. The source files are hashed
before/after inspection. Readback works from retained frozen members after original
temporary source paths disappear; actual restart still has its stronger original
source/adapter/engine obligations.

The acceptance receipt is now `fisics.electrothermal_acceptance.v3`, requiring
full/partial diagnostics, their hashes/denominators and the derived Markdown
summary to agree with native observations. The physical binding and runtime
bundle schemas remain unchanged. Earlier v1/v2 acceptance receipts stay frozen with
their original scope/source checker; no receipt or expected hash is upgraded.
Correctly rehashing false consumed/pending J, time/reference/boundary accounting or
a truncated report cannot satisfy the current receipt. These runtime diagnostics do not confer
whole-owner C strict coverage, hardware acceptance or coupled feedback.


## Importer and accepted-prefix failure qualification

The same opt-in gate includes a separate `electrothermal_adversarial.py` test
module and publishes `adversarial/report.json`. It executes malformed imports
through the actual owner importer: unsupported component/layer/plan, duplicate
allocation, supply substituted for loss, authored layer/cycle power, gaps,
overlaps, future/missing support, RC profiles, negative/nonfinite loss,
port-energy substitution and changed model. The controls rehash source bytes
and interval identities where appropriate, require the intended refusal reason,
and require no new output directory. Existing accepted records remain unchanged.

A balanced ledger with every power and energy consistently doubled intentionally
passes importer consistency admission. It fails comparison with a fresh run of
the actual producer under the declared setup. This demonstrates the external
model-reference obligation; it is not a compiler or importer physics inference.
Hashes alone cannot establish that arbitrary incoming numbers obey a circuit.

A real material upper bound permits the first thermal candidate but refuses the
second. Tests check whole-batch rollback from zero and a nonzero accepted prefix,
repeat refusal, unchanged revision/numerical state and identical lossless saved
journals. Diagnostic error text may change. Failed execute/resume publishes no
experiment, a valid first-step prefix remains usable, and recovery in the same
native session reaches the exact original full trace. Adapter/original-source
refusal and recovery, create-only destination preservation and scheduled restart
splits 0,1,2,9,10,47,72,73,99 are also required. Existing old-engine controls remain.

Portable C fixtures accept same-form and mixed-form aggregation but reject
mixed/unknown narrowing, cast relabelling and restoration through a raw scalar.
Ordinary Clang accepts all fixtures without extension enforcement. The v3 receipt
binds their selected results, exact adversarial coverage, independent model
witness and lossless atomic/recovery/restart traces. Rehashed missing coverage or
an older v2 scope cannot satisfy that contract. These are bounded source/runtime
checks, not a claim of whole-owner strictness, hardware acceptance or final ABI
and resource-cost qualification.


## Final fixed-resistor boundary acceptance

`make integration-electrothermal-acceptance` composes the numerical, consumer,
transaction, compiler/frontend and resource checks for this finite boundary. It
is opt-in and is not a dependency of `make test`. Set:

- `ELECTROTHERMAL_COMPILER`: a source-qualified compiler executable;
- `THERMAL_CORE_ROOT`, `THERMAL_HOST_ROOT`, `ELECTROTHERMAL_ESP_ROOT` and
  `THERMAL_PHYSICS_ROOT`: explicit current owners;
- `THERMAL_BASELINE_CORE`: the immutable pre-adoption thermal source/header/test
  snapshot, and `THERMAL_BASELINE_BIN`: its retained exact original CLI;
- `ELECTROTHERMAL_LEGACY_NATIVE`: the exact original assembly helper;
- `ELECTROTHERMAL_ACCEPTANCE_OUT`: a new evidence directory; and
- `ELECTROTHERMAL_ACCEPTANCE_SCRATCH`: a separate new disposable directory.

The direct CLI `tests/integration/run_electrothermal_acceptance.py` takes the
corresponding `--compiler`, `--core-root`, `--host-root`, `--esp-root`,
`--physics-root`, `--baseline-core`, `--baseline-bin`, `--legacy-native`, `--out`
and `--scratch` arguments. Optional `--frontend-build-root` reuses a compatible,
task-owned compiler build rather than building its archive and tests afresh.
The selected compiler is held read-only during broad tests; defaults are not
selected as output. Keep paths free of whitespace for legacy Make test recipes.

The gate runs six core variants against original public headers: original
Clang/fisiCs, current ordinary/portable Clang, units and physical semantics.
All variants must have identical layouts and numerical test output. Original
Clang drivers call every core variant; original-header fisiCs drivers also call
current Clang core objects. Actual native, units and physical host consumers run
the owner tests. ABI hybrids are disposable numerical probes and cannot serve as
restart provenance for the Clang identity compiled into their host objects.
Only the actual newly built native helper and identity are retained for replay.

The existing boundary gate supplies actual C contracts, independent modeled
producer/overlap/storage references, full/partial signed diagnostics, transaction
controls and old-engine policy. A final compiler `test frontend-contract-test`
checkpoint follows. Three legacy example/receipt recipes are redirected to owned
scratch with the same scripts and assertions. No goldens, receipt semantics or
runtime equations change.

Five actual translation units (four core sources and assembly cycle) receive
matched base/units/physical compilation measurements. Nine selected functions,
including the real power-sharing and cycle-energy helpers, receive strict/report
measurements. Each mode/function has nine measured fresh processes plus one
warmup. The unchanged `solver_overlay_budget.json` applies. Other gate children
finish before these serial measurements; the result is compiler wall-time and
peak RSS qualification, not runtime or Python adapter performance.

The separate `fisics.electrothermal_final_acceptance.v1` receipt joins source,
tool, numerical/layout, native/consumer, v3 boundary, broad/frontend and cost
artifacts. The inner v3 receipt keeps its existing schema and obligations. With
the same explicit owner/tool/baseline arguments, `--verify-receipt` reads existing
final evidence without rebuilding or resuming. It checks current pins, artifact
bytes, inner semantics, native identity and cost samples against the current
unchanged budget. Existing directories refuse before test effects; a failed run
has `failure.json` and cannot publish `results.json`.

Retain the compact evidence/native tools and immutable baseline dependencies for
readback; remove owned objects, archives, ABI hybrid executables and scratch only
after active-consumer checks. Frozen traces remain inspectable after scratch
source paths disappear, while actual restart keeps its stronger original-source
requirements. This acceptance does not imply canonical adoption, whole-owner
strictness, hardware calibration, installed UI acceptance or arbitrary feedback.

## Prescribed Thermal-to-Structural source pilot

The optional `integration-thermostructural-source` gate checks the existing
section-uniform, constant-property, linear-elastic spatial thermal-strain helpers
under an explicit Structural v7 policy. It requires an explicitly selected
compiler, core root and fresh output directory:

```sh
python3 -B tests/integration/run_thermostructural_source_contracts.py \
  --compiler /path/to/selected/fisics \
  --core-root /path/to/shared/core/core_structural \
  --out /fresh/thermostructural-source-proof
```

The policy distinguishes absolute Kelvin temperature with `origin.kelvin.zero`,
temperature difference, inverse-K `quantity.thermal_expansion_coefficient`, and
dimensionless `quantity.mechanical_strain`. The finite rules cover alpha times
a temperature difference, axial rigidity times strain, displacement divided by
geometric length, and strain times geometric length. Absolute temperature cannot
replace a difference; a ratio or angle cannot silently become strain. A malformed
linear temperature-difference role is rejected at policy admission.

Actual validation, element and recovery sites use the private helpers. Selected
physical arithmetic and elastic aggregation are checked; completed-square bending
is dimensionally checked. Raw SI ingress, local basis/DOF selection, EB coefficient
and linear-elastic energy classification remain declared boundaries; whole
spatial functions and matrices are not physically strict. Portable macros erase
for Clang; ordinary Clang is a runtime compatibility control, not semantic proof.

The external history bridge binds complete original-engine observations, exact
stored indices/cells, geometry/materials and explicit mechanical Tref/alpha.
Tref is independent of Thermal material min_k. Each sample is a separate static
equilibrium; mechanical load factor does not scale eigenstrain. Thermal heat and
Structural generalized elastic energy are separate accounts, with no feedback or
heat debit. Shared runtime records, public ABI and Dim8 are unchanged. Numerical,
ABI, failure/recovery, native-consumer and resource qualification are separate
from this source-only gate; there is no implicit default compiler adoption.
