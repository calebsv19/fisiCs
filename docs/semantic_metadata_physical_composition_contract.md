# Physical Composition Graph Contract

Status: implemented

Optional detached operation: [Frozen Offline Graph Inputs](frozen_graph_inputs.md)
captures units evidence, source bytes and catalogs, then regenerates this graph.
It preserves existing graph schemas and does not regenerate C metadata exports.

Physical manifest contract ID: `fisiCs.physical_project_manifest`

Units evidence contract ID: `fisiCs.units_metadata`

Physical catalog contract ID: `fisiCs.physical_contract_catalog`

Physical graph contract ID: `fisiCs.physical_project_graph`

Schema version: `1`

Inputs: `fisiCs.semantic_project_graph` schema `1` and current-source units
evidence produced by the same compiler line

Last audited against source: 2026-09-02

This document defines the general physical-semantics composition boundary for
`fisiCs`. It uses a reusable physical quantity model that can later
serve hardware contracts, simulation models, IDEs, and agents without making
any of those consumers part of the compiler.

The physical composition graph performs static contract composition. It says whether declared physical meaning
and compiler-known units evidence are internally compatible. It does not say
that a sensor is present, calibrated, connected, sampled, or correct, and it
does not execute a simulation.

## 1. Why The Physical Composition Graph Is More Than A Unit Check

A dimension vector answers algebraic compatibility, but it does not completely
describe a physical value:

- energy and torque share the same SI dimension but have different meanings;
- absolute temperature and temperature difference need different affine
  treatment;
- a velocity vector needs a reference frame while speed does not;
- radians and ratios are both dimensionless but are not interchangeable in
  every semantic context; and
- a voltage value can be expressed in several convertible units without
  changing the quantity being represented.

The physical composition graph therefore keeps six facets orthogonal:

```text
subject identity
    what semantic entity or source declaration the contract describes

quantity kind
    the physical meaning, independent of its dimension

dimension
    algebraic SI-exponent compatibility

unit
    the numeric encoding and conversion family

value form
    scalar, vector, tensor, or opaque representation intent

reference semantics and frame
    how the value is anchored and, when spatial, where it is expressed
```

No facet may be inferred from another. In particular, the graph never infers a
quantity kind from a dimension, a device class from a target name, or a
coordinate frame from a variable name.

## 2. Ownership And Layering

The physical composition graph composes three separately owned inputs:

```text
semantic project graph
    owns semantic records, canonical authored IDs, source identities,
    produces/consumes edges, and allowed external target identity

physics-units evidence
    owns Dim8 resolution, concrete unit identity, unit family,
    registry scale/offset data, and units diagnostics

physical contract catalogs
    own quantity-kind definitions, expected units policy, value form,
    reference semantics, and optional frame identity

physical composition graph
    owns only deterministic joins, compatibility results, coverage,
    conversion recipes, provenance, and explicit unknown states
```

The physics-units implementation remains the sole authority for dimension
parsing, dimension algebra, the concrete-unit registry, and conversions. The graph
must query or serialize that authority; it must not introduce a second unit
table or duplicate conversion math.

Hardware component catalogs own devices, boards, buses, pins,
addresses, drivers, and capabilities. Simulation systems remain owners of
geometry, materials, dynamics, solvers, state evolution, and calibration.
Runtime/telemetry systems remain owners of observations, timestamps,
uncertainty, health, and physical acceptance.

## 3. Hard Non-Goals

The physical composition graph v1 must not:

- change core C parsing, type compatibility, object generation, or runtime
  behavior;
- add unit literals or implicit conversions;
- infer physical meaning from `domain`, `role`, `produces`, `consumes`, C names,
  or canonical target prefixes;
- treat a catalog statement as measured or physically verified truth;
- validate function return/parameter ports without an explicit future carrier
  binding contract;
- merge C linkage or reconstruct compiler semantic models from JSON;
- define devices, pins, buses, addresses, drivers, sample rates, telemetry,
  calibration curves, geometry, material models, or solver behavior;
