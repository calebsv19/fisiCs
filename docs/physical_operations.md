# Physical operation model and standard helpers

The opt-in `physics-semantics` overlay supports explicit `fabs` and `fmax`
contracts through physical policy schema version2, and finite ratio/time scalar
contracts through version3. Version4 adds four bounded mechanical application
contracts. Version5 adds separately declared [C representations and bounded
shape helper contracts](physical_representations.md). This is compiler-side metadata;
ordinary C values, Dim8, and the public frontend v1 ABI retain their representation.

## Compatibility and opt-in

Version1 physical policies keep their exact seven-field schema, fingerprints and
behavior. Their source receipts remain `fisiCs.physical_semantics` version1.
No new helper inference is enabled for a v1 policy.

Version2 policies use those same fields, set `schema_version:2`, and require an
additional `operation_rules` array. An empty array is valid and admits no helper
rules. Rules are not inferred from kind names, equal dimensions, the receiving
variable, or a user function's name. Version1 rejects the new field; a v2 policy
cannot be represented as a v1 receipt.

For an existing catalog declaring `quantity.energy` as a linear kind, the new
array can contain:

```json
[
  {
    "id": "rule.energy.abs",
    "operation": "fabs",
    "operand_kinds": ["quantity.energy"],
    "result_kind": "quantity.energy",
    "reference_role": "linear",
    "origin_policy": "preserve",
    "provenance": "inputs"
  },
  {
    "id": "rule.energy.maximum",
    "operation": "fmax",
    "operand_kinds": ["quantity.energy", "quantity.energy"],
    "result_kind": "quantity.energy",
    "reference_role": "linear",
    "origin_policy": "same",
    "provenance": "inputs"
  }
]
```

Each rule has exactly these seven fields. IDs must be canonical and unique.
Operand/result kinds must exist and must be the same canonical kind. Their Dim8
relationship is checked through the existing dimensional engine. The declared
reference role must match the loaded kind model. There is no wildcard, priority,
implicit commutation, kind conversion or override mechanism. Multiple rules for
one `(operation, kind)` reject as ambiguous even if they use different IDs.

Only scalar operands qualify; homogeneous arrays, vector/tensor intent and frame
compatibility remain later carrier/frame work. The current fixed primitive contracts are:

| Operation | Admitted roles | Result | Origin policy |
| --- | --- | --- | --- |
| `fabs` | linear, difference | same kind as its one operand | `preserve`; absolute points are unsupported |
| `fmax` | linear, difference, absolute | same kind as both operands | `same`; absolute operands require the same explicit origin |

Unknown kinds, unprofiled roles, incompatible roles/origins, quantity relabeling,
wrong arity, unsupported fields/operations and any provenance policy except
`inputs` reject model loading. At most4096 rules are admitted, within the existing
1MiB policy and aggregate catalog/model limits. Failed loads leave a previous
live model unchanged. Successful v2-to-v1 reloads remove the operation model.

`sqrt`, `hypot`, multiplication/division, dot/cross products and time-step rules
are not admitted by this operation schema yet. Definitions for unsupported
operations reject; source expressions retain unknown physical meaning even when
the dimensional engine can validate them. New rule families need their own
bounded contract and qualification before registration.

## Helper identity and source behavior

Use the ordinary portable C call and existing declaration macros:

```c
#include <fisics/extensions.h>
#include <math.h>

FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
double magnitude(double value FISICS_DIM(energy)
                       FISICS_QUANTITY_KIND("quantity.energy")) {
    return fabs(value);
}
```

Compile or report it with the existing overlay/policy CLI flags and a v2 policy.
For Clang the macros erase; they do not give Clang physical checking.

The physical evaluator reads the units layer's captured direct-callee identity.
Standard system math declarations, the existing supported builtin `fabs` route,
and recognized virtual generic wrappers use that established recognition path.
The `float` and `long double` standard variants use the same physical primitive.
User functions, local shadows, aliases and indirect calls do not gain standard
helper inference merely by using the same spelling. An independently annotated
user function remains a declared interface and requires the existing explicit
trust permission in strict scopes.

Known mismatched `fmax` kinds reject with4317; differing point origins reject
with4318. Dimensional and concrete-unit contradictions still reject through units.
Unknown operands remain unknown; neither a typed destination nor either strict
trust permission supplies missing operand kinds. Bare literals in a mixed helper
call do not acquire their neighbor's kind. Typed literal boundaries retain their
explicit assumption category.

