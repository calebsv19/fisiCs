# Semantic Metadata Historical Implementation Record

Status: implementation complete; retained as maintainer history

Depends on: `docs/semantic_metadata_contract.md`

Last audited against source: 2026-09-03

This document preserves the source audit, design decisions, implementation
order, and proof history for the semantic metadata and static graph system. It
is maintainer provenance, not the public learning path. Current users should
start with `metadata_system_concepts.md`, `extensions_guide.md`, and the
capability-specific contracts.

## 1. Current Source Findings

### 1.1 Attribute syntax is already generic enough

`src/Parser/Helpers/parser_attributes.c` currently recognizes:

- GNU `__attribute__((...))`;
- standard-style `[[...]]`;
- `__declspec(...)`;
- assembler and selected compatibility attributes.

`[[...]]` content is reconstructed as one raw payload string and stored in one
`ASTAttribute`. The parser does not split a comma-separated standard attribute
list into independently typed attributes.

Initial implementation implication:

- reuse this parser unchanged;
- require one fisiCs metadata attribute per `[[...]]` specifier;
- implement a bounded semantic payload extractor;
- do not turn the attribute parser into a metadata grammar.

### 1.2 `ASTAttribute` should stay small

`src/AST/ast_attribute.h` currently stores only syntax kind and payload. Clone,
list append, and destruction logic are simple and widely reused.

Implication:

- do not add typed metadata fields to `ASTAttribute`;
- use the owner AST node's range initially;
- defer exact attribute-token provenance until it has an independent parser
  justification.

### 1.3 Parser acceptance is broader than semantic support should be

Attributes are attached in variable, function, parameter, type, aggregate,
field, typedef, and statement paths. The common `ASTNode` shape also has an
attribute list.

Implication:

- the metadata resolver must validate the semantic target;
- first acceptance is limited to functions and single-declarator file-scope
  variables;
- parser acceptance alone must not imply a supported metadata contract.

### 1.4 Multi-declarator variables are an ownership trap

One `AST_VARIABLE_DECLARATION` may own several names and declared types. Current
units metadata is keyed by the owner declaration node, with per-declarator
projection handled separately when exporting units.

Implication:

- reject metadata-bearing multi-declarator declarations in the first contract;
- do not guess that metadata applies to all declarators;
- do not add declarator-index ownership without a dedicated contract.

### 1.5 Function attributes are present but units parser hooks are not the right model

Function parser paths append attributes to function declaration/definition AST
nodes. Existing `fisics_extension_note_declaration()` calls are concentrated on
variable, parameter, and field parsing for units.

Implication:

- semantic metadata should not depend solely on the units parser-time note hook;
- run a metadata AST walk from the existing post-core-semantics extension hook;
- bind functions after core semantic analysis has created their symbols.

### 1.6 The extension semantic lifecycle is suitable

`src/Syntax/semantic_pass.c` currently runs:

1. core semantic analysis;
2. control-flow analysis;
3. `fisics_extension_run_semantic_scaffold(root, globalScope)`;
4. final error reporting/model creation.

This is the correct first semantic-metadata integration point because core C
binding remains authoritative and extension diagnostics still participate in
the normal semantic result.

### 1.7 Extension state is units-shaped, not generic

`FisicsExtensionState` currently directly owns units annotation, expression,
binding, and call-contract arrays.

Implication:

- add separately named metadata storage with independent allocation/destruction;
- do not rewrite the units subsystem merely to make the root state aesthetically
  generic;
- a later cleanup may split nested `units` and `semanticMetadata` sub-states
  after behavior is proven.

### 1.8 Frontend export is additive and capability-gated

`FisicsAnalysisResult` currently exports optional units attachments. The
frontend copies compiler-owned state into owned result arrays, advertises
capability bits only for populated valid lanes, and frees every copied string.

Implication:

- semantic metadata declaration model must not change the frontend ABI;
- translation-unit metadata export adds a separate metadata projection and capability through the frozen
  size/version-tagged extended API; it does not resize the legacy result;
- invalid-only input must remain diagnostics-only;
- CLI and frontend export must share one projection implementation.

### 1.9 Stable symbol coverage differs by declaration scope

Top-level exported symbols have deterministic stable IDs. Existing local units
attachments can instead use source ranges when no stable exported symbol ID is
available.

Implication:

- begin with functions and file-scope variables;
- add local declarations only after a location-backed metadata identity policy
  is specified and tested;
- never invent a stable symbol ID for a local record that lacks one.

### 1.10 Driver multi-TU compilation is not a semantic graph merge

The driver compiles translation units through independent compiler contexts and
aggregates diagnostics/link inputs. Existing per-TU semantic models do not form
one project-wide symbol or extension-state graph.

Implication:

- semantic metadata declaration model uniqueness is translation-unit scoped;
- declaration relationship model isolated references may remain external;
- project-wide merge belongs to a later manifest/graph assembly owner;
- ordinary multi-file linking does not prove metadata resolution.

### 1.11 Diagnostics have a stable extension lane

Units already use generic extension/bootstrap codes in `4000-4099` and units
semantic codes in `4100-4199`, categorized as extension diagnostics.

Implication:

- reserve proposed metadata codes in `4200-4299`;
- register names/stages/explanations when implementation begins;
- add frontend taxonomy tests before treating numbers as public contract.

### 1.12 `ide-metadata` must not be silently redefined

`FISICS_OVERLAY_IDE_METADATA` is an existing profile bit, participates in
`overlay=all`, and is recorded in build-graph configuration.

Implication:

- add `FISICS_OVERLAY_SEMANTIC_METADATA` separately;
- leave `ide-metadata` behavior and emitted profile identity unchanged;
- defer aliasing/deprecation decisions.

## 2. Proposed Source Ownership

The first implementation should prefer focused new files over continued growth
of units-specific sources.

Proposed ownership map:

```text
src/Extensions/SemanticMetadata/
    semantic_metadata_model.[ch]
        kinds, records, canonical-name validation, storage ownership

    semantic_metadata_parser.[ch]
        exact extraction of metadata-owned ASTAttribute payloads

    semantic_metadata_semantics.[ch]
        AST walk, target validation, symbol binding, duplicate/redecl merge

src/Extensions/
    extension_profile.[ch]
        new feature bit and CLI/environment spelling

    extension_hooks.[ch]
        state lifetime and post-core-semantics dispatch only

    extension_diagnostics.[ch]
        metadata diagnostic constructors or routing

src/Syntax/
    semantic_model_printer.c
        deterministic debug visibility
```

semantic metadata declaration model did not edit:

- code generation;
- object emission;
- linker/runtime support;
- `fisics_frontend.h`;
- project manifest schemas;
- IDE source;
- hardware or simulation repositories.

Build-graph overlay serialization was extended only to preserve the selected
profile identity. No graph schema or execution behavior changed during
semantic metadata declaration model. The separately designed public view was then added in translation-unit metadata export
without changing that build-graph schema.

## 3. Minimal Static Descriptor Table

Avoid a dynamic plugin/handler system. Use a small compiler-owned descriptor
table sufficient to make accepted metadata explicit:

```text
source name
metadata kind
argument kind
allowed target mask
cardinality
redeclaration merge policy
```

Initial entries:

| Attribute | Argument | Targets | Cardinality |
| --- | --- | --- | --- |
| `fisics::id` | canonical string | function, file-scope variable | singular |
| `fisics::domain` | registered canonical string | function, file-scope variable | singular |
| `fisics::role` | registered canonical string | function, file-scope variable | singular |

declaration relationship model adds repeatable `produces` and `consumes` reference descriptors.

The table is an internal dispatch seam, not a public plugin ABI.

## 4. Capability Implementation Sequence

### initial semantic metadata contract — Contract freeze candidate

Scope:

- review `docs/semantic_metadata_contract.md` against source;
- lock canonical name grammar;
- lock semantic metadata overlay scaffold through deterministic semantic dump targets and redeclaration policy;
- lock overlay-disabled and `overlay=all` behavior;
- create valid/invalid fixture text and expected diagnostics before semantic
  implementation.

Exit gate:

- every semantic metadata overlay scaffold through deterministic semantic dump behavior has an explicit expected outcome;
- no unresolved decision would change public syntax or record ownership.

No source behavior changes in initial semantic metadata contract.

### semantic metadata overlay scaffold — Overlay and typed state scaffold (complete)

Scope:

- add the overlay bit and parser spelling;
- create metadata state lifetime and read-only view;
- add exact payload extraction for `id`, `domain`, and `role`;
- record pending typed attachments during the post-core-semantics walk.

Stop before:

- diagnostics beyond essential malformed/disabled checks;
- redeclaration merging;
- dumps;
- relationships;
- frontend or JSON export.

Proof:

- overlay parser contract test;
- state create/destroy test;
- exact extractor tests;
- default and existing overlay bit isolation.

### metadata validation and redeclaration rules — Target validation, canonicalization, and redeclarations (complete)

Scope:

- validate functions and single-declarator file-scope variables;
- validate canonical ASCII names and seed domain/role registries;
- reject same-owner duplicates;
- merge identical prototype/definition metadata;
- reject conflicting redeclaration metadata;
- detect translation-unit duplicate authored IDs;
- add proposed metadata diagnostics and taxonomy registration.

Stop before:

- relationship edges;
- project-global claims;
- public frontend ABI;
- JSON export.

Proof:

- valid function declaration/definition pair;
- valid file-scope variable;
- invalid local, parameter, field, type, statement, and multi-declarator targets;
- malformed, noncanonical, duplicate, conflicting, and overlay-disabled cases;
- core-invalid redeclarations remain owned by core diagnostics.

### deterministic semantic dump — Deterministic `--dump-sema` visibility (complete)

Scope:

- print accepted attachment kind, canonical value, symbol identity, and source
  location;
- print pending/invalid/conflicted aggregate state without presenting it as
  accepted metadata;
- sort independently of allocation and symbol-table bucket order.

Proof:

- repeat-run byte equality;
- input-order and redeclaration fixtures;
- no metadata output when the overlay is disabled or no valid records exist.

The deterministic semantic dump was the first complete contract boundary.

### declaration relationship model — Relationship records and internal graph (complete)

Scope:

- add `produces` and `consumes`;
- make them repeatable for distinct target identities;
- diagnose a repeated identical edge on one declaration owner;
- union distinct edges across compatible redeclarations and merge an identical
  edge repeated across separate compatible redeclarations;
- derive typed edges from accepted attachments;
- allow a source without an authored ID by using deterministic compiler
  projection identity;
- treat every target as a category-neutral canonical semantic identity;
- use `external` for valid references outside isolated-TU resolution authority;
- add deterministic graph iteration.

Stop before:

- hardware-specific edge kinds;
- strict project resolution;
- public export.

Proof:

- multiple relationships preserve deterministic ordering;
- same-owner duplicate relationships fail while compatible-redeclaration
  duplicates merge;
- malformed targets fail;
- target prefixes do not trigger inferred category or resolution behavior;
- sources without authored IDs receive stable, pointer-independent projection
  identities;
- isolated valid external targets do not falsely resolve or fail compilation;
- graph nodes/edges contain no pointer-derived identity.

Implementation sequence:

1. Add frozen valid/invalid fixtures and diagnostic expectations.
2. Introduce the small internal metadata descriptor seam, preserving all
   existing `id`, `domain`, and `role` behavior.
3. Add a separately owned repeatable relationship-edge array; do not overload
   singular declaration-record fields.
4. Parse and validate `produces` and `consumes` through the existing bounded
   one-string extractor and canonical-name validator.
5. Bind relationship sources after core C semantics, then merge redeclarations
   and derive translation-unit-local graph nodes and edges.
6. Resolve matching in-TU authored identities, mark other valid targets
   `external`, and leave strict `unresolved` behavior for semantic project graph.
7. Extend `--dump-sema` with deterministically sorted relationship and graph
   projections.
8. Run the focused metadata contract, ordinary-C byte-isolation, overlay
   composition, frontend-contract, and broad `make test` gates.

### translation-unit metadata export — Additive frontend and CLI export

Contract status: implemented as specified in
`semantic_metadata_export_contract.md`.

Scope:

- implement the frozen versioned semantic metadata projection;
- add the size/version-tagged `FisicsAnalysisResultEx` API without changing
  `FisicsAnalysisResult` or the legacy entry points;
- add the capability-gated frontend lane using bit `1ULL << 8`;
- add exact CLI option `--emit-semantic-metadata-json <path>`;
- share one compiler-owned projection between consumers;
- preserve partial/fatal and source provenance.

Proof:

- legacy caller allocation and function behavior remain unchanged;
- incompatible extended ABI version/size fails before any out-of-bounds write;
- empty overlay lane stays null/unadvertised;
- invalid-only input stays diagnostics-only;
- valid metadata advertises the capability;
- ownership/freeing tests cover every allocated string/array;
- CLI JSON is byte-deterministic;
- CLI output is atomic and preserves an existing destination on failure;
- frontend and CLI projections are semantically identical;
- older contract consumers remain compatible.

### semantic project graph — Strict project graph assembly

Frozen contract:

- `docs/semantic_metadata_project_graph_contract.md`

Scope:

- use a dedicated `fisiCs.semantic_project_manifest` v1 aggregation owner,
  separate from the existing build-manifest v0 contract;
- merge per-TU metadata projections;
- enforce project-global authored ID uniqueness;
- resolve source-defined semantic targets;
- distinguish permitted external references from missing required references;
- verify every input artifact against current source bytes; and
- publish one deterministic, atomic `fisiCs.semantic_project_graph` v1 artifact.

The design checkpoint is complete. Implementation must consume complete translation-unit metadata export
artifacts rather than retaining or reconstructing merged compiler semantic
models. The exact CLI, manifest, project graph, ordering, failure, and stop
contracts are frozen in the dedicated document.

### physical composition graph — General physical composition

Scope:

- emit a dedicated current-source units evidence artifact without changing
  translation-unit metadata export, semantic project graph, or the legacy frontend result;