- generate drivers, firmware, simulation code, or deployment configuration; or
- alter the translation-unit metadata export, semantic project graph, build-manifest v0, or legacy frontend ABI schemas.

## 4. Identity Model

The physical composition graph preserves four different identities:

| Identity | Meaning |
| --- | --- |
| semantic authored ID | project-global canonical identity from semantic project graph |
| semantic project graph source identity | deterministic fallback for a record without an authored ID |
| source binding identity | deterministic identity of one units-decorated declaration |
| physical contract ID | canonical identity of one catalog contract |

A physical contract subject is an exact tagged union:

```json
{"kind":"semantic_id","id":"signal.cabin.temperature"}
```

or:

```json
{"kind":"source_identity","id":"symbol:function:/canonical/path.c:12:1:read_value"}
```

`semantic_id` means canonical semantic identity only. It does not encode a
category, device type, physical kind, or ownership hierarchy. `source_identity`
allows a source-defined record to participate without forcing an authored
physics ID, but it remains source-location-sensitive and is not promoted to a
project-global semantic name.

Units attachments also receive a deterministic `binding_id` derived from the
canonical source receipt, declaration kind, exact source range, symbol name,
and stable symbol ID when available. Bindings use source identity, never an AST
pointer, allocation order, or hash-table order.

## 5. General Physical Type

Every catalog contract contains this conceptual type:

```text
PhysicalType = {
    quantity_kind,
    dimension_system,
    dimension_vector,
    unit_policy,
    value_form,
    reference_semantics,
    frame_id?
}
```

### 5.1 Quantity kind

`quantity_kind` is a canonical dotted lowercase identity such as:

```text
thermodynamics.temperature
thermodynamics.temperature_difference
mechanics.energy
mechanics.torque
mechanics.speed
mechanics.velocity
electromagnetism.voltage
fluid.pressure
```

These examples are vocabulary, not string-prefix semantics. A quantity kind is
valid only when its catalog definition supplies an exact dimension. Catalogs
may define project- or domain-specific kinds, so the compiler is not limited to
one closed list. Duplicate definitions of one quantity-kind ID must have
identical canonical fields after parsing or the project fails.

The project may later publish a reusable standard catalog, but physical composition graph correctness
cannot depend on English aliases or name-based inference.

### 5.2 Dimension system

The physical composition graph v1 supports the existing units engine through the explicit identity:

```text
fisiCs.si_dim8.v1
```

Its vector order is:

```text
[length, mass, time, current, temperature, amount, luminous, custom]
```

Every exponent is a signed 8-bit integer and the vector is serialized as eight
JSON integers. The existing custom slot is one independent extension axis; it
must not be presented as an unlimited set of custom dimensions. A future need
for additional independent bases requires a new dimension-system identity and
an explicit compatibility design, not reinterpretation of this vector.

The dimension vector is normative. Human-readable dimension text is display
metadata and must round-trip to the same vector through the units parser.

### 5.3 Unit policy

Each physical contract selects exactly one policy:

| Mode | Requirement |
| --- | --- |
| `dimension_only` | resolved source dimension must match; concrete unit may be absent |
| `exact_unit` | dimension, family, and canonical unit identity must match exactly |
| `convertible_unit` | resolved source unit must be in the same conversion-supported family; an explicit conversion recipe is emitted |

`exact_unit` and `convertible_unit` name one canonical registry unit.
`convertible_unit` authorizes a static recipe only; it does not insert code or
convert runtime values.

Unit registry numbers are serialized as exact binary64 hexadecimal strings,
not locale-dependent decimal text. The normative affine form is:

```text
canonical_value = source_value * scale_to_canonical + offset_to_canonical
```

For `difference` reference semantics, the offset is ignored and only scale is
used. The units registry remains authoritative for whether a family supports
conversion. Non-finite, zero-scale, incompatible-family, or unknown-unit data
is rejected before graph publication.

Each recipe names its `conversion_model`. physical composition graph v1 accepts `linear` and
`affine`, matching the existing scale/offset registry representation.
Logarithmic, reciprocal, piecewise, table-driven, or context-dependent
conversions are not approximated as affine; they require a later named model
and contract revision.

