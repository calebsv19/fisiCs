# Semantic Metadata Extension Contract

Status: semantic metadata and static graph system implemented

Contract track: `semantic-metadata` design v0

Last audited against source: 2026-09-03

This document defines the typed semantic metadata contract for `fisiCs`. The
identity/domain/role, relationship-graph, single-translation-unit export, and
strict project-graph capabilities are implemented. The general physical
composition contract and static hardware contract are also implemented;
runtime evidence and simulation integration remain later design boundaries.

The first goal is deliberately narrow:

> Attach an authored identity, one canonical domain, and one canonical role to
> selected C declarations through the existing `[[fisics::...]]` attribute
> pipeline, resolve them into compiler-owned semantic records, diagnose invalid
> or conflicting input, and expose them in `--dump-sema` without changing core
> C semantics or generated code.

The exact implemented translation-unit metadata export frontend and JSON boundary is defined in
`semantic_metadata_export_contract.md`; the implemented project-wide consumer
is defined in `semantic_metadata_project_graph_contract.md`; the implemented
physical composition boundary is defined in
`semantic_metadata_physical_composition_contract.md`; the implemented hardware compatibility graph
boundary is defined in `semantic_metadata_hardware_contract.md`. Runtime
hardware operation, simulation, and IDE presentation remain later additive
capabilities.

## 1. Design Invariants

The following are hard boundaries for the initial design:

1. Ordinary C remains authoritative. A source file with no enabled overlay and
   no fisiCs metadata retains its existing parse, semantic, and code-generation
   behavior.
2. `[[fisics::...]]` remains the canonical direct extension syntax. No `@`
   sigil or new lexer grammar is introduced.
3. `ASTAttribute` remains a parsed syntax carrier. Typed metadata meaning lives
   in extension-owned semantic state.
4. Semantic metadata is not added to `ParsedType` or `TypeInfo`.
5. The metadata overlay has no code-generation or runtime effect in the first
   contract.
6. The physics-units overlay remains the sole authority for physical dimensions
   and concrete units.
7. Generic semantic metadata is not a free-form key/value bag. Each accepted
   attribute has a compiler-known kind, argument contract, target policy,
   cardinality, and validation rule.
8. Compiler symbol identity, authored semantic identity, external catalog
   identity, and runtime instance identity remain distinct concepts.
9. Translation-unit validation must not claim project-global uniqueness or
   external-reference resolution.
10. Exported or dumped identities must never depend on `ASTNode*` addresses,
    allocation order, or hash-table iteration order.

## 2. Layer Ownership

The intended ownership model is:

```text
core C parser and semantics
    owns C meaning and symbol binding

attribute parser / ASTAttribute
    owns syntax capture and declaration attachment

semantic-metadata overlay
    owns typed identity, classification, references, graph derivation,
    metadata diagnostics, and deterministic metadata projection

physics-units overlay
    owns dimensions, concrete units, conversions, and units diagnostics

hardware compatibility consumer
    owns device catalogs, topology, driver/capability checks, and hardware
    reference policy

runtime and simulation systems
    own telemetry, device discovery, transport, state evolution, calibration,
    supervision, and physical acceptance
```

The implemented hardware compatibility graph consumer proves whether supplied static topology is
internally coherent. It cannot prove that physical hardware is
wired, powered, flashed, or behaving as declared.

## 3. Overlay Contract

The overlay name is:

```text
semantic-metadata
```

The implementation should add a distinct feature bit rather than repurpose
`FISICS_OVERLAY_IDE_METADATA`.

Initial overlay policy:

- default: disabled;
- `ide-metadata`: unchanged;
- `physics-units`: unchanged;
- `memory-check`: unchanged;
- `all`: unchanged while the metadata overlay was introduced;
- direct metadata syntax while the overlay is disabled: fail explicitly rather
  than silently discarding intended semantic meaning;
- ordinary C without semantic metadata: unaffected.

Adding `semantic-metadata` to `all`, aliasing it from `ide-metadata`, or
deprecating an existing spelling requires a later explicit compatibility
decision.

## 4. Canonical Source Syntax

The direct source spelling uses one fisiCs attribute per attribute specifier:

```c
[[fisics::id("thermal.read_battery_temperature")]]
[[fisics::domain("thermal")]]
[[fisics::role("sensor.read")]]
double read_battery_temperature(void);
```

The first contract does not accept a comma-separated collection inside one
`[[...]]` group. This restriction matches the current raw-payload attribute
representation and avoids prematurely redesigning the shared parser.