- compose semantic identity, quantity kind, dimension, concrete-unit policy,
  value form, reference semantics, and optional coordinate-frame identity;
- accept project-defined quantity kinds with explicit dimensions instead of
  freezing one application-specific vocabulary;
- distinguish source-compatible, source-unverified, externally declared, and
  uncovered states without claiming physical verification;
- retain unbound units attachments instead of discarding evidence that does not
  yet map to a semantic project graph record; and
- publish a deterministic physical project graph for later hardware and
  simulation consumers.

Stop before:

- expression metadata;
- callable port inference or C-layout validation;
- unit model duplication;
- hardware catalogs, calibration, telemetry, simulation, or runtime behavior.

Frozen contract:

- `docs/semantic_metadata_physical_composition_contract.md`

### hardware compatibility graph — Hardware-contract canary

Status: implemented.

Scope:

- one node/board profile;
- one bus;
- one sensor;
- one actuator;
- one small external catalog;
- device existence, node ownership, driver/model pairing, one allocation
  conflict check, and one signal-dimension check.

The contract interprets existence as exact resolution in supplied static
catalog/topology artifacts, never physical presence. The canary counts bound
the proof fixture rather than the schema vocabulary. Reusable profiles/models,
configured instances, physical contracts, and future runtime observation
identity remain separate. Catalog authority, topology compatibility, and
propagated physical composition graph evidence state are explicit; hardware presence, wiring, driver
implementation, calibration, telemetry, simulation, deployment, and physical
acceptance remain `not_assessed`.

Stop before:

- driver generation;
- CAN execution;
- telemetry;
- simulation;
- deployment/flashing;
- physical acceptance.

Frozen contract:

- `docs/semantic_metadata_hardware_contract.md`

Implementation proceeded through fixture lock, owned readers, catalog merge and
provenance, instance and ownership resolution, driver and allocation
validation, physical-signal binding, deterministic graph publication, focused
and broad proof, and documentation closeout.

## 5. Initial Test Matrix

The initial metadata proof should cover:

| Area | Required case |
| --- | --- |
| Overlay | exact `semantic-metadata` spelling parses |
| Overlay | comma-composition with `physics-units` works |
| Overlay | existing bits remain independent |
| Overlay | `all` remains unchanged |
| Default C | no-metadata compile output/status unchanged |
| Syntax | one exact string argument accepted |
| Syntax | missing, extra, non-string, empty, escaped, or oversized argument rejected |
| Name | valid dotted lowercase canonical form accepted |
| Name | uppercase, whitespace, empty segment, leading/trailing dot rejected |
| Target | function prototype accepted |
| Target | function definition accepted |
| Target | one file-scope variable accepted |
| Target | local/parameter/field/type/statement rejected as deferred |
| Ownership | multi-declarator variable rejected |
| Cardinality | repeated singular attribute on one owner rejected |
| Redeclaration | identical prototype/definition values merge |
| Redeclaration | conflicting values reject the merged record |
| Identity | duplicate authored IDs in one TU rejected |
| Diagnostics | stable code/category/stage/name metadata |
| Dump | deterministic accepted output |
| Isolation | code generation and runtime output unchanged |
| Regression | physics-units focused lane remains green |
| Regression | memory-check overlay profile contract remains green |
| ABI | frontend header/result shape unchanged through deterministic semantic dump |

## 6. Validation Ladder

Use repository confidence tiers rather than running the full suite first.

For semantic metadata overlay scaffold through deterministic semantic dump:

1. `make`
2. focused unit/parser/extension integration target introduced for metadata
3. `make test`
4. `make frontend-contract-test`
5. focused physics-units overlay gate
6. focused memory-check overlay profile contract
7. a suitable stable final manifest or bucket after the behavior is promoted

Only use `make final-monitored` at the first broader handoff/merge checkpoint or
when the touched shared behavior justifies Tier 6.

No release archive, `VERSION`, publication, package, Registry, IDE, or hardware
operation was part of this implementation sequence.

## 7. Risk Register

### Attribute-list ambiguity

Risk: current `[[...]]` capture stores one payload and does not structurally
split multiple comma-separated attributes.

Control: one fisiCs attribute per specifier in the initial contract.

### Declarator ownership ambiguity

Risk: one variable declaration node may own multiple symbols.

Control: require exactly one declarator for metadata-bearing variables.

### Function redeclaration fragmentation

Risk: prototypes and definitions are separate AST nodes but represent one C
entity.

Control: merge through the core-resolved symbol and reject conflicting singular
metadata.

### Hash/symbol-table nondeterminism

Risk: semantic symbol iteration follows symbol-table buckets.

Control: collect then sort every dump/export projection by explicit stable keys.

### False cross-TU confidence