Every units artifact carries a deterministic unit-registry fingerprint. physical composition graph
rejects artifacts produced against a different registry snapshot rather than
quietly composing different conversion definitions.

### 5.4 Value form

The v1 catalog can declare:

| Form | Meaning |
| --- | --- |
| `scalar` | one physical magnitude |
| `vector` | ordered components of one quantity kind |
| `tensor` | ranked multidimensional physical value |
| `opaque` | representation deliberately owned by a later consumer |

Vector component count and tensor rank/shape are positive bounded integers
when present. In v1 these fields are declared intent, not proof that a C struct
or array has the requested layout. C-representation validation requires a
separate carrier-binding contract. Vector and tensor forms are homogeneous:
all components share one physical type. A heterogeneous object such as a pose
or complete sensor packet must be represented as several contracts or remain
`opaque` until a typed aggregate-composition contract exists.

### 5.5 Reference semantics

The v1 values are:

| Value | Meaning |
| --- | --- |
| `linear` | ordinary linear magnitude with no affine origin |
| `absolute` | value measured from a defined origin; unit offsets apply |
| `difference` | interval/delta; unit offsets never apply |
| `relative` | value relative to another explicitly named subject |
| `cyclic` | value wraps at an explicitly declared positive period |

`relative` requires `reference_subject`. `cyclic` requires a positive finite
period expressed as an exact binary64 hexadecimal string in the contract unit.
Reference subjects must resolve to a semantic project graph authored identity or allowed
external target. These declarations make conversion meaning explicit but do
not prove that runtime code respects the origin or wrapping policy.

### 5.6 Reference frame

`frame_id` is nullable. It is a canonical semantic identity naming a coordinate
frame, for example `frame.van.body` or `frame.world.local`. It must resolve to
a semantic project graph authored identity or allowed external target. Vector/tensor contracts
may require it; scalar contracts normally omit it. physical composition graph compares frame identity
exactly and does not infer or calculate transforms. Frame geometry and
transforms belong to later simulation or hardware catalogs.

## 6. Units Evidence Artifact

The physical composition graph adds a dedicated per-translation-unit artifact rather than changing translation-unit metadata export:

```text
--emit-units-metadata-json <path>
```

The option uses the current frontend units projection and writes
`fisiCs.units_metadata` schema `1`. It is permitted only for exactly one source
input. It has no code-generation effect and uses the same atomic-write and
destination-preservation contract as translation-unit metadata export.

The top-level fields are normative:

```json
{
  "schema": "fisiCs.units_metadata",
  "schema_version": 1,
  "producer": {"name": "fisiCs", "version": "<version>"},
  "analysis_contract": {"id": "fisiCs.analysis.contract", "version": "<version>"},
  "source": {
    "path": "/canonical/source.c",
    "hash_fnv1a64": "0000000000000000",
    "length_bytes": 0
  },
  "dimension_system": "fisiCs.si_dim8.v1",
  "unit_registry": {"id": "fisiCs.unit_registry", "fingerprint_fnv1a64": "0000000000000000"},
  "overlay_enabled": true,
  "complete": true,
  "attachments": []
}
```

Each attachment contains deterministic binding identity, symbol identity and
kind when available, exact source range, dimension vector/text, and optional
canonical unit name/symbol/family plus registry scale and offset strings.
Unresolved annotations remain compiler diagnostics and make the artifact
`complete: false`; an incomplete artifact is never accepted by physical composition graph.

All resolved declaration attachments are retained, including attachments that
cannot yet join a semantic project graph record. They appear as unbound units evidence rather
than being discarded. Expression-result units remain outside the v1 artifact.

## 7. Physical Contract Catalog

`fisiCs.physical_contract_catalog` schema `1` is a strict, declarative input.
It contains:

```json
{
  "schema": "fisiCs.physical_contract_catalog",
  "schema_version": 1,
  "catalog_id": "van.physical_contracts",
  "quantity_kinds": [],
  "contracts": []
}
```

