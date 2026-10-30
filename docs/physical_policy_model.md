# Physical catalog and policy model

P5.2 provides a reusable **compiler-internal** reader and owned model for physical
quantity identities. P5.3 now connects this model to the explicitly opt-in
[physical source checks](physical_semantics.md). Dim8 and ordinary C data layout
remain unchanged. The model API described here is internal; public frontend v1
structures do not acquire a policy field.

## Catalog reuse

`src/Compiler/physical_contract_catalog.{h,c}` owns the existing strict
`fisiCs.physical_contract_catalog` version 1 reader, quantity canonicalization,
contract uniqueness, and catalog-only dimension/unit checks. The physical project
graph uses this reader and retains its own entity, frame, source-evidence,
conversion, and coverage joins in their previous order. Existing catalog and graph
schemas are unchanged. A `reference_kind` field added to a quantity definition is
still an error; reference arithmetic belongs to the separate policy.

The neutral catalog header preserves the graph's existing quantity and contract
structures. Vector/tensor declarations remain descriptive contracts; this reader
does not validate their C representation or resolve frame/reference subjects.
Identical quantity definitions merge. Definitions with the same ID but different
dimension systems, Dim8 vectors, or dimension text conflict. Algebraically
equivalent dimension text is still distinct definition text, matching catalog v1.

## Separate explicit policy

`src/Extensions/PhysicalSemantics/physical_policy.h` is the internal API. A version 1 policy
has exactly these seven root fields, with no optional or unknown fields:

```json
{
  "schema": "fisiCs.physical_semantics_policy",
  "schema_version": 1,
  "policy_id": "example.physics",
  "catalog_paths": ["physical.catalog.json"],
  "linear_kinds": ["quantity.energy", "quantity.torque"],
  "affine_pairs": [
    {
      "absolute_kind": "quantity.temperature",
      "difference_kind": "quantity.temperature_delta"
    }
  ],
  "origins": [
    {"id": "origin.kelvin", "absolute_kind": "quantity.temperature"}
  ]
}
```

The example assumes its catalog declares all referenced kinds. All IDs use the
existing canonical-name grammar. The policy path and catalog paths resolve to
existing local files. Catalog paths are relative to the policy directory, or
absolute; no discovery, environment expansion, or network lookup occurs. Canonical
path duplicates, including aliases/symlinks, fail. JSON duplicate keys fail.

Each listed kind must exist. Roles are explicit: unprofiled, linear, absolute, or
difference. A kind may occupy only one role and at most one affine pair. Pair
members must be distinct and have identical Dim8 vectors. Origins have exactly
`id` and `absolute_kind`; IDs are unique and the kind must have an explicit
absolute role. Multiple origins for one absolute kind are allowed. Unprofiled
catalog kinds remain usable model identities without an inferred arithmetic role.
Catalog contract reference facets do not implicitly assign policy roles.

## Ownership, handles, and failures

Zero-initialize `FisicsPhysicalPolicy`, then call `fisics_physical_policy_load`.
Success replaces and frees any previous model. Every failure leaves the existing
model unchanged. `fisics_physical_policy_free` releases all owned storage and
zeros the model; repeated free is safe. Do not shallow-copy a live model and free
both copies. Concurrent independent models have independent ownership; this API
has no global mutable registry and is not yet attached to `CompilerContext`.

Kinds and origins receive canonical sorted, model-local 32-bit handles. Zero means
unknown, including for dimensionless kinds. IDs may differ between models with
different contents, so callers must resolve canonical names in the receiving
model. The catalog permits 65,536 kinds; a 16-bit nonzero handle cannot represent
that full range. No AST address or numeric runtime payload is part of the model.

`fisics_physical_policy_make_type` checks model references only. An absolute kind
requires a matching explicit origin. Linear, difference, and unprofiled kinds
accept origin zero and reject point origins. Invalid handles or origin mismatches
leave the destination type unchanged. This is not a C assignment/operation check.

Structured loader diagnostics use `4301` invalid policy/options, `4302` catalog,
`4303` role/origin reference, `4304` resource limit, `4305` owned allocation, and
`4306` input I/O. Existing diagnostic IDs retain their values. New explanations
are available through `fisics --explain 4301` through `4306`.

Production ceilings are 1 MiB per policy, 64 MiB per catalog, 512 MiB aggregate
policy/catalog bytes, 1,024 catalog paths, 65,536 authored quantity definitions,
262,144 contracts, and 65,536 origins. Quantity/contract counts apply across input
catalogs before identical-definition merging. Test options may lower these limits,
never raise or disable them. A per-load allocation threshold exercises allocations
owned by the catalog/model loader; existing JSON parser, unit registry, and libc
internal allocations are outside that injection mechanism.

## Fingerprints and receipts

Every input receipt hashes the exact bytes parsed and records their size. The
semantic policy fingerprint excludes paths, input order, and JSON formatting. It
includes the policy ID, unit-registry fingerprint, sorted quantity definitions,
explicit roles/pairs, and sorted origins. Per-kind fingerprints include canonical
ID, dimension system, all eight signed exponent bytes, and exact dimension text.
Catalog contracts are not arithmetic policy and are excluded from that semantic
fingerprint; their file bytes remain covered by the catalog input receipts.