Risk: the driver compiles TUs independently and aggregates diagnostics, which
can look like a merged semantic analysis.

Control: label early scope as translation-unit-only and defer strict resolution
to manifest-backed graph assembly.

### ABI expansion too early

Risk: freezing public structures before the internal model stabilizes creates
compatibility debt.

Control: keep semantic metadata declaration model internal/debug-only; export in translation-unit metadata export.

### Units duplication

Risk: signal or hardware metadata grows a second dimension system.

Control: all physical validation queries the existing units read-only view.

### Generic metadata becoming untyped tags

Risk: accepting arbitrary keys produces documentation strings rather than
compiler semantics.

Control: use a closed descriptor table and versioned canonical registries.

### Existing overlay behavior drift

Risk: redefining `ide-metadata` or `all` changes existing manifests and build
graphs.

Control: add an independent bit and preserve current bundle behavior initially.

## 8. Decisions Frozen For The Initial Metadata Surface

The initial semantic metadata contract checkpoint froze these decisions:

1. Accept `semantic-metadata` as the canonical overlay spelling.
2. Keep it outside `overlay=all` throughout the initial rollout.
3. Use `4200-4299` as the diagnostic reservation, with `4201-4206` live.
4. Require one attribute per `[[...]]` specifier.
5. Require canonical lowercase dotted names rather than normalizing input.
6. Accept only functions and single-declarator file-scope variables initially.
7. Treat same-owner repeated singular attributes as errors.
8. Merge identical metadata across compatible redeclarations.
9. Treat isolated reference targets as external, not resolved or erroneous.
10. Delay public frontend/export work until the internal graph is stable.

## 9. Decisions Frozen For Declaration Relationships

The declaration relationship model design checkpoint froze these additional decisions before source
implementation:

1. Add only `produces` and `consumes` as relationship kinds.
2. Treat relationships as authored assertions, not inferred or runtime-proven
   behavior.
3. Allow multiple distinct targets for each relationship kind.
4. Diagnose an identical kind/target repeated on one declaration owner rather
   than silently deduplicating it; reserve diagnostic `4209` for this case.
5. Union distinct relationships across compatible redeclarations and merge one
   identical relationship appearing on separate compatible redeclarations.
6. Allow relationship sources without `fisics::id` by using deterministic
   compiler projection identity; never use pointers or allocation order.
7. Treat the target string only as a canonical semantic identity. Do not infer
   a signal, device, method, hardware, simulation, or runtime category from its
   prefix.
8. Resolve a target only when its authored identity exists in the active
   translation unit; otherwise preserve the valid target as `external`.
9. Keep strict missing-reference diagnostics in semantic project graph assembly.
10. Keep declaration relationship model internal and debug-visible through `--dump-sema`; do not change
    the frontend ABI or add JSON export.

## 10. Initial Contract Completion Checklist

- [x] Review syntax examples against parser fixtures.
- [x] Confirm canonical name grammar and length limit.
- [x] Confirm seed domain and role registries.
- [x] Confirm function attribute placement used by canonical examples.
- [x] Confirm single-declarator restriction.
- [x] Confirm redeclaration merge/conflict rules.
- [x] Confirm overlay-disabled diagnostic policy.
- [x] Confirm `overlay=all` remains unchanged.
- [x] Confirm diagnostic reservation.
- [x] Implement valid/invalid outcomes in `make integration-overlay-semantic-metadata`.
- [x] Add overlay isolation to `make frontend-contract-test`.
- [x] Preserve unrelated OS-policy/probe work in the dirty worktree.

declaration relationship model implementation followed this order and stopped before public export,
strict project resolution, units composition, or hardware-specific vocabulary.

## 11. Relationship Model Execution Checklist (complete)

Work through declaration relationship model in this order:

- [x] Freeze exact valid, invalid, duplicate, redeclaration, and external-target
  fixture output before changing semantic behavior.
- [x] Add the bounded descriptor seam and prove no regression in existing
  `id`, `domain`, and `role` parsing or diagnostics.
- [x] Add separately owned relationship-edge state and lifecycle coverage.
- [x] Implement `produces` and `consumes` parsing, target validation, and
  same-owner duplicate diagnostic `4209`.
- [x] Implement compatible-redeclaration set union and deterministic
  source-identity fallback when no authored ID exists.
- [x] Derive translation-unit-local graph records and edges, resolving only
  in-scope authored IDs and retaining all other valid targets as `external`.
- [x] Extend `--dump-sema` and prove repeat-run byte equality plus stable
  source/edge ordering.
- [x] Prove ordinary-C object identity, overlay isolation/composition, unchanged
  frontend ABI, and focused units and memory-check regression behavior.