Every quantity-kind definition has `id`, `dimension_system`,
`dimension_vector`, and canonical display text. Every contract has
`contract_id`, tagged `subject`, `quantity_kind`, `unit_policy`, `value_form`,
`reference_semantics`, and nullable `frame_id`.

Catalog order is not semantic. Duplicate catalog paths, contract IDs, subject
assignments, conflicting quantity definitions, unknown fields, duplicate JSON
keys, and noncanonical identities are rejected. A subject has at most one
physical contract in v1.

A catalog is an authored declaration. It is not compiler evidence, hardware
inventory, calibration evidence, or observation.

## 8. Physical Project Manifest And CLI

The physical composition graph has a dedicated terminal offline mode:

```text
--physical-project-manifest <manifest-path> \
--emit-physical-project-graph-json <output-path>
```

The paired options are required exactly once, accept no stdin path, and cannot
be combined with compilation, linking, build manifests, translation-unit metadata export emission, semantic project graph
assembly, dumps, release operations, or other output modes.

The manifest shape is:

```json
{
  "schema": "fisiCs.physical_project_manifest",
  "schema_version": 1,
  "project_id": "van.control",
  "semantic_project_graph_path": "semantic-project-graph.json",
  "units_metadata_paths": ["controller.units.json", "sensor.units.json"],
  "physical_catalog_paths": ["physical-contracts.json"],
  "coverage_policy": "allow_partial",
  "source_evidence_policy": "report_unverified"
}
```

Paths resolve relative to the manifest directory and are canonicalized before
duplicate and output-alias checks. Wildcard discovery, directory scanning,
network retrieval, and implicit adjacent-file lookup are forbidden.

Coverage policies are:

| Policy | Required catalog coverage |
| --- | --- |
| `allow_partial` | uncovered semantic records and targets are retained and counted |
| `require_relationship_targets` | every `produces`/`consumes` target has one contract |
| `require_all_authored_records` | every authored semantic project graph record and relationship target has one contract |

Source evidence policies are:

| Policy | Missing source units evidence |
| --- | --- |
| `report_unverified` | emit an explicit unverified result |
| `require_for_source_contracts` | fail when a source-defined contract lacks matching resolved units evidence |

External targets are catalog-declared and are never required to have source
evidence. Every semantic project graph translation unit must have exactly one fresh complete
units receipt so that “no attachment” is distinguishable from “artifact not
provided.”

## 9. Join And Compatibility Rules

Processing order is normative:

1. Strictly read and validate the semantic project graph.
2. Strictly read all units receipts and verify current source path, length, and
   hash.
3. Require one units receipt for every semantic project graph translation unit and reject extra
   source receipts.
4. Require one dimension-system identity and one unit-registry fingerprint
   across all receipts and the current compiler.
5. Strictly read, merge, and canonicalize catalog definitions.
6. Join units attachments to semantic project graph records using stable symbol ID plus exact
   source/name/range confirmation. Never join by name alone.
7. Resolve each catalog subject against one project authored ID, project source
   identity, or allowed external target.
8. Evaluate dimension, quantity, unit, reference, value-form, and frame facets.
9. Apply manifest coverage/evidence policy.
10. Serialize only the fully validated owned physical graph.

Dimension mismatch, unknown unit, unsupported conversion, contradictory
quantity-kind definition, duplicate subject contract, invalid reference, or
ambiguous binding is a terminal incompatibility. No destination is published.

Absent evidence is not incompatibility. It is represented explicitly unless
the selected evidence policy makes it terminal.

A semantic project graph edge may reference a physical contract through its target subject, but
that association alone does not prove that the producing function return or
consuming function parameter carries the quantity. physical composition graph v1 records the
association as `target_contract_index` and leaves callable port validation for
a later explicit carrier-binding design.

## 10. Truth And Evidence Vocabulary

The physical composition graph must never emit a generic `verified: true` field. It uses scoped states:

| State | Exact meaning |
| --- | --- |
| `source_compatible` | compiler units evidence satisfies the static catalog contract |
| `source_unverified` | source record exists but matching resolved units evidence is absent |
| `external_declared` | catalog describes an allowed external semantic target |
| `uncovered` | no physical contract was supplied under a policy that permits it |

