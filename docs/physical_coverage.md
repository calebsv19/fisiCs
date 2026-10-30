# Selected physical coverage and cross-TU receipts

P5.5 adds a separate physical report and opt-in selected-function strict policy.
It consumes the physical source checks and the explicit policy/model from
[physical semantics](physical_semantics.md). Existing units coverage v1, semantic
metadata and graph schemas retain their meanings and fields.

## Report or enforce one function

```sh
./fisics --overlay=physics-semantics \
  --physical-semantics-policy physical.policy.json \
  --physical-semantics-function update \
  --physical-semantics-strict \
  --emit-physical-semantics-json update.physical.json \
  -I include solver.c
```

Omit `--physical-semantics-strict` to collect a report without rejecting otherwise
valid C solely for incomplete physical coverage. The report still records the
selected policy assessment. Physical and dimensional contradictions remain errors.
Both modes perform no code generation. They require one exact source-defined
function, one direct C input, internal preprocessing, the explicit physical overlay
and policy, and one filesystem JSON destination. Forced includes, other output/
terminal modes, object inputs, multi-source compilation and build manifests are
refused in this mode. Public frontend v1 structs remain unchanged.

Strict acceptance requires all of the following:

- Successful C, units and physical semantic analysis.
- Complete AST enumeration and captured source dependencies.
- Valid ordinary-control roles, without physical annotations or contradictions on
  rows excluded as ordinary C controls.
- Nonempty required obligations and at least one checked physical obligation.
- Every required obligation checked or covered by its explicit trust permission.

Unknown, unsupported and contradictory inputs cannot be waived. A trusted
interface cannot infer an absent kind or repair a raw pointer restoration. Generic
products, division, point unary signs and other unimplemented physical operations
remain unknown even when their dimensions are valid.

## Stable obligations, controls and trust

Physical collection reuses P3's `(site, boundary, slot)` AST enumeration for
initializers, assignments, arguments, returns and operations. It reads physical
facts and decisions independently. Removing kind or dimension metadata retains
the obligation rows; the missing facts block strict acceptance. Units checked
counts never become physical checked counts.

Narrow ordinary C roles reuse P3 loop counters, pointer/boolean predicates and
literal boolean status returns, using an internal structural-role collection entry
point. Physical collection derives these roles from C syntax/types independently
of quantity metadata. The original units caller retains its existing role policy.
A `_Bool` result returning a comparison/logical expression is also a C control;
the expression's physical operand obligations remain required.

Physical annotations or contradictions on an ordinary-control row set
`selection.ordinary_controls_valid=false` and reject strict. They do not reclassify
the row or change the denominator. Thus metadata removal from a loop counter or
boolean status result cannot change the control counts. This rule is deliberately
conservative: authored physical quantities used in an excluded control role must
be restructured into explicitly checked physical operations. Control rows never
infer `quantity.ratio` or count as checked physical work. A function containing
only controls cannot pass strict.

Two independent permissions apply only with strict mode:

```sh
--physical-semantics-allow-literal-assumptions
--physical-semantics-allow-declared-boundaries
```

Literal permission admits an authored literal-only initializer/plain write/return,
including its pure literal expression tree, as a representation assumption. A
literal inside mixed physical arithmetic does not gain the other operand's kind.
Declared-boundary permission admits a declared numeric call result with valid
physical metadata; it does not check the callee's implementation. Dependency
tracking retains both trust requirements when an expression uses both categories.
Trusted rows stay separate from checked rows.

The narrow P3 source-defined `void`/identical-record-pointer interface can use
interface trust only when every record field also has a valid physical contract.
Only directly authored identifier arguments are admitted, with the existing raw
cast/alias checks. Missing field metadata and raw pointer restoration remain
blocked. This classification checks the interface, not the body or runtime data.

The assessment identities are:

```text
total = required + ordinary_controls
required = checked + permitted_trust + blocked
```

Physical strict failure reports diagnostic4319. Use `--explain 4319` and the
receipt's obligation reasons and loss locations. Diagnostic collection or output
failure never publishes a successful receipt. A missing function or a traversal/
allocation failure can prevent publication entirely.

## Receipt schema and provenance

The new `fisiCs.physical_semantics` schema version1 contains:

- Compiler contract version, analysis validity, dimension-system and registry
  fingerprint, explicit policy identity/fingerprint, and policy/catalog byte receipts.
- Complete sorted canonical file receipts from the actual internal-preprocessor
  resolver buffers, including empty and guard-only headers. Source bytes are
  captured before the resolver is destroyed. They are not inferred from AST ranges.
- Relevant analysis options, explicit defines and include paths, effective resolver
  search paths, and the canonical main source path.
- Translation-unit declaration bindings with source locations, and a sorted list
  of externally visible variable/function signatures using canonical kind, reference
  role, origin, definition fingerprint, dimension, concrete unit and array-carrier
  facts. Context-local handles and AST addresses are not serialized.