- [x] Run `make test` only after focused declaration relationship model gates pass.
- [x] Update the then-current docs from implemented evidence while keeping
  project resolution, physical composition, and hardware validation explicitly
  unimplemented at that checkpoint.

The translation-unit export was completed before the semantic project graph
was designed and implemented separately. Project-wide behavior was never
inferred from the single-translation-unit artifact.

## 12. Translation-Unit Metadata Export Implementation Record

The frozen contract was implemented in these separately proven steps:

1. **[complete] Baseline and fixture lock.** Record the legacy result size,
   capability mask, public function signatures, ordinary-object hash, accepted
   declaration relationship model record/edge inputs, exact JSON goldens, and preserved-destination
   failure fixture before adding the new API. Add the new tests to focused
   targets in a disabled/expected-pending state rather than weakening current
   green gates.
2. **[complete] Public declarations and ABI guards.** Add the frozen constants,
   frontend-prefixed enums/structures, extended entry-point declarations, and
   compile-time layout/version tests. Preserve the legacy result size and
   signatures exactly.
3. **[complete] Shared ordered projection builder.** Copy valid declaration relationship model records and
   edges into independently owned frontend-prefixed arrays, normalize ranges
   and paths, assign sorted record indices, and derive resolved target indices.
4. **[complete] Extended frontend lifecycle.** Implement ABI-size validation,
   single-pass analysis integration, capability gating, fatal/allocation
   cleanup, required-after-call destruction, and idempotent destruction.
5. **[complete] Deterministic JSON serializer.** Serialize only the shared
   projection using the frozen schema, null rules, enum spellings, stable-ID
   string encoding, source hash, and normative key order.
6. **[complete] CLI and atomic publication.** Add the exact single-TU option,
   overlay and path validation, destination-directory temporary file, complete
   flush/close, atomic replacement, and no-write/preserve-existing failure
   behavior.
7. **[complete] Focused negative and parity proof.** Cover every row in the frozen
   acceptance matrix, allocation failure injection, legacy API isolation,
   frontend/JSON equivalence, repeat-run bytes, and ordinary-object identity.
8. **[complete] Broad regression and documentation closeout.** Run the focused
   frontend, CLI, parser, units, memory-check, build-graph, and full repository
   gates; then change translation-unit metadata export documentation from frozen to implemented using only
   the resulting evidence.

Recommended ownership seams:

- a new `pipeline_artifacts_semantic_metadata` module owns the single ordered,
  public copy-out projection and its destruction helpers;
- `fisics_frontend.h` and `fisics_frontend.c` own only the extended public API,
  ABI validation, base-result composition, and capability publication;
- a dedicated semantic-metadata JSON module owns schema-v1 serialization but
  consumes the shared projection rather than compiler internals;
- `main.c` and the existing driver configuration/result lifetime own option
  parsing, single-input rejection, and the atomic artifact publication point;
- the internal declaration relationship model parser, semantic merge, and model remain authoritative for
  accepted meaning and must not be duplicated in any export module.

The export implementation deliberately stopped before project aggregation,
strict unresolved diagnostics, physical composition, and hardware validation.

Closeout evidence on 2026-09-02:

- `make frontend-contract-test`: pass, including legacy ABI guards, extended
  lifecycle/ownership, records-only/null handling, invalid metadata, and
  injected projection-allocation failure;
- `make integration-cli-semantic-metadata`: pass, including exact schema-v1
  bytes, frontend/CLI parity, repeatability, object identity, linked and
  compile-only use, single-input/overlay/path/manifest rejection, atomic
  replacement failure, and preserved destinations;
- `make test`: pass across parser, syntax, codegen, preprocessor, diagnostics,
  units, memory-check, build-graph, build-manifest, and integration lanes; and
- the documentation stale-reference verifier scanned 38 documents with zero
  findings.

## 13. Semantic Project Graph Execution Record

The semantic project graph contract and implementation are complete. Work proceeded in this
order:

1. **[complete] Fixture and negative-behavior lock.** Froze exact inputs,
   graph output, cross-TU resolution, allowed external, missing-required,
   duplicate-ID, stale-source, permutation, and destination-preservation cases.
2. **[complete] Dedicated manifest parser.** Implemented the owned
   `fisiCs.semantic_project_manifest` v1 parser without changing
   `FisicsBuildManifest` or build-manifest v0 behavior.
3. **[complete] Artifact reader and source verifier.** Parses and validates
   complete translation-unit metadata export schema-v1 artifacts, then require current source length/hash
   equality before accepting them.
4. **[complete] Global identity index.** Deterministically sorts translation
   units and records, assign global indices, and reject duplicate authored IDs
   or fallback source identities with origin evidence.
5. **[complete] Strict edge resolution.** Resolves project-authored targets,
   preserve only explicitly allowlisted external targets, and reject all other
   missing targets without publishing a graph.