Incompatibility is a failure, not a partially published graph state.

Every result also identifies evidence origin:

```text
compiler_units_attachment
catalog_declaration
none
```

The output top level permanently carries:

```json
{"physical_acceptance":"not_assessed"}
```

No physical composition graph input can change that value. Only a future runtime/physical acceptance
system may publish a separate observation or acceptance artifact.

## 11. Physical Project Graph

The output is `fisiCs.physical_project_graph` schema `1`. Its normative
top-level key order is:

```text
schema
schema_version
producer
project
complete
physical_acceptance
dimension_system
unit_registry
coverage
semantic_graph_receipt
source_receipts
quantity_kinds
units_bindings
physical_contracts
semantic_edges
unbound_units_evidence
```

The graph embeds receipts and resolved indices, not raw input JSON. Canonical
sort order is:

1. source receipts by source path, source hash, then artifact path;
2. quantity kinds by quantity-kind ID;
3. units bindings by source path, range, declaration kind, symbol name, then
   stable ID;
4. physical contracts by subject kind, subject ID, then contract ID;
5. semantic edges by their frozen semantic project graph order; and
6. unbound evidence by units-binding order.

Serializer output is UTF-8, two-space-indented JSON with LF endings, no
trailing whitespace, and exactly one final newline. Two identical semantic
inputs must produce byte-identical output independent of manifest/catalog/input
array order.

## 12. Limits And Publication Safety

The initial implementation freezes conservative ceilings before allocation:

| Resource | Maximum |
| --- | ---: |
| manifest bytes | 1 MiB |
| one catalog or units artifact | 64 MiB |
| all input bytes | 512 MiB |
| units receipts | 4096 |
| catalogs | 1024 |
| quantity kinds | 65536 |
| physical contracts | 262144 |
| units bindings | 262144 |
| semantic edge annotations | 262144 |
| JSON nesting depth | 64 |

All count arithmetic must be overflow-checked. JSON objects reject duplicate
keys. Unknown fields are rejected in v1. Strings must be valid UTF-8 and may
not decode embedded NUL.

Publication uses a temporary file in the destination directory, successful
write/flush/fsync/close, then atomic rename. Every failure removes the
temporary file and preserves an existing destination. The output path may not
alias the physical manifest, semantic graph, units artifact, catalog, or any
analyzed source.

## 13. Acceptance Matrix

The physical composition graph implementation is complete only when focused tests prove:

| Case | Required outcome |
| --- | --- |
| Same quantity, dimension, and exact unit | `source_compatible` |
| Same dimension, convertible unit | compatible plus deterministic conversion recipe |
| Dimension-only contract, no concrete unit | `source_compatible` |
| Energy versus torque with same dimension | distinct quantity kinds remain distinct |
| Absolute versus difference temperature | reference policy remains distinct; affine offset handling applies when the registry supplies a nonzero offset |
| Vector contract with frame | frame retained exactly; no transform inferred |
| Source record without authored ID | exact semantic project graph source identity can bind it |
| Units attachment without semantic project graph record | retained as unbound evidence |
| Missing source attachment | `source_unverified` or failure according to policy |
| External target with catalog | `external_declared`, never source-verified |
| Missing required target contract | failure under strict coverage policy |
| Dimension mismatch | nonzero exit; destination preserved |
| Unknown/incompatible unit | nonzero exit; destination preserved |
| Registry fingerprint mismatch | reject before composition |
| Missing, stale, duplicate, or extra source receipt | reject before composition |
| Duplicate/conflicting quantity or subject | reject with both origins |
| Self-edge/cycle | retained without scheduling meaning |
| Permuted inputs/catalog arrays | byte-identical output |
| Two identical runs | byte-identical output |
| Allocation/write/replacement failure | old destination preserved; no temp residue |
| Output aliases any input/source | reject before mutation |
| Physical mode plus compile/build/semantic project graph mode | reject before compiler initialization |
| Ordinary compile and translation-unit metadata export/semantic project graph artifacts | unchanged bytes and behavior |
| Full existing test gate | unchanged and green |