- The selected function's complete/accepted flags, strict settings and counts,
  expression dependency IDs, declaration origins, reasons, trust masks, loss
  locations and obligation results.

Reference roles are versioned model values:0 unprofiled,1 linear,2 absolute,
3 difference. A null kind is unknown, distinct from a declared unprofiled kind or
a dimensionless quantity. Definition and policy fingerprints are canonical model
identities; file receipts use the repository's FNV-1a64 convention and byte count.
These local receipts are consistency evidence, not authenticated signatures.

A read-set closure is not a hermetic build capsule. It records the files actually
loaded and the analysis options, not failed include-search candidates, environment
replay or files that a future filesystem layout might select instead. Regenerate
receipts after changing source, dependencies or configuration. A successful selected
report is local AST compatibility evidence, not path-sensitive value provenance or
whole-program body verification.

Publication checks current bytes against captured source/policy/catalog receipts,
refuses source/header/policy/catalog aliases, and writes through a sibling temporary
file plus flush/fsync/rename. Failed publication preserves the old destination.
Capture is opt-in for this report path; ordinary disabled-overlay compilation does
not allocate the physical source receipt tables.

Limits include4096 captured files,64 MiB per file/report and512 MiB source bytes.
Existing catalog/model limits remain in force. AST enumeration remains bounded by
P3 limits; receipt publication refuses more than131072 stored physical entries.

## Compare declarations across translation units

Export a receipt separately for each translation unit, then run:

```sh
./fisics --physical-semantics-check a.physical.json b.physical.json
```

This dedicated terminal command accepts2–64 receipts and no compile/output options.
It validates the schema, successful analysis, complete current source read sets,
policy/catalog freshness, current registry, loaded model/definition fingerprints
and relevant target/analysis settings. At least one shared physical signature slot
is required. Duplicate main-source inputs are refused.

Externally named variables/functions shared across receipts must have identical
physical signature facts, parameter counts, prototype/variadic flags, concrete
units and scalar/array carriers. Under v1–v4, only primitive scalar and authored homogeneous
numeric-array signature slots are admitted. Shared pointer, record, callback,
complex and named-alias slots explicitly refuse cross-TU admission; their nested
contracts are not flattened into an unknown scalar. Local source checks and
selected interface trust remain separately available for supported C routes. Conflicting or missing kinds and different point
origins reject. Model policy identity/fingerprint must agree, so the same canonical
ID cannot silently acquire a different role or definition. Static declarations are
local and excluded. Sorted signatures use a linear merge for each receipt pair;
input order does not change acceptance.

Per-TU defines and include directories are retained independently; they need not be
identical across source files. The command compares their resulting physical
contracts. It does not link objects, check general C ABI compatibility, resolve all
symbols, or require every selected body to have passed strict. A declaration
consistency pass is separate from each receipt's selected-body assessment. Unknown
or unsupported physical declarations cannot provide a valid shared physical slot.

This receipt is not consumed by existing physical/hardware graph v1 readers.
A future downstream adapter requires explicit versioned admission. Multi-source
physical compilation remains refused; cross-TU declaration checking uses the
separate receipt command above.

## Qualification

```sh
make physical-coverage-contract-test
make physical-semantics-contract-test
make physical-affine-contract-test
make physical-policy-contract-test
make frontend-contract-test
```

The P5.5 gate checks strict/report behavior, stable metadata-removal denominators,
separate and combined trust requirements, raw-restoration controls, dependency
capture/freshness, deterministic output, publication and allocation failures,
canonical signature conflicts and both input orders. The existing probe builder's
`--cli` mode includes the new physical coverage, provenance and receipt modules for
scoped ASan/UBSan or owned-allocation checks. Parser/JSON/LLVM internals are outside
that sanitizer/ledger claim; macOS LeakSanitizer remains unavailable.