A valid standard helper derives its result from the admitted input contracts.
Its operation row can be checked against that primitive model; this does not
prove the library implementation or runtime ranges. Literal and declared-call
trust propagate through helper inputs, including both trust bits in nested calls.
Such rows remain separate from checked inference. Strict still requires actual
checked work; a scope containing only permitted trust does not pass.

## Receipts and model identity

V2 source analysis emits physical receipt version2 with the existing v1 fields
plus `operation_model`. That object has exactly `version` (currently1),
`fingerprint`, and `rules`. Rules are serialized in canonical operation/kind
order. V2 expression records also include nullable `operation_rule` identities
carried with derived facts. Declaration signatures retain their existing fields.

The operation fingerprint uses the repository's FNV-1a64 convention and the
`fisiCs.physical_helpers.v1` domain: rule count, then each canonical rule's ID,
operation, kind, reference role, origin policy and `inputs` provenance. Strings
are length-prefixed UTF-8 and numbers are little-endian u64. V2 policy fingerprints
extend the unchanged v1 model encoding with the v2 domain and operation fingerprint.
Rule order and JSON formatting do not change model identity; rule identity and
contract changes do. These are consistency receipts, not authenticated signatures.

The cross-TU checker accepts both receipt versions, requires the matching loaded
policy version, and checks every serialized operation definition against the live
model. Missing, downgraded, modified or stale operation models reject. Existing
source/catalog freshness and signature restrictions remain. It compares declared
interfaces; it does not independently prove expression/body claims in a receipt.
Existing semantic/physical/hardware graph formats do not consume this schema.

Two existing limits remain explicit:

- Virtual `tgmath.h` source buffers cannot currently form a complete physical file
  receipt closure. Direct generic-helper recognition works, but strict/report
  publication refuses rather than claiming complete provenance.
- Two translation units including a shared math-header declaration with an
  unsupported pointer/record/callback/alias signature cannot pass the existing
  cross-TU checker. An admitted scalar producer/consumer boundary can pass when
  the unsupported declarations are not shared. This is separate from local helper
  checking; the operation model does not widen general signature admission.

## Structural prerequisite and qualification

The finite controls distinguish translation-to-force stiffness, force-per-angle
coupling, moment-per-displacement coupling, and rotation-to-moment stiffness.
The coupling pair has the same force dimensions; rotational stiffness also shares
energy/torque dimensions. Those quantities remain distinct kinds through helper
selection. No force-times-displacement inference is added. Updating the actual
pilot field annotations and admitting mechanical application rules remain a later
checkpoint; the P5.6 candidate profile is unchanged.

```sh
make physical-operation-contract-test
```

The gate covers policy/receipt compatibility, independent fingerprint encoding,
standard helper variants, same-dimension and origin failures, stable metadata-
removal denominators, combined trust, fake/indirect helper identities, unsupported
operations, a source-shaped normal-stress envelope, scalar cross-TU consistency,
receipt tampering and Clang/fisiCs numerical/layout controls. A native probe checks
atomic failed replacement, owned allocation thresholds, independent models and
v2-to-v1 lifetime handling.

`build_physical_policy_probe.py --cli` includes the helper/evaluator/receipt modules
for scoped ASan/UBSan or owned-allocation CLI checks. `--operations` selects the
native operation-model driver. Instrumentation covers selected new modules, not
all parser/JSON/LLVM internals; macOS LeakSanitizer remains disabled. Standard
helper types do not establish numerical stability, conservation, general matrix
provenance, frame identity, vector shape or physical model acceptance.


## Version 3 finite scalar rules (P6.1.2)

Version 3 keeps the same eight root fields and seven rule fields as v2, but
explicitly admits `multiply` and `divide` rules in addition to the unchanged
standard-helper rules. V1 and v2 retain their schemas, fingerprints and behavior;
v2 cannot opt into scalar products. An empty v3 rule array admits no operations.
The scalar table is deliberately finite:

| Ordered operands | Operator | Result |
|---|---|---|
| K, `quantity.ratio` | multiply or divide | K |
| `quantity.ratio`, K | multiply | K |
| `quantity.acceleration`, `quantity.duration` (either order) | multiply | `quantity.velocity` |
| `quantity.velocity`, `quantity.duration` (either order) | multiply | `quantity.displacement` |
| `quantity.displacement`, `quantity.duration` | divide | `quantity.velocity` |
| `quantity.velocity`, `quantity.duration` | divide | `quantity.acceleration` |