6. **[complete] Graph serializer and atomic writer.** Emits the frozen
   `fisiCs.semantic_project_graph` v1 shape from owned validated state and
   preserve existing destinations on every failure.
7. **[complete] Terminal CLI integration.** Adds the exact paired
   `--semantic-project-manifest` and
   `--emit-semantic-project-graph-json` options, rejecting compile/build modes
   before compiler initialization.
8. **[complete] Focused and broad proof.** Proves exact bytes, input-order
   independence, repeatability, negative cases, atomicity, ordinary-object
   identity, unchanged translation-unit metadata export behavior, and unchanged build-manifest v0 behavior;
   then widen through the documented compiler confidence tiers.
9. **[complete] Documentation closeout.** Marks the contract implemented
   from passing evidence and leaves units, hardware, simulation, deployment,
   and physical acceptance outside semantic project graph.

The focused and broad evidence commands are recorded in
`docs/semantic_metadata_project_graph_contract.md`.

## 14. Physical Composition Graph Execution Record

The general physical composition contract is implemented in
`docs/semantic_metadata_physical_composition_contract.md` in this order:

1. **[complete] Vocabulary and fixture lock.** Freezes scalar, vector,
   same-dimension/different-kind, reference, external, fallback-source, unbound,
   permutation, and negative fixtures plus exact output bytes.
2. **[complete] Units evidence artifact.** Emits deterministic
   `fisiCs.units_metadata` v1 from the existing units view with complete source
   provenance and unit-registry fingerprint, without changing units semantics
   or existing public result layouts.
3. **[complete] Catalog and manifest readers.** Implements strict owned
   `fisiCs.physical_contract_catalog` and `fisiCs.physical_project_manifest`
   v1 readers, limits, path canonicalization, and destruction.
4. **[complete] Provenance and binding index.** Verifies every source
   receipt, require exact TU coverage and registry identity, join only through
   stable ID plus source/name/range confirmation, and retain unbound evidence.
5. **[complete] Compatibility engine.** Validates quantity definitions,
   dimension, unit policy, reference semantics, value form, and frame identity
   through the read-only units authority; emit scoped states and explicit
   conversion recipes.
6. **[complete] Physical graph serializer.** Emits deterministic
   `fisiCs.physical_project_graph` v1 with receipts, coverage, exact binary64
   conversion strings, and atomic destination preservation.
7. **[complete] Terminal CLI integration.** Adds the exact paired
   physical manifest/output options and reject compilation, build, translation-unit metadata export, semantic project graph,
   and other output modes before compiler initialization.
8. **[complete] Focused and broad proof.** Completes the frozen acceptance
   matrix, sanitized failure injection, deterministic/parity checks, ordinary
   object identity, translation-unit metadata export/semantic project graph/build-manifest regressions, and the justified
   repository confidence tier.
9. **[complete] Documentation closeout.** Marks physical composition graph implemented
   from passing evidence. hardware compatibility graph was still pending at that checkpoint and is now
   implemented under its separate contract; simulation, runtime, deployment,
   and physical-acceptance work remain pending.

The focused commands are `make integration-cli-units-metadata` and
`make integration-physical-project-graph`. Sanitized focused execution and the
inherited translation-unit metadata export/semantic project graph regression lanes pass. The hardware catalog consumer
is now implemented; every simulation/runtime consumer remains a later,
separately authorized boundary.

## 15. Hardware Compatibility Graph Execution Record

The static hardware contract is implemented in
`docs/semantic_metadata_hardware_contract.md`. The implementation provides a
strict offline parser, resolver, deterministic graph, and terminal CLI without
executing hardware behavior.

1. **[complete] Vocabulary and fixture lock.** Froze one vocabulary-neutral
   node/board, bus, sensor, actuator, project catalog, external-declared catalog,
   positive topology, negative corpus, and normalized graph golden. Manifest
   path-order permutations are byte-identical; changed catalog/topology bytes
   retain distinct raw provenance receipts.
2. **[complete] Owned readers and models.** Implemented strict manifest,
   component-catalog, and topology readers with canonical paths, receipts,
   limits, exact schemas, and complete destruction.
3. **[complete] Catalog merge and provenance.** Verifies the complete physical composition graph
   input and current source receipts, retain authority, deterministically
   deduplicate identical definitions, and reject conflicting revisions.
4. **[complete] Instance and ownership resolution.** Resolves board, bus,
   device, endpoint, attachment, capability, scope, and node-owner references.
5. **[complete] Driver and allocation validation.** Enforces model driver
   policy, exact driver/model/transport compatibility, required claims, ranges,
   and exclusive allocation/endpoint conflict rules.