The implemented public compatibility header provides macros such as:

```c
FISICS_ID("thermal.read_battery_temperature")
FISICS_DOMAIN("thermal")
FISICS_ROLE("sensor.read")
double read_battery_temperature(void);
```

Those macros are a portability spelling only. They expand to the canonical
attribute form under fisiCs and to nothing under an unrelated C compiler. The
metadata subsystem is not designed around macro expansion. The complete public
contract is in `docs/extension_portability.md`.

## 5. Initial Vocabulary

### 5.1 Authored identity

```c
[[fisics::id("thermal.read_battery_temperature")]]
```

`id` supplies a durable authored semantic identity. It does not replace the
compiler's existing stable symbol ID.

Rules:

- exactly one string argument;
- singular on one declaration owner;
- canonical ID grammar applies;
- repeated `id` on the same declaration is invalid, even when identical;
- identical `id` values on compatible redeclarations may merge;
- conflicting `id` values on redeclarations invalidate the semantic record;
- uniqueness is translation-unit scoped until a strict project graph exists.

### 5.2 Domain

```c
[[fisics::domain("thermal")]]
```

`domain` gives the declaration one broad canonical semantic classification.

Rules:

- exactly one string argument;
- singular in the first contract;
- canonical name grammar applies;
- repeated use on one declaration is invalid;
- identical redeclaration values may merge;
- conflicting redeclaration values are invalid.

The initial registry should remain intentionally small. Proposed seed values:

```text
control
data_io
hardware
physics
rendering
simulation
storage
thermal
```

Registry additions are behavior changes and require tests. No suffix or symbol
name heuristics infer a domain.

### 5.3 Role

```c
[[fisics::role("sensor.read")]]
```

`role` gives the declaration one more specific semantic classification.

Rules match `domain`: one canonical string, singular per owner, compatible
identical redeclarations, and fail-closed conflicts.

Proposed seed values:

```text
control.decide
data.read
data.write
driver.initialize
persistence.read
persistence.write
render.integrator
sensor.read
simulation.integrator
telemetry.publish
```

The first registry is a proof vocabulary, not a universal ontology.

## 6. Canonical Name Grammar

Authored IDs, domains, roles, and later reference targets use canonical ASCII
names rather than silently normalized free text.

Canonical grammar:

```text
segment = [a-z][a-z0-9_]*
name    = segment ('.' segment)*
```

Additional first-contract rules:

- maximum encoded length: 255 bytes;
- no leading or trailing dot;
- no empty segment;
- no whitespace;
- no uppercase characters;
- no escaped or non-ASCII characters;
- no silent lowercasing or punctuation rewriting;
- diagnostics recommend the canonical form rather than mutating identity.

These restrictions make equality, ordering, JSON emission, and cross-tool
references deterministic.

## 7. Initial Attachment Targets

The first accepted targets are:

- function declarations;
- function definitions;
- file-scope variable declarations containing exactly one declarator.

Explicitly deferred targets:

- local variables;
- parameters;
- struct/union fields;
- typedefs;
- struct/union/enum definitions;
- enum constants;
- labels and statements;
- expressions;
- the translation unit itself.

The parser already accepts attributes in more positions than this initial
contract. Syntactic attachment does not imply semantic acceptance.

Metadata-bearing multi-declarator variable declarations are rejected in the
first contract:

```c
/* Deferred because one AST declaration node owns multiple symbols. */
[[fisics::domain("thermal")]] double inside, outside;
```

Users should write separate declarations:

```c
[[fisics::domain("thermal")]] double inside;
[[fisics::domain("thermal")]] double outside;
```

This avoids ambiguous per-symbol ownership until a declarator-level attachment
model is deliberately designed.

## 8. Parsing And Semantic Resolution

The current parser preserves a standard-style attribute as a syntax kind plus
one reconstructed payload string. The initial metadata resolver should parse
only exact, bounded forms such as:

```text
fisics::id("canonical.name")
fisics::domain("canonical.name")
fisics::role("canonical.name")
```

The argument grammar must not become a general C expression evaluator.

Recommended lifecycle:

1. Parse C and attach `ASTAttribute` records normally.
2. Run authoritative core semantic analysis and symbol binding.
3. Run a semantic-metadata AST walk from the existing extension semantic hook.
4. Recognize metadata-owned attribute names.
5. Validate target, arity, string form, and canonical spelling.
6. Bind accepted attachments to the declaration and resolved compiler symbol.
7. Merge compatible redeclarations and reject conflicts.
8. Store typed records in `CompilerContext` extension state.
9. Print debug state when `--dump-sema` is requested.