## 14. Implementation Record

The physical composition graph must proceed in this order:

1. **Vocabulary and fixture lock.** Freeze representative scalar,
   vector, same-dimension/different-kind, absolute/difference, external,
   fallback-source, unbound, and negative fixtures plus exact golden output.
2. **Units evidence artifact.** Implement deterministic
   `fisiCs.units_metadata` emission from the existing frontend units view,
   including source receipt and registry fingerprint. Do not change units
   algebra or the legacy frontend result.
3. **Catalog and manifest readers.** Add strict owned models, limits,
   canonical path handling, duplicate/unknown-field rejection, and complete
   destruction paths.
4. **Provenance and binding index.** Verify source bytes, TU coverage,
   registry identity, deterministic binding IDs, stable-ID/range joins, and
   unbound evidence retention. Stop before compatibility evaluation.
5. **Physical compatibility engine.** Validate quantity definitions,
   dimensions, unit policies, reference semantics, value form, and frame
   identity through read-only units authority. Produce scoped evidence states
   and deterministic conversion recipes.
6. **Physical graph serializer.** Emit only validated owned state with
   normative order, exact binary64 strings, receipts, coverage summary, and
   atomic destination preservation.
7. **Terminal CLI integration.** Add only the exact paired physical
   graph options and reject all incompatible modes before normal compiler
   initialization.
8. **Focused and broad proof.** Complete the acceptance matrix,
   sanitized failure injection, translation-unit metadata export/semantic project graph parity, ordinary-object identity,
   build-manifest regression, and the repository confidence tier justified by
   the touched code.
9. **Documentation closeout.** Mark physical composition graph implemented only from passing
   evidence and preserve hardware, simulation, runtime, deployment, and
   physical acceptance boundaries.

Stop and return to contract review if implementation requires implicit
conversions, expression-result export, callable port inference, C-layout
claims, another dimension algebra, dynamic observations, hardware vocabulary,
or simulation behavior.

## 15. Implementation Evidence

The physical composition graph is implemented. Its permanent focused gates are:

```bash
make integration-cli-units-metadata
make integration-physical-project-graph
```

The units gate proves deterministic single-TU evidence, stable exported-symbol
identity, retained local evidence, object-byte parity with and without the
sidecar, strict CLI gating, source/output collision rejection, and atomic
destination preservation. The physical graph gate proves the frozen positive
composition corpus, a normalized-byte golden, repeatability and permutation
invariance, coverage/evidence policies, scoped truth states, exact conversion
recipes, stale/duplicate/extra receipt rejection, conflict diagnostics, output
alias rejection, and failure preservation.

The focused gates pass in sanitized and unsanitized builds. The inherited translation-unit metadata export
and semantic project graph focused gates also pass after the fallback `file_variable` identity
spelling was aligned with the already-frozen reader contract. The sanitizer
pass found and closed two ownership defects before completion: stale AST file
pointer use in units projection and zero-length `realloc` ownership during
empty-receipt merging.

The current unit registry intentionally remains multiplicative-only. The physical composition graph
preserves `absolute` and `difference` reference semantics and implements the
normative offset formula, but the focused public fixture uses kelvin and does
not add Celsius/Fahrenheit or otherwise widen the units registry. A future
affine-registry extension must prove nonzero-offset examples before claiming them
as supported concrete units.

## 16. Completion Boundary

The physical composition graph is complete when one deterministic artifact can answer:

```text
Which semantic subjects have declared physical meaning?
Which current-source declarations provide units evidence for them?
Are those declarations statically compatible with the declared quantity,
dimension, unit, reference, form, and frame contract?
Which subjects or attachments remain explicitly unverified or unbound?
```

That is the shared compiler-to-physical-world foundation. The implemented
hardware compatibility graph adds supplied static device and topology facts to
it. A simulation consumer can
map the same quantity contracts into models and state variables. Neither needs
to redefine units, quantity identity, evidence status, or provenance.

Completion still does not establish that hardware exists or works, that a model
is accurate, that telemetry is fresh, or that any physical outcome has been
accepted.
