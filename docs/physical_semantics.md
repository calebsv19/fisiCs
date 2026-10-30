# Opt-in physical quantity checking

P5.3/P5.4 add physical-kind and affine-reference checks alongside the existing physics-units overlay.
Kinds distinguish quantities with identical dimensions, such as energy and torque,
or angle and ratio. Dim8 and concrete-unit rules remain authoritative and separate.
Numeric values, C types and runtime layout stay ordinary C.

## Enable one translation unit

```sh
./fisics --overlay=physics-semantics \
  --physical-semantics-policy physical.policy.json \
  -I include -c solver.c -o solver.o
```

`physics-semantics` implies `physics-units`. It does not enable entity metadata or
invoke an offline graph. The existing default, `all`, and units-only overlay
selections retain their meanings and do not enable physical checking. An explicit
policy is required; a policy without the physical overlay also fails. The existing
`FISICS_OVERLAY` selection mechanism can select the new overlay, but does not
supply or discover a policy.

This checkpoint accepts one direct C source, including `-c` or a single-source
executable build, with internal preprocessing. It refuses multi-TU, object-input,
build-manifest and external-preprocessing combinations. Separate one-source
compilations can produce ordinary C objects, but that does not establish physical
cross-TU compatibility. Physical policy options are an internal CLI integration;
public frontend v1 options/results are unchanged and cannot load this policy.
Selecting the new feature bit through that unprepared API fails explicitly.

## Author explicit kinds

For shared Clang/fisiCs source, use `<fisics/extensions.h>`:

```c
#include <fisics/extensions.h>

double stored_energy
    FISICS_DIM(energy)
    FISICS_QUANTITY_KIND("quantity.energy") = 3.0;
double applied_torque
    FISICS_DIM(energy)
    FISICS_QUANTITY_KIND("quantity.torque") = 2.0;

void update(void) {
    stored_energy = applied_torque; /* physical boundary error 4316 */
}
```

Native equivalents are `[[fisics::quantity_kind("quantity.energy")]]` and
`[[fisics::physical_origin("origin.world")]]`. Kind/origin names are canonical
string literals. Use separate bracket pairs; combined lists, malformed values,
and duplicate annotations on an individual slot fail. With the physical overlay
disabled, recognized native annotations diagnose4310. Clang sees the portability
macros as empty declarations; it does not perform the physical checks.

Under policies v1–v4, admitted carriers are real numeric scalar variables, individual parameters,
scalar fields, homogeneous scalar array contents, and scalar function results.
Place field/array annotations after the declarator, as with units. Prefix
annotations on variables/functions are normalized to their actual declaration
slots; shared prefixes apply to each declarator and suffixes remain isolated.
Under those versions, whole-record quantities, physical pointer values, annotated typedef aliases,
complex values and aggregate-result quantities are refused. Ordinary pointer or
record C types without a quantity annotation remain usable.

Source retains its dimensional annotation: the kind must agree with the existing
Dim8 facts. A missing dimension remains unverified; a known contradictory dimension
is4314. No dimension or kind is inferred from a variable name, unit name, entity ID,
frame declaration, or a policy ID's spelling.

## Minimal policy/catalog

The [policy/model reference](physical_policy_model.md) specifies strict schemas,
ownership, fingerprints, limits and diagnostics. For the example above,
`physical.policy.json` contains:

```json
{
  "schema": "fisiCs.physical_semantics_policy",
  "schema_version": 1,
  "policy_id": "example.physics",
  "catalog_paths": ["physical.catalog.json"],
  "linear_kinds": ["quantity.energy", "quantity.torque"],
  "affine_pairs": [],
  "origins": []
}
```

`physical.catalog.json` contains existing catalog v1 definitions:

```json
{
  "schema": "fisiCs.physical_contract_catalog",
  "schema_version": 1,
  "catalog_id": "example.quantities",
  "quantity_kinds": [
    {"id": "quantity.energy", "dimension_system": "fisiCs.si_dim8.v1",
     "dimension_vector": [2,1,-2,0,0,0,0,0], "dimension_text": "energy"},
    {"id": "quantity.torque", "dimension_system": "fisiCs.si_dim8.v1",
     "dimension_vector": [2,1,-2,0,0,0,0,0], "dimension_text": "energy"}
  ],
  "contracts": []
}
```