The resolver should not add parser-time semantic callbacks for every metadata
kind. A post-core-semantics walk naturally reaches attributed functions and
keeps syntax parsing separate from semantic interpretation.

## 9. Proposed Internal Record Boundary

The exact C structure is not frozen, but the first implementation needs the
equivalent of:

```text
owner AST node
owner compiler symbol when available
metadata kind
preserved source value
canonical value
owner source range
validation state
duplicate count
conflict state
```

Suggested validation states:

```text
pending
resolved
invalid
conflicted
```

The initial implementation may use the owner declaration range for diagnostics.
Precise attribute-token ranges are a later parser-provenance improvement.

The units arrays already present in `FisicsExtensionState` do not need to be
restructured before this work. Metadata may add a separately owned state table
without moving or generalizing stable units behavior.

## 10. Redeclaration Policy

Metadata must follow C redeclaration identity rather than raw AST occurrence
count.

For singular metadata kinds:

| Condition | Result |
| --- | --- |
| One valid occurrence | accept |
| Repeated on the same declaration | error |
| Same value on compatible prototype and definition | merge |
| Different values on compatible redeclarations | error; emit no accepted merged value |
| Metadata on incompatible C redeclarations | core C diagnostic remains authoritative; metadata does not repair it |

The graph/export layer must retain enough provenance to identify all conflicting
source occurrences, even if the first text diagnostic points to only one owner
range.

## 11. Declaration Relationship Contract

declaration relationship model adds:

```c
[[fisics::produces("signal.battery.temperature")]]
[[fisics::consumes("signal.fan.target")]]
```

These attributes are repeatable for distinct target identities and become
typed graph edges. They are authored semantic assertions: the compiler does not
infer them from function bodies or claim that runtime behavior fulfills them.

Initial edge kinds:

```text
PRODUCES
CONSUMES
```

Reserved future concepts such as `CONTROLS`, `USES_DEVICE`, `LOCATED_AT`,
`SIMULATES`, and `COMMUNICATES_VIA` are not implemented by declaration relationship model.

Each edge needs:

```text
source compiler symbol identity
source authored ID when present
edge kind
target canonical ID
resolution state
source location
```

### 11.1 Target identity meaning

A relationship target is only a canonical semantic identity. declaration relationship model does not
require or infer a target category from prefixes such as `signal.`, `device.`,
or `method.`. The same canonical-name grammar used by authored IDs applies to
relationship targets, but the name may describe any semantic concept.

Catalogs and later typed node schemas may classify a target explicitly. The
compiler must not infer signal, hardware, simulation, or runtime meaning from
the target's spelling.

### 11.2 Cardinality and duplicates

`produces` and `consumes` are repeatable for distinct targets:

```c
[[fisics::produces("signal.cabin.temperature")]]
[[fisics::produces("signal.cabin.humidity")]]
double read_cabin_environment(void);
```

Repeating the same edge kind and target on one declaration owner is an error,
not a silent deduplication:

```c
[[fisics::produces("signal.cabin.temperature")]]
[[fisics::produces("signal.cabin.temperature")]] /* duplicate relationship */
double read_cabin_temperature(void);
```

This policy catches accidental copy/paste while retaining repeatable
relationships as a set of distinct authored assertions.

### 11.3 Redeclaration merge policy

Relationships follow compatible C redeclaration identity:

| Condition | Result |
| --- | --- |
| Distinct relationships on compatible redeclarations | union into one deterministic edge set |
| Identical relationship on separate compatible redeclarations | merge into one edge |
| Identical relationship repeated on one declaration owner | diagnose as a duplicate relationship |
| Relationship on core-incompatible redeclarations | core C diagnostics remain authoritative |

The graph retains enough source provenance to explain merged or rejected
occurrences even when its accepted projection contains only one edge.

### 11.4 Source identity without an authored ID

An declaration relationship model relationship source does not require `fisics::id`. When an authored ID
is absent, the current internal dump uses an explicit stable tuple based on
symbol kind, the translation-unit source path as recorded by the compiler,
source line and column, and symbol name. translation-unit metadata export may map this to the existing
public frontend stable-symbol identity when that projection is exported.

Pointer values, allocation order, AST addresses, symbol-table bucket order, and
process-local hashes are never semantic identity. Authored IDs remain strongly
recommended for anything intended to cross translation-unit, catalog, IDE, or
runtime boundaries. A later strict project contract may require them without
retroactively changing isolated declaration relationship model analysis.