K must be explicitly linear or difference. Ratio is explicitly linear and has
zero Dim8. Duration is explicitly linear with time Dim8. Velocity and acceleration
are linear, respectively length/time and length/time squared. Displacement is a
difference with length Dim8, belonging to a declared affine pair. These canonical
IDs and dimensions define the bounded time primitive vocabulary. Merely declaring
an angle or another dimensionless kind cannot substitute for a ratio. Other
catalog names do not gain time semantics from their dimensions.

Every ordered form requires its own authored rule. No commutative or inverse rule
is synthesized. For example, with the relevant catalog and roles declared:

```json
{
  "id": "rule.acceleration.duration",
  "operation": "multiply",
  "operand_kinds": ["quantity.acceleration", "quantity.duration"],
  "result_kind": "quantity.velocity",
  "reference_role": "linear",
  "origin_policy": "none",
  "provenance": "inputs"
}
```

The result role must match the policy; scalar rules require `origin_policy` equal
to `none` and input provenance. Absolute/unprofiled kinds, carriers, unknown
operands and point origins cannot enter scalar inference. Dim8 arithmetic is
checked at load time using wider integer intermediates, and expression inference
also requires the existing units result to be resolved. Distinct rules for the
same operator and ordered operand pair reject, even with different result IDs.
Unknown/unsupported products stay unknown; destinations never choose a rule.
The 4096-rule and existing policy byte/allocation limits still apply. One sorted
policy-owned table and binary lookup serve helpers and scalar expressions; there
is no rule allocation per expression.

Scalar `*` and `/`, plus compatible `*=` and `/=`, use these rules. A compound
assignment must still satisfy its target kind. Existing point/difference addition
handles position updates after a velocity-duration result; scaling an absolute
position is refused. No energy/torque, mass, force/displacement, dot/cross, matrix,
frame or energy-form inference is added. This phase checks scalar components;
it does not establish a vector representation or coordinate frame.

Bare `0.5` in a mixed product stays unknown. Declare a ratio coefficient explicitly
at its literal boundary and authorize literal assumptions in a strict scope when
appropriate. Declared function results retain declared-boundary trust through
products. All applicable trust bits remain obligations, and a scope containing
only permitted trust still fails the existing checked-work requirement. Ordinary
control classifications and obligation denominators do not depend on annotations.
Storage reads retain the existing declaration-based model; this is not path-
sensitive proof of the values previously written to that storage.

V3 emits physical receipt version 3, with the same root fields as v2 and an
`operation_model.version` of 2. Ordered operand IDs, result IDs and each rule's
identity/role/origin/provenance enter its canonical fingerprint. Its hash domain
is `fisiCs.physical_scalars.v1`; the field sequence is rule count, then each rule's
ID, operation, left ID, right ID (empty for unary helpers), result ID, role number,
origin policy and provenance. Encoding remains length-prefixed UTF-8 and u64LE.
The policy extends the unchanged v1 model hash with
`fisiCs.physical_semantics_policy.v3` and the operation fingerprint. Sorting uses
helper codes fabs=2/fmax=4, multiply=100/divide=101, then canonical operand handles.
V2 helper hashing remains byte-compatible. The cross-TU checker requires exact
policy/receipt/model version agreement and live model equality; it preserves the
existing source-freshness and shared-signature restrictions. These receipts are
consistency records, not authenticated body proofs.

```sh
make physical-scalar-contract-test
```

This gate covers finite rules and failure controls, stable missing-metadata
obligations, ordinary controls, trust propagation, independent fingerprint
encoding, model tampering/freshness, native failed replacement/allocation sweeps,
and Clang/fisiCs formula and layout parity. It projects kick-drift-kick and
semi-implicit Euler formulas into a small scalar fixture. It does not migrate or
qualify an entire solver, prove a denominator nonzero, constrain runtime values,
prove integration accuracy, or establish timestep stability/conservation.
Version4 below supplies the bounded mechanical application contracts. Joined
solver/cost acceptance follows separately.


## Version 4 mechanical application rules (P6.1.3)

V4 retains the exact eight policy root fields and seven rule fields. It admits
the v2 helper and v3 scalar contracts plus four finite mechanical primitives:

| Coefficient kind | Coordinate kind | Result kind |
|---|---|---|
| `quantity.translational_stiffness` | `quantity.displacement` | `quantity.force` |
| `quantity.force_per_angle` | `quantity.angle` | `quantity.force` |
| `quantity.moment_per_displacement` | `quantity.displacement` | `quantity.torque` |
| `quantity.rotational_stiffness` | `quantity.angle` | `quantity.torque` |