Fingerprints use the repository's FNV-1a64 convention (initial value
1469598103934665603, multiplier 1099511628211), with domain-separated,
length-prefixed UTF-8 strings and little-endian 64-bit integers. These detect
accidental drift and are not cryptographic authentication or a new export schema.

## Verification

```sh
make physical-policy-contract-test
```

The stable gate builds `tests/unit/physical_policy_loader.c` against the frontend
archive and runs generated fixtures in a disposable temporary directory. It checks
strict parsing, definition conflicts, roles/origins, model replacement, independent
contexts, deterministic/reference fingerprints, input receipts, exact/lowered
limits, and the actual 65,536-kind handle boundary. Failed-reload controls compare
the prior model before freeing it; allocation sweeps reach every owned allocation
in representative scalar/vector/tensor and relative/cyclic contract loads.

For instrumentation, first build the frontend archive, then use a fresh scratch
folder for each command. Paths shown below are caller-owned scratch examples:

```sh
python3 tests/integration/build_physical_policy_probe.py --mode sanitized \
  --library libfisics_frontend.a --out /tmp/policy-asan-build
ASAN_OPTIONS=detect_leaks=0 python3 tests/integration/run_physical_policy_contracts.py \
  --probe /tmp/policy-asan-build/probe
python3 tests/integration/build_physical_policy_probe.py --mode ledger \
  --library libfisics_frontend.a --out /tmp/policy-ledger-build
python3 tests/integration/run_physical_policy_contracts.py \
  --probe /tmp/policy-ledger-build/probe
```

ASan/UBSan rebuild the catalog/model and operation-model implementation files; the existing
frontend archive supplies JSON and unit services. The test-only allocation ledger
checks that allocations owned by the selected model files are freed at process exit,
including failed replacements. This is not a sanitizer/leak qualification of the
entire compiler. LeakSanitizer is unavailable on this macOS host; its disabled
setting is explicit above. Remove successful caller-owned scratch builds after
review. `--out` for fixture results requires a fresh directory; without it the
runner cleans its temporary fixtures automatically.

P5.3 connects optional kinds to source declarations and bounded assignment,
argument, return, merge and linear-operation checks. See the physical source
reference for admitted routes and explicit unknown/trust behavior. P5.4 applies
explicit affine pairs/origins to scalar point/difference arithmetic without changing
this schema. P5.5 adds separate [physical coverage receipts and cross-TU checks](physical_coverage.md).
Joined solver acceptance and cost qualification are described in
[solver qualification](solver_source_qualification.md).


## Version2 operation rules

[Physical operation contracts](physical_operations.md) add an explicit v2 policy
with required `operation_rules`, identity-checked `fabs`/`fmax` primitives and v2
source receipts. Version1 schema, fingerprints and behavior remain unchanged.
An empty v2 rule array admits no helper inference. Model conflicts and failed
replacement preserve existing fail-closed and atomic-load behavior.


## Version 3 scalar operation rules

[P6.1.2 scalar contracts](physical_operations.md#version-3-finite-scalar-rules-p612)
add an explicit v3 policy and v3 source receipt with operation-model version 2.
Only declared ratio scaling and the finite translational duration operations are
admitted. V1/v2 behavior and identity remain; a v2 policy cannot declare products.
Ordered operands participate in validation, ambiguity rejection, lookup and v3
fingerprints. The existing atomic loader and resource limits apply.


## Version 4 mechanical rules

[P6.1.3 mechanical application](physical_operations.md#version-4-mechanical-application-rules-p613)
adds v4 policy/receipts and operation-model version3. It preserves v1–v3 identity
and behavior, and admits only four explicit scalar stiffness/coordinate contracts.
The ordered model uses `fisiCs.physical_mechanics.v1` and the policy uses the v4
identity domain. Loading/lookup/atomic ownership/resource limits remain shared.

## Version 5 representation model (P6.2)

[Physical representations](physical_representations.md) specify the exact opt-in
v5 root and descriptor/helper entries. Its scalar operation submodel retains
version3 (v4 mechanics), while a separate representation submodel version1
fingerprints precision, ordered storage mapping, shape, parity and declared helper
preconditions. The catalog/physical graph v1 schemas retain their descriptive
meaning; no graph intent becomes C layout proof. Existing policy v1–v4 hashes
and receipts retain their contracts.

## Version 6 frame model and declaration checkpoint

[Frame declarations](physical_frames.md) add four exact policy tables to v5, with
atomic bounded loading, canonical handles/fingerprints, and directed transform
signature checks. P6.3.3 now adds
[static frame/axis propagation](physical_frame_propagation.md) and internal
same-frame coverage qualification. [P6.3.4 rotation calls](physical_rotations.md)
check directed source/target contracts and retain implementation/basis trust.
[V6 receipts and cross-TU checks](physical_coverage.md#version-6-frame-evidence)
now serialize exact context facets and the separately fingerprinted frame model.
V1–v5 policy hashes and receipt behavior remain unchanged.

## Energy qualifier model

[Energy forms and declared transfers](physical_energy_forms.md) add opt-in v7
aggregation, conservative mixed results, exact directed helper contracts and
separate declared implementation trust. Dim8 and v1–v6 behavior remain intact.
Use `make physical-energy-contract-test`; solver adoption remains separate.