### 11.5 Resolution states

Reference resolution states are:

```text
resolved    target is present in the active resolution scope
external    target is valid but the current compile has no authority to resolve it
unresolved  strict resolution required a target and none was found
```

An isolated translation-unit compile may retain a canonical target as
`external`; it must not falsely claim resolution or fail merely because a
project catalog was not supplied. A spelling such as `signal.*` has no special
resolution behavior. A later strict project/catalog mode converts missing
required targets into `unresolved` diagnostics.

## 12. Derived Relationship Graph Contract

The graph is derived from accepted semantic records. It is not the parser
representation.

Logical node classes:

```text
source_symbol
semantic_identity
external_reference
```

Materialized declaration relationship model edge classes:

```text
PRODUCES
CONSUMES
```

Identity, domain, and role remain properties on the existing declaration-record
projection. A later normalized export may derive `HAS_ID`, `IN_DOMAIN`, and
`HAS_ROLE` edges without reparsing source. The declaration relationship model debug view prints declaration
records as graph sources and a separately sorted `SemanticMetadataGraph` edge
section rather than duplicating a standalone node list.

Graph source identity is based on canonical authored identity or the stable
compiler tuple described above. It never uses memory addresses.

## 13. Translation-Unit Metadata Export

Semantic metadata receives its own additive frontend/artifact lane. It is not
stored inside `FisicsUnitsAttachment`.

The authoritative frozen translation-unit metadata export shape, compatibility strategy, capability bit,
ownership rules, failure behavior, deterministic ordering, and CLI JSON schema
are defined in `semantic_metadata_export_contract.md`. In particular, translation-unit metadata export
uses a separately named size/version-tagged extended result rather than
appending fields to the caller-allocated legacy `FisicsAnalysisResult`.

The frozen projection contains source identity, optional authored identity,
stable-symbol linkage when available, symbol kind/range, optional domain and
role, typed edges, target resolution/index linkage, and relationship occurrence
provenance. The separately named extended frontend API is capability-gated;
the legacy result and entry points remain unchanged. CLI export is limited to
one translation unit and uses an atomic deterministic JSON writer.

The CLI and frontend must project from one shared ordered copy-out model. The
complete field layout, enum values, ordering keys, nullability rules, JSON key
order, and negative acceptance matrix live only in the dedicated translation-unit metadata export
contract.

## 14. Determinism Rules

For identical source bytes, normalized input identity, compiler version, and
options, the translation-unit metadata export CLI artifact must be byte-identical. The authoritative record
and edge sort keys and normative JSON key order are frozen in
`semantic_metadata_export_contract.md`. Consumers must still compare JSON
structurally when evaluating semantic equivalence.

## 15. Diagnostics Namespace

The metadata diagnostic reservation is `4200-4299` within
`FISICS_DIAG_CATEGORY_EXTENSION`.

Proposed initial meanings:

| Code | Proposed meaning |
| --- | --- |
| `4201` | semantic metadata used while overlay is disabled |
| `4202` | malformed metadata attribute or invalid canonical value |
| `4203` | metadata kind is not allowed on this target |
| `4204` | duplicate singular metadata on one declaration |
| `4205` | conflicting metadata across redeclarations |
| `4206` | duplicate authored ID in the active uniqueness scope |
| `4207` | required reference unresolved in strict resolution mode |
| `4208` | metadata/units contract mismatch |
| `4209` | duplicate relationship on one declaration owner |

Codes `4201-4206` and `4209` are implemented, registered in diagnostic metadata,
and covered by the focused contract lane. Codes `4207-4208` remain reserved and
are not emitted by the implemented offline semantic project graph/physical composition graph consumers, which
report their terminal validation failures without entering compiler analysis.

## 16. General Physical Composition

Semantic metadata answers:

> What does this symbol mean or relate to?

Physics units answer:

> What physical quantity does this value represent?

The implemented physical composition graph contract generalizes this bridge into a physical type with
orthogonal subject identity, quantity kind, dimension, unit policy, value form,
reference semantics, and optional frame identity. This avoids treating a
dimension vector as complete physical meaning and supports later hardware and
simulation consumers without embedding either lane in the compiler.

The physical composition graph composes the semantic project graph, new compiler-owned units evidence artifacts, and
strict author-owned physical contract catalogs. The metadata layer queries
units through a narrow read-only validation/export view. It does not duplicate
the dimension vector, concrete-unit registry, conversion rules, or units
diagnostics.