Catalog paths resolve explicitly relative to the policy file. No directory,
environment-variable or network catalog discovery occurs. Existing catalog and
graph schemas are unchanged. See `tests/fixtures/physical_semantics/` for a richer
finite qualification policy including points, differences, and origins.

## Checked operations and conservative boundaries

When both identities are available, scalar assignment/initialization, typed
arguments, returns and conditional merges require the exact same kind and point
origin. Compatible redeclarations inherit omitted contracts; conflicting variable,
function-result or parameter declarations fail4315. Linked block-scope extern
paths use the same rule. Earlier captured raw uses are not retroactively proved.

Same-kind linear/difference addition and subtraction preserve kind. Distinct
known linear kinds reject4317. Unary signs preserve linear/difference kinds.
Comparisons require compatible known identities and yield ordinary C controls,
not `quantity.ratio`. Numeric scalar casts preserve identity and cannot relabel
torque as energy. Function-pointer parameter checks use the actual captured C
signature slots; matching callback signatures are not proofs of their bodies.

Scalar field accesses and homogeneous array elements retain their declared kinds
through the captured C access type. Direct dereference of an authored address is
admitted. Untracked scalar-pointer aliases, void/record casts, union type punning,
and cast callable signatures remain unverified. Unsupported expression boundaries
do not recover a kind by matching dimensions or by the destination's expected type.
There is no general pointer/data-flow or whole-record provenance proof.

Literal-only initializers, plain writes and literal returns can take an authored
boundary's representation. These are **literal assumptions**, not inferred
physical facts. Bare literals inside physical arithmetic do not inherit an
operand's kind. A declared call result is **interface trust**, not proof of the
callee's implementation. Trust is retained through supported expression forms.

Unknown input remains unknown on the initializer/assignment/return expression;
a typed destination does not turn that boundary into a checked derivation.
Subsequent reads of the declared storage still expose its authored contract.
The recorded unchecked write is not erased. This checkpoint performs local AST
compatibility checks rather than path-sensitive numerical provenance analysis.

Absolute declarations require an explicit matching origin; linear/difference
declarations reject a point origin. P5.4 uses explicit policy pairs for scalar
point/difference arithmetic. With A an absolute point and D its paired difference:

| Operation | Result |
| --- | --- |
| A + A | error4317 |
| A - A | D, with no point origin; same kind and origin required |
| A + D, D + A, A - D | A, preserving the point operand's origin |
| D - A | error4317 |
| D + D, D - D | D, requiring the same difference kind |

Pairing comes only from the loaded policy, never matching dimensions or names.
Unrelated same-dimension kinds reject. Different-origin point subtraction,
comparison, assignment or merge rejects4318. A point/difference comparison or merge
also rejects. Numeric casts cannot retag kinds or origins. Compound assignments
check both the arithmetic result and the destination: A += D and A -= D work;
A -= A fails because its difference result cannot be stored as a point.

Both operands' literal/interface trust is retained, including its loss location.
A missing operand kind remains unknown, and bare literals acquire no implicit
kind in arithmetic; independent dimensional checks can still reject the expression.
Unprofiled addition/subtraction and unary signs on points remain unverified.
Reference algebra does not perform coordinate-frame or Celsius/Fahrenheit offset
conversion and does not assess wrap policy or physically admissible temperatures.

Products/divisions, work or torque derivation, dot/cross products, differentiation,
integration, general unit-conversion propagation, frame/shape rules and energy-form
transformations have no physical inference rule here. Dimensional analysis still
runs independently; physical output remains unknown with its loss location.

## Inspect and validate

`--dump-sema` adds a debug-only physical section when enabled: canonical IDs,
origins, known/unknown/control/contradictory state, literal/interface trust,
boundary decisions and first loss locations. `storage-complete` describes bounded
fact storage/traversal, not full physical coverage. Hash indexing supports local
node/slot lookup; insertion order preserves deterministic debug output without
serializing AST addresses.