The operator is `multiply`. Each operand order requires an explicit rule; no
commutative or inverse entry is synthesized. The coupling kinds share force
Dim8, but remain distinct. Rotational stiffness shares torque/energy dimensions
and likewise has its own kind. V3 still rejects these rules. V1–v3 schemas,
fingerprints, receipt versions and admitted behavior remain unchanged.

Coefficient and result kinds must be linear. Displacement may be explicitly
linear (as in the existing local structural carrier profile) or difference;
angle must be linear. Every named kind must have its exact expected Dim8:
force/length, force, force, force*length for the four coefficients; length or
zero for the coordinate; force or force*length for the result. Points, origins,
unprofiled kinds, missing metadata, carriers and arbitrary dimension-equivalent
catalog aliases cannot supply a mechanical primitive. No coordinate frame or
vector/matrix representation is inferred. An explicitly declared rule is still
required, even when all canonical operand names and dimensions match.

Example rule (with the referenced catalog kinds and roles declared):

```json
{
  "id": "rule.rotation.force",
  "operation": "multiply",
  "operand_kinds": ["quantity.force_per_angle", "quantity.angle"],
  "result_kind": "quantity.force",
  "reference_role": "linear",
  "origin_policy": "none",
  "provenance": "inputs"
}
```

Ordinary force is not a coupling coefficient. `force * displacement` does not
infer either energy or torque; neither does a receiving variable choose the
result. No stiffness construction, inverse division, distributed-load integration,
stress formula, work/energy, dot/cross or matrix operation is admitted here.
Existing strict/trust, dimensional resolution, ordinary controls, ambiguity,
resource ceilings and atomic-load requirements remain. Lookup reuses the sorted
policy-owned scalar table; the mechanical validator adds no per-expression
allocation or runtime value payload.

V4 emits physical receipt version4 and operation-model version3. The canonical
encoding is the v3 ordered operand/result encoding under the distinct
`fisiCs.physical_mechanics.v1` domain. The policy extends the unchanged v1 model
hash with `fisiCs.physical_semantics_policy.v4` and the operation fingerprint.
Model version, rule identities, operand order, result/role/origin/provenance,
source freshness and loaded policy must match. Existing body-proof and shared
signature limitations remain; receipts are consistency records.

```sh
make physical-mechanical-contract-test
```

The gate includes a separate mechanical profile adapter for the copied structural
candidate. It refines `tr`/`rt` fields, their coefficient locals and zero locals.
Line-load `x`/`y` fields receive a separate `quantity.line_load` kind rather than
sharing stiffness identity merely because their dimensions match. The shared
coupling formula is evaluated for two independently typed local values;
no semantic kind is reinterpreted as another. Erasing kind annotations and
collapsing the deliberate coefficient split recovers the prior header exactly.
The old P5.6 profile is unchanged; external/shared owners are not edited.

Selected copied `cs_physical_apply` has 26 checked obligations and two literal
zero assumptions; only literal-assumption permission is needed. Selected copied
`cs_physical_recover_end` has 28 checked obligations and needs no trust permission.
Both retain six ordinary controls and complete enumeration. Swapping or collapsing
coupling kinds rejects while preserving denominators. Construction and raw dense
restoration remain strict refusals; the former still contains unadmitted products.

Numerical qualification compares baseline Clang, refined Clang and refined fisiCs
against an independent 6x6 Euler-Bernoulli matrix, three displacement/rotation
cases, recovered end loads, and eight record size/offset checks. A sign mutant
must fail the numerical oracle. This proves the sampled scalar/helper behavior;
whole-solver integration, broader numerical/ABI acceptance, compile-time/memory
costs, conservation, stability and physical acceptance remain separate.


## Joined actual-source qualification