6. **[complete] Physical-signal bridge.** Resolves device physical ports to
   physical composition graph contract IDs, compare exact quantity or Dim8 requirements, retain
   value/reference constraints, propagate evidence state, and enforce coverage.
7. **[complete] Graph serializer and terminal CLI.** Publishes normative,
   deterministic, atomic `fisiCs.hardware_project_graph` v1 only through the
   exact paired hardware manifest/output mode.
8. **[complete] Focused and broad proof.** Completed deterministic,
   negative, duplicate, provenance, alias, failure-injection, sanitizer,
   ordinary-C, translation-unit metadata export/semantic project graph/physical composition graph, build-manifest, and justified broad gates.
9. **[complete] Documentation closeout.** Marks implementation complete
   only from passing evidence while retaining all runtime and physical states
   as `not_assessed`.

The focused command is `make integration-hardware-project-graph`. Its full
acceptance matrix passes under both sanitized and ordinary builds, inherited
translation-unit metadata export/semantic project graph/physical composition graph lanes pass, and `make test` passes. Calibration, telemetry,
simulation, driver generation/execution, deployment, and physical acceptance
remain separately designed future boundaries.

## 16. Extension Public Adoption Record

The first public-adoption effort is implemented without adding another
language or artifact capability:

1. **[complete] Current-state cleanup.** Public indexes and planning prose no
   longer describe implemented hardware compatibility graph behavior as pending.
2. **[complete] Extension selection guide.** `docs/extensions_guide.md` gives a
   task-oriented route across ordinary C, the three overlays, and the offline
   graph pipeline.
3. **[complete] Basic semantic example.** `examples-semantic-metadata` emits and
   checks one small translation-unit metadata export artifact and deterministic runtime result.
4. **[complete] End-to-end hardware example.** `examples-semantic-hardware`
   constructs fresh translation-unit metadata export, semantic project graph, physical composition graph, and hardware compatibility graph artifacts beneath `build/examples`
   and verifies repeat graph bytes and scoped truth states.
5. **[complete] Portability header.** `include/fisics/extensions.h` covers the
   implemented units and semantic attributes through the existing
   `__FISICS__` compiler identity.
6. **[complete] Clang/fisiCs comparison.** `examples-semantic-metadata-compare`
   proves strict Clang C99 compilation, exact runtime parity, and separately
   retained fisiCs semantic and units evidence.

## 17. Extension Agent Adoption Record

The agent-facing layer is implemented without changing compiler behavior or
the semantic metadata and static graph system contracts:

1. **[complete] Repository-owned agent contract.**
   `docs/agent_extension_guide.md` defines layer selection, source-of-truth
   order, provenance/freshness checks, evidence vocabulary, failure routing,
   proportional validation, and completion reporting.
2. **[complete] Specialized units-skill refinement.** The canonical
   `fisics-units-authoring` skill now prefers the portability macros for
   intentional shared-source work, retains native attributes for fisiCs-only
   surfaces, and hands multi-file identity/physical/hardware composition to the
   extension router.
3. **[complete] Companion extension router.** The canonical
   `fisics-extension-authoring` skill routes ordinary C, units, memory-check,
   semantic metadata, and static physical-system graph pipeline requests to focused references and
   public commands.
4. **[complete] Agent-use evaluation matrix.** Realistic units-only,
   metadata-only, hardware-graph, stale-artifact, strict-Clang, and
   physical-proof cases lock the intended routing and non-claim decisions.

## 18. Portable Macro Adoption Record

The in-repository public extension examples now use the portability header
without rewriting native compiler evidence:

1. **[complete] Classification.** Public examples were separated from native
   compiler/parser, API, integration, probe, and negative-fixture coverage.
2. **[complete] Units adoption.** The complete physics-units pilot and the
   units-bearing `kinematics_stepper` and `collision_1d` curated projects use
   `FISICS_DIM` and `FISICS_UNIT`.
3. **[complete] Metadata adoption.** The basic, portable, and static physical-system graph pipeline
   hardware examples use the semantic `FISICS_*` macros.
4. **[complete] Native parity fixture.** Equivalent native and macro sources
   cover all seven public macros. Normalized translation-unit metadata export records/edges and units
   attachments must remain identical while source-specific provenance remains
   distinct.
5. **[complete] Strict control build.** The macro corpus preprocesses without
   `fisics::` residue and compiles with Clang C99, `-Wall -Wextra -Werror
   -pedantic`.
6. **[complete] Runtime and negative proof.** Executable public examples match
   exact Clang/fisiCs output; intentionally units-invalid macro sources compile
   as ordinary C under Clang and remain rejected by fisiCs units validation.

The permanent focused command is
`make integration-extension-portability-macros`; the public alias is
`make examples-extension-portability`. External program repositories remain
separate customer-adoption boundaries, and native syntax fixtures remain
direct attributes by design.