Diagnostics4301–4306 retain policy/model meanings. Source diagnostics4310–4318 cover
disabled annotations, malformed/unsupported attachments, unknown IDs, Dim8 conflict,
redeclaration, boundary, physical-operation and origin mismatch. `--explain` supports
all codes. Boundary diagnostics retain canonical expected/actual identities,
roles/origins and source locations. Unavailable indirect-signature locations are
marked unavailable rather than invented.

```sh
make physical-semantics-contract-test
make physical-affine-contract-test
make physical-coverage-contract-test
make physical-policy-contract-test
make frontend-contract-test
```

The affine gate checks the full table for positional, kelvin and arbitrary-ID
pairs, unrelated same-dimension identities, wrong origins, compound assignments,
unknown/trust controls and Clang/ordinary/physical numerical parity. Its native
gate checks72 combinations of result identity, cleared/preserved origin and
assumption loss locations.

The source gate checks required failures, unknown/trust controls, ordinary C and
Clang numerical/layout parity, and double calling ABI in both cross-compiler
directions. The native gate checks independent contexts,17 allocation-failure
thresholds, disabled state, and unprepared public-frontend refusal. The reusable
probe builder supports `--semantics` for the native gate or `--cli` for the full
source gate under scoped ASan/UBSan or an owned-allocation ledger. Existing JSON,
C analysis, LLVM and parser-owned attribute storage are not whole-compiler leak
qualified by that scoped test. LeakSanitizer is unavailable on this macOS host.

P5.5 adds [selected physical strict coverage and receipts](physical_coverage.md),
with actual preprocessor-buffer read-set provenance and a separate cross-TU physical
declaration checker. The ordinary source-checking path above still accepts one TU;
existing units reports retain only their units coverage meaning. P5.6 owns joined
solver acceptance and cost qualification. Compilation alone does not prove
simulation physics, conservation, coordinate frames or hardware.


## Declared standard-helper rules

P6.1.1 adds [explicit physical operation contracts](physical_operations.md) for
recognized standard `fabs` and `fmax` calls under a v2 policy. V1 policy behavior
is unchanged. These rules preserve admitted operand identity; they cannot infer
missing kinds, relabel quantities or waive origin/unit contradictions. P6.1.2 adds explicit v3 ratio scaling and finite translational time operations;
see the same operation reference. General mechanical products, dot/cross,
carrier/frame rules and energy qualifiers remain later checkpoints.


P6.1.3 adds four v4 scalar mechanical application contracts and a refined copied
structural carrier profile; see [physical operations](physical_operations.md).
Selected copied application and end-recovery helpers now pass strict checks with
explicit coupling identities. Construction, raw restoration, frame/representation,
work/energy and whole-solver acceptance retain their separate boundaries.

## Version 5 explicit representations (P6.2)

The [representation contract](physical_representations.md) admits explicitly
bound Cartesian2/3 float/double records/fixed arrays, plus homogeneous square2/3
rank2 storage/copy. The same C storage can carry separately declared quantity
kinds. Resolved record typedefs work; quantity annotations on typedef definitions
do not define a universal physical type. Scalar point/torque behavior under older
policies remains unchanged. Pointers, adjusted array parameters, absolute point
vectors, unknown extents, attributed/anonymous layouts and heterogeneous records
remain outside the admitted binding surface.

Copy/component checks use actual C storage. Finite add/subtract/ratio-scale,
force-dot-displacement and 3D lever-arm-cross-force helpers require a resolved
callee identity and exact declared signature. Generic calls cannot restore a
represented quantity. Helper implementation and common Cartesian basis are
declared trust; frames are unassessed. This is no general solver or body proof.

## Energy qualifier model

[Energy forms and declared transfers](physical_energy_forms.md) add opt-in v7
aggregation, conservative mixed results, exact directed helper contracts and
separate declared implementation trust. Dim8 and v1–v6 behavior remain intact.
Use `make physical-energy-contract-test`; solver adoption remains separate.