[P6.1.4](solver_source_qualification.md#joined-mechanical-solver-acceptance-p614)
qualifies the v4 profile in the full copied structural source candidate with the
existing numerical suite, analytical probe, six ABI combinations and frozen
compile-time/RSS budgets. Application/recovery strict coverage is accepted;
construction, line-load integration and raw restoration remain explicit gaps.
This checkpoint changes qualification workflows, not compiler rules or the shared
owner. It does not extend coverage to arbitrary matrix, frame or energy operations.

## V7 bounded structural construction and work contracts

The structural adoption model adds opt-in v7 scalar contracts for the admitted
planar Euler–Bernoulli path. V1–V6 rules, schemas and fingerprints are unchanged.
V7 retains its existing envelope and exact serialized operation model; an older
compiler refuses the new operations. No new runtime type or Dim8 representation
is introduced. A policy must explicitly list every ordered rule.

The finite arithmetic additions are pressure×area→axial rigidity,
pressure×second moment of area→flexural rigidity, geometric length²/length³,
EA/length and EI/length³→translational stiffness, stiffness×geometric
length→coupling coefficient, coupling×geometric length→rotational stiffness,
line load×geometric length→force, line load×length²→torque, force/area→pressure
and torque/section modulus→pressure. Canonical IDs and canonical Dim8 vectors
are checked at model load. Second moment of area has L⁴ dimensions; it cannot
be substituted by mass moment of inertia. Existing ratio rules scale the named
coefficient kinds only when explicitly listed. Bare numbers do not gain ratio
meaning from their neighbors or destination.

Geometric length is distinct from displacement. There is no global
force×geometric length→torque or force×displacement→energy inference. A generic
coupling coefficient also cannot be assigned directly to either matrix role.
Use explicit scalar construction identities where that interpretation is needed:

| Operation | Inputs | Result | Required definition body |
| --- | --- | --- | --- |
| `frame_coupling_tr` | coupling coefficient | force per angle | `return value;` |
| `frame_coupling_rt` | coupling coefficient | moment per displacement | `return value;` |
| `transverse_line_moment` | transverse resultant force, geometric span | torque | ordered product of the two parameters |
| `translation_work` | force, displacement | energy | ordered product of the two parameters |
| `rotation_work` | torque, angle | energy | ordered product of the two parameters |

These use the same seven operation-rule fields. The coupling rules have one
operand; the other constructors have two. All require linear results,
`origin_policy:"none"` and `provenance:"inputs"`. A constructor rule's ID is
selected by `FISICS_PHYSICAL_OPERATION("rule.id")` on the function result.
It must not collide with a shape-helper ID. Ordinary arithmetic rule IDs cannot
be used as constructor tags.

Constructor functions must be definitions visible at the call, with exact
by-value double parameter/result kinds and dimensions, no variadic parameters,
no pointer/record/vector, frame, point, origin or energy-form context, and a body
containing only the stated return. The compiler checks the AST parameters and
ordered expression rather than trusting a name or annotated return alone.
Changed bodies, declaration-only interfaces and indirect/cast calls do not gain
checked constructor inference. Known wrong kinds reject; missing metadata
remains unknown even with both strict trust permissions enabled. Input literal
or declared trust remains attached to the result.

The same exact primitive validates both a constructor's return expression and
its direct calls. These are checked scalar component contracts, not general
interprocedural body proofs or frame/basis/point geometry guarantees. The work
helpers compute endpoint contractions. They do not infer an elastic energy form,
path-integrated work, a loading history or conservation. Connecting them to the
solver's existing stored-energy accounting is separate work.

The source-owned example model is `core_structural/physics/frame_policy.json`
with its adjacent catalog. The maintained-header semantic opt-in is
`CORE_STRUCTURAL_PHYSICAL_SEMANTICS`. The stable qualification command is:

```sh
python3 tests/integration/run_physical_structural_contracts.py \
  --compiler /path/to/fisics \
  --source-root /path/to/shared/core/core_structural \
  --base-root /path/to/shared/core/core_base \
  --probe /path/to/selected/build/tests/physical_operations_contract \
  --out /tmp/physical-structural-new
```

It checks selected actual-owner kernels/helper bodies, finite rules and their
missing-rule/unknown controls, same-Dim8 mistakes, changed implementations,
indirect calls, identity collisions, legacy refusal, independently encoded model
fingerprints, cross-TU tampering, scalar numerical parity and native allocation
rollback. Numerical solver and layout acceptance remains a separate actual-source
gate described in [solver qualification](solver_source_qualification.md).


## Bounded thermal prerequisites

V7 also admits the finite thermal arithmetic and exact expm1/log/fmin helper
contracts described in [Thermal compiler contracts](physical_thermal_contracts.md).
The opt-in fixture gate is `make physical-thermal-contract-test`. It preserves
absolute/difference temperature, generic energy versus energy form, direct
standard-callee identity and strict unknown/refusal boundaries. It is not actual
thermal solver adoption.

## Bounded Cosmic scalar prerequisites

V7 also supports the explicit finite scalar primitives in [Cosmic contracts](physical_cosmic_contracts.md). The focused gate is `make physical-cosmic-contract-test`. Actual solver adoption and exact norm/trigonometric helper contracts remain separate.