Compatibility states are scoped: `source_compatible`, `source_unverified`,
`external_declared`, and `uncovered`. Catalog facts are declarations rather
than observations, and physical acceptance is always `not_assessed` in physical composition graph.
The complete implementation boundary and focused proof are recorded in
`docs/semantic_metadata_physical_composition_contract.md`.

## 17. Translation-Unit And Project Metadata

Declaration attributes, translation-unit facts, and project topology remain
different contracts:

```text
attributes              source-symbol semantics
future #pragma fisics   translation-unit semantics
project/catalog files   project topology and external entities
```

No file-level pragma is part of the first implementation. The existing lexer
recognizes pragma tokens, but a dedicated preserved translation-unit semantic
contract still needs separate design and validation.

Project-global ID uniqueness and strict reference resolution require a
manifest-backed or graph-assembly operation. Compiling several translation units
independently and linking their objects is not sufficient evidence of a merged
semantic graph.

The implemented semantic project graph boundary is defined in
`docs/semantic_metadata_project_graph_contract.md`. It selects a dedicated
semantic-project manifest over changes to the build-manifest v0 schema,
consumes complete translation-unit metadata export artifacts, verifies them against current source bytes,
rejects project-global authored-ID collisions, resolves cross-TU authored
targets, permits only manifest-allowlisted external targets, and atomically
publishes one deterministic project graph through the paired graph-only CLI
options. It does not add file pragmas, linkage merging, units composition,
hardware catalogs, simulation behavior, or runtime effects.

## 18. Hardware Compatibility Boundary

The hardware compatibility graph is a static consumer of the implemented
physical composition graph. It remains a separate schema and validation owner.

The implemented hardware compatibility graph v1 catalog/topology contract owns only:

- device identity and model;
- board/node assignment;
- driver binding;
- bus, address, and pin allocation;
- signal contracts;

Physical placement, electrical limits, simulation-model identity, datasheet
trust, observations, and calibration remain later contracts rather than empty
hardware compatibility placeholders.

The first hardware canary is limited to one node, one bus, one sensor, one
actuator, and a few static checks. It does not generate drivers, execute CAN,
run a simulation, or claim physical acceptance.

The exact manifest, catalog, topology, graph, identity, duplicate, allocation,
signal-binding, evidence, CLI, and failure contracts are
implemented in `docs/semantic_metadata_hardware_contract.md`.

## 19. Explicit Non-Goals For The Initial Contract

- raw `@` syntax;
- arbitrary metadata keys;
- user-defined compiler metadata schemas;
- dynamic handler/plugin loading;
- expression metadata or propagation;
- inference from symbol spelling;
- local-variable, parameter, field, type, or statement metadata;
- translation-unit pragmas;
- project catalogs;
- cross-TU resolution in isolated compilation;
- frontend ABI changes through declaration relationship model;
- JSON graph export through declaration relationship model;
- code-generation attributes;
- runtime reflection;
- telemetry or persistence;
- hardware discovery or driver generation;
- simulation execution or calibration;
- IDE presentation;
- deployment, flashing, wiring, or physical-device proof.

## 20. First Contract Completion Statement

The first contract is complete only when this statement is true:

> With `semantic-metadata` explicitly enabled, fisiCs can attach one authored
> ID, one canonical domain, and one canonical role to a function or
> single-declarator file-scope variable, merge compatible redeclarations,
> diagnose malformed, duplicate, conflicting, disabled, or invalid-target use,
> and show deterministic compiler-owned records in `--dump-sema`, while leaving
> ordinary C, units behavior, the frontend ABI, and generated code unchanged.

This statement is implemented and covered by the semantic metadata overlay scaffold through deterministic semantic dump validation
lane.

## 21. Relationship Model Completion Statement

declaration relationship model is complete only when this statement is true:

> With `semantic-metadata` explicitly enabled, fisiCs accepts repeatable,
> distinct `produces` and `consumes` relationships on the existing supported
> declaration owners; diagnoses a duplicate identical relationship on one
> owner; unions compatible redeclaration relationships; uses deterministic
> compiler projection identity when the source has no authored ID; treats each
> target as a category-neutral canonical semantic identity; marks an in-TU
> authored target `resolved` and every other valid isolated target `external`;
> and shows a byte-deterministic internal graph through `--dump-sema`, without
> changing ordinary C, generated code, units behavior, frontend ABI, project
> resolution, hardware behavior, or runtime behavior.

This statement is implemented and covered by the focused declaration relationship model contract plus the
broad repository test gate.