[P5.6 solver qualification](solver_source_qualification.md#physical-semantic-solver-acceptance-p56)
adds actual-source numerical/layout checks, carrier projections, mixed-toolchain
ABI controls and matched ordinary/units/physical compiler costs. Full product
kernels remain strict refusals. P6 retains
general operation, frame/shape and energy-form rules. These compiler checks do not
establish simulation accuracy, conservation, hardware behavior or physical acceptance.


## Operation-model receipts (P6.1.1)

A [v2 physical policy](physical_operations.md) emits receipt version2 with an
`operation_model` and nullable per-expression `operation_rule` identities.
The v1 format and behavior above remain unchanged for v1 policies. Standard
`fabs`/`fmax` inference is checked against explicit input-derived rules; literal
and interface trust remain separate and propagate through nested calls. Receipt
checking validates the loaded operation model without widening pointer/record
signature admission or proving callee implementations.

The owning gate is `make physical-operation-contract-test`. Virtual generic math
header source closure and shared unsupported math-header signatures retain their
explicit refusals; see the operation contract for the exact limits.


P6.1.2 v3 policies emit receipt version 3 and operation-model version 2 for
explicit ratio/time rules. The checker accepts versions 1, 2 and 3 only with their
matching policy and exact model definitions. Scalar compound assignments retain
input trust dependencies. `make physical-scalar-contract-test` owns the new finite
rule, trust, denominator, cross-TU and numerical/layout controls.


V4 mechanical policies emit receipt version4 with operation-model version3.
The v4 checkpoint accepts versions1–4 with their matching live policy/model;
the subsequent v5 representation contract adds its own exact model and fields.
`make physical-mechanical-contract-test` checks the copied application/recovery
helper obligations, stable swapped/collapsed-field denominators, explicit literal
permissions, receipts and independent numerical/layout parity. This is bounded
helper acceptance, not whole-solver provenance or a frame/matrix representation proof.

## Version 5 carrier evidence (P6.2)

[Representation policies](physical_representations.md) produce receipt version5.
The root adds `representation_model` (version1, fingerprint, sorted representation
and helper identities/fingerprints). Facts add `representation`,
`representation_fingerprint`, `component_depth` and `frames: "unassessed"`.
Exported slots add `c_scalar` and `c_record_tag`; exported declarations add
nullable `shape_helper`. The checker reloads the exact current model, validates
these fields and helper slots, and compares ordered C/physical signatures. It
continues to refuse unqualified pointers, callbacks and other record routes.

The original AST denominator remains stable under annotation removal. Fully
validated representation aggregate sites may discharge existing unsupported-site
rows; they are never deleted. Other unsupported sites still block completeness.
Checked component/shape compatibility, literal assumptions, declared helper/basis
trust and unknowns remain separate. A helper can require declared-boundary
permission and still cannot make a trust-only scope pass. No frame proof or
implementation authentication follows from a fresh receipt.

Run `make physical-representation-contract-test` for storage, helper, strict,
model allocation/canonicalization and cross-TU freshness/failure checks.


## Version 6 frame evidence

V6 source receipts are now supported for the bounded [frame](physical_frames.md),
[rotation](physical_rotations.md) and [point](physical_points.md) families.
The exact root is v5's sixteen fields plus `frame_model`: seventeen fields.
The frame model has exactly `version`, `fingerprint`, `frame_contexts`,
`reference_points`, `frames`, `frame_transforms`; entries contain sorted
`id`/`fingerprint` pairs checked against the freshly loaded canonical policy.

A v6 fact retains v5's fields except `frames`, replaced by `physical_context`.
That object has exactly seven nullable canonical IDs: `frame`, `component_axis`,
`applied_at`, `displacement_from`, `displacement_to`, `moment_reference`,
`point_subject`, plus `facets`. Compiler-local handles are never exported.
The five facets are:

| Facet | Values |
| --- | --- |
| frame_identity | declared, unknown, contradictory, not_applicable |
| compatibility | checked, unknown, contradictory, not_applicable |
| geometry / observer_state | declared, unassessed, not_applicable |
| references | declared, checked_compatible, unknown, contradictory, not_applicable |

A missing required vector frame is unknown, not not-applicable. Unframed scalar
magnitudes and ordinary controls have no coordinate requirement; an admitted dot
result can have invariant scalar output while its dependencies retain the checked
input frames. Geometry and observer/state premises never become compiler-proved
runtime facts. Literal assumptions and declared helper/basis trust remain explicit.

Shared declarations retain v5's concrete C scalar/tag and shape helper fields,
with a nullable `frame_transform` identity added for v6. The checker compares
ordered signatures including frame, axis, origin, physical references and
transform direction, after validating current source/policy/catalog read sets,
all three model fingerprints and local definition identities. Missing, stale,
conflicting, unknown-model and schema-mixed inputs refuse. Selection counters,
fact/context schemas and required/checked row counts must agree. A v5
frames-unassessed artifact is never v6 evidence.

Strict scopes retain the existing AST denominator under metadata removal and
require nonzero checked work and no blocked obligations. A checked transform
argument is real compatibility work; a scope containing only an assumed result
and no checked input remains rejected. Trust permission cannot waive missing
frames/references or contradictions. Raw pointers, callbacks, unsupported records
and existing unsupported expression/control-flow paths retain their refusals.

Receipts are unsigned local evidence. Freshness, schema and shared-declaration
consistency checks do not authenticate arbitrary callee bodies or protect against
a coordinated, self-consistent forgery of all evidence. They do not replace
runtime failure/basis validation or physical acceptance. Atomic output and alias/
stale-input protection remain in force. V1–v5 schemas/hashes and behavior remain
available unchanged. Use `make physical-frame-completion-contract-test`.

## Energy qualifier model

[Energy forms and declared transfers](physical_energy_forms.md) add opt-in v7
aggregation, conservative mixed results, exact directed helper contracts and
separate declared implementation trust. Dim8 and v1–v6 behavior remain intact.
Use `make physical-energy-contract-test`; solver adoption remains separate.
