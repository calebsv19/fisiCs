# Translation-Unit Metadata Export Contract

Status: implemented

Contract ID: `fisiCs.semantic_metadata`

Schema version: `1`

Frontend analysis contract target: `fisiCs.analysis.contract` `1.9.0`

Last audited against source: 2026-09-02

This document defines the implemented machine-consumable export boundary for
translation-unit semantic-metadata records and relationships.

The export provides two consumers of one compiler-owned projection:

1. an opt-in, versioned extended frontend result; and
2. one deterministic CLI JSON artifact.

Both consumers must be populated from the accepted compiler-owned declaration
and relationship model.
Neither consumer may reparse source attributes or reconstruct relationships
from `--dump-sema` text.

## 1. Compatibility Decision

The export must not append fields to `FisicsAnalysisResult` or change the signature,
allocation size, ownership, or freeing behavior of `fisics_analyze_buffer` and
`fisics_free_analysis_result`.

`FisicsAnalysisResult` is allocated by its caller. Appending producer-written
fields would allow a new library to write beyond storage allocated by an older
compiled consumer. The existing API therefore remains the legacy-compatible
analysis entry point and never exposes the metadata arrays.

The implementation instead adds a separately named, size- and ABI-version-tagged extended
result API. Existing source and binaries continue using the original API.
New consumers must compile against the current frontend header and opt into the extended
entry point.

The analysis contract advances from `1.8.0` to the implemented `1.9.0`. Under
major version `1`, the semantic
metadata capability remains optional and authoritative. A `1.9.x` producer
without the capability is valid.

## 2. Public Frontend Constants And Enums

The implementation must add these public identities with these values:

```c
#define FISICS_ANALYSIS_RESULT_EX_ABI_VERSION 1u
#define FISICS_SEMANTIC_METADATA_SCHEMA_ID "fisiCs.semantic_metadata"
#define FISICS_SEMANTIC_METADATA_SCHEMA_VERSION 1u

FISICS_CONTRACT_CAP_EXTENSION_SEMANTIC_METADATA = (1ULL << 8)
```

No second capability bit is used for relationships. Records and edges are one
atomic v1 lane; a consumer that sees the capability understands both arrays.

Public enum values are frozen as:

```c
typedef enum {
    FISICS_SEMANTIC_METADATA_SYMBOL_FUNCTION = 1,
    FISICS_SEMANTIC_METADATA_SYMBOL_FILE_VARIABLE = 2
} FisicsFrontendSemanticMetadataSymbolKind;

typedef enum {
    FISICS_SEMANTIC_METADATA_EDGE_PRODUCES = 1,
    FISICS_SEMANTIC_METADATA_EDGE_CONSUMES = 2
} FisicsFrontendSemanticMetadataEdgeKind;

typedef enum {
    FISICS_SEMANTIC_METADATA_REFERENCE_EXTERNAL = 1,
    FISICS_SEMANTIC_METADATA_REFERENCE_RESOLVED = 2
} FisicsFrontendSemanticMetadataReferenceState;
```

Zero is intentionally not a valid emitted enum value. `UNRESOLVED` is not a
translation-unit export state; it remains reserved for strict project resolution
contract.

## 3. Public Record Shape

The public record shape is frozen as:

```c
typedef struct {
    const char* source_identity;
    const char* authored_id;

    uint64_t symbol_stable_id;
    bool has_symbol_stable_id;
    const char* symbol_name;
    FisicsFrontendSemanticMetadataSymbolKind symbol_kind;

    const char* source_file_path;
    int start_line;
    int start_col;
    int end_line;
    int end_col;

    const char* domain;
    const char* role;
} FisicsFrontendSemanticMetadataRecord;
```

Field rules:

- `source_identity` is always non-null and non-empty.
- `authored_id` is the accepted `fisics::id` value or null.
- When `authored_id` is present, `source_identity` has the same text.
- Without an authored ID, `source_identity` is the deterministic declaration relationship model fallback
  `symbol:<kind>:<normalized-path>:<line>:<column>:<symbol-name>`.
- `source_identity` is opaque. Consumers must not split the fallback form on
  colons or infer meaning from its spelling.
- `symbol_stable_id` is meaningful only when
  `has_symbol_stable_id == true`; otherwise it is zero.
- `symbol_name` and `source_file_path` are always non-null.
- the export emits only function and single-declarator file-scope-variable records.
- `domain` and `role` are null when absent. Empty strings are never emitted as
  substitutes for absent optional values.
- Source coordinates are one-based. A missing end coordinate is normalized to
  the start coordinate before export.
- Paths use the frontend contract's normalized absolute input identity.

Conflicted or invalid singular values are never copied into this structure.
Because any live translation-unit metadata error makes the compile unsuccessful, the whole
lane is suppressed under the fatal rule in section 7 rather than exporting a
misleading accepted subset.

## 4. Public Edge Shape

The public edge shape is frozen as:

```c
typedef struct {
    size_t source_record_index;
    FisicsFrontendSemanticMetadataEdgeKind kind;
    const char* target_id;
    FisicsFrontendSemanticMetadataReferenceState resolution;

    size_t target_record_index;
    bool has_target_record_index;

    const char* source_file_path;
    int start_line;
    int start_col;
    int end_line;
    int end_col;
} FisicsFrontendSemanticMetadataEdge;
```

Field rules:

- `source_record_index` always addresses an item in the same result's record
  array.
- `target_id` is always a non-null canonical semantic identity.
- `resolution == RESOLVED` requires `has_target_record_index == true`, and the
  indexed record's non-null `authored_id` must equal `target_id` byte-for-byte.
- `resolution == EXTERNAL` requires `has_target_record_index == false` and
  `target_record_index == 0`.
- The source range records the accepted relationship occurrence. When one edge
  is merged from compatible redeclarations, the export retains the first occurrence
  under the deterministic record/occurrence ordering defined in section 8.
- Edges do not own or imply runtime direction, transport, causality, units,
  hardware availability, or behavioral fulfillment.

## 5. Extended Result API

The additive frontend surface is frozen as:

```c
typedef struct {
    uint32_t abi_version;
    size_t struct_size;

    FisicsAnalysisResult analysis;

    FisicsFrontendSemanticMetadataRecord* semantic_metadata_records;
    size_t semantic_metadata_record_count;

    FisicsFrontendSemanticMetadataEdge* semantic_metadata_edges;
    size_t semantic_metadata_edge_count;
} FisicsAnalysisResultEx;

bool fisics_analyze_buffer_ex(const char* file_path,
                              const char* source,
                              size_t length,
                              const FisicsFrontendOptions* opts,
                              FisicsAnalysisResultEx* out);

void fisics_free_analysis_result_ex(FisicsAnalysisResultEx* result);
```

Caller protocol:

```c
FisicsAnalysisResultEx result = {0};
result.abi_version = FISICS_ANALYSIS_RESULT_EX_ABI_VERSION;
result.struct_size = sizeof(result);
```

The producer must validate both input fields before writing any other byte:

- unsupported `abi_version`: return `false`, leave all bytes except the two
  caller initialization fields unchanged;
- `struct_size` smaller than the complete ABI-v1 structure: same behavior;
- larger `struct_size`: accept it, write only the known ABI-v1 prefix, and
  leave the unknown suffix untouched.

The translation-unit metadata export does not provide a partially sized metadata structure. A future extension
must either use the safely advertised larger `struct_size` or introduce a new
ABI version.

The `FisicsFrontendSemanticMetadata*` prefix is deliberate. It keeps the public
copy-out ABI distinct from the existing compiler-internal
`FisicsSemanticMetadata*` model and prevents internal/public typedef collisions
or accidental ownership sharing.

`fisics_analyze_buffer_ex` shares the existing analysis pipeline and base-copy
logic. It must not run a second parse or semantic analysis merely to build
metadata. The embedded `analysis` result follows the existing frontend
contract, including diagnostics, symbols, includes, tokens, units attachments,
mode, source hash/length, and partial/fatal flags.

Once ABI version and size validation succeeds, the caller must invoke
`fisics_free_analysis_result_ex` after every call, including a `false` return,
because fail-closed paths may still return owned base diagnostics. No destructor
call is required when ABI validation itself rejects the untouched result.

## 6. Capability And Presence Rules

`FISICS_CONTRACT_CAP_EXTENSION_SEMANTIC_METADATA` is set in
`result.analysis.contract.capabilities` if and only if all are true:

1. the semantic-metadata overlay was enabled;
2. analysis is not fatal;
3. projection construction succeeded completely; and
4. at least one accepted metadata record exists.

When the capability is absent:

- `semantic_metadata_records == NULL`;
- `semantic_metadata_record_count == 0`;
- `semantic_metadata_edges == NULL`; and
- `semantic_metadata_edge_count == 0`.

Edges alone can never advertise the lane because every edge must reference a
source record. Valid identity/domain/role metadata may advertise a records-only
lane with a null, zero-length edge array.

The following inputs leave the capability absent:

- overlay disabled with no metadata syntax;
- overlay enabled but no metadata syntax;
- invalid-only metadata input;
- a fatal compile;
- allocation or projection failure.

The original `fisics_analyze_buffer` never advertises the capability because it
has no corresponding result storage.

## 7. Partial, Fatal, And Failure Behavior

The translation-unit metadata export preserves the base analysis result's existing meanings:

- `partial == false && fatal == false`: the metadata lane may be authoritative
  when advertised;
- `partial == true && fatal == false`: a future recoverable analysis may export
  the lane, but consumers must present it as incomplete;
- `fatal == true`: the metadata lane must be null, zero-length, and
  unadvertised even if internal accepted records were collected before the
  fatal error.

In the current frontend, failed analysis sets both `partial` and `fatal`. translation-unit metadata export
does not weaken that behavior.

Metadata projection allocation failure is fail-closed:

- return `false` from `fisics_analyze_buffer_ex`;
- set the embedded contract's `partial` and `fatal` fields to true after the
  base contract has been initialized;
- clear the semantic-metadata capability;
- free and zero both metadata arrays; and
- leave any already owned base diagnostic data releasable through the extended
  destructor.

Invalid metadata remains diagnostics-only. translation-unit metadata export does not export invalid values,
conflicted fields, rejected duplicate edges, or an "invalid graph" payload.

## 8. Projection Ordering And Merge Rules

Record ordering is frozen as:

1. normalized `source_file_path` byte order;
2. `start_line`;
3. `start_col`;
4. `symbol_kind` numeric value;
5. `symbol_name` byte order; and
6. `source_identity` byte order as a final total-order tie-break.

Edge ordering is frozen as:

1. `source_record_index`;
2. edge `kind` numeric value (`PRODUCES` before `CONSUMES`);
3. `target_id` byte order;
4. occurrence `source_file_path` byte order;
5. occurrence `start_line`;
6. occurrence `start_col`; and
7. occurrence end line and column.

The projection must sort copied public arrays; it must not reorder the
compiler's internal symbol table or extension state.

Compatible-redeclaration behavior remains the declaration relationship model rule:

- distinct relationships union;
- identical relationships on different compatible owners merge;
- identical relationships on one owner are diagnosed and rejected;
- merged edges retain the deterministically earliest occurrence.

Record and target indices are assigned only after record sorting. The same
ordered projection feeds both the frontend result and CLI JSON serializer.

## 9. Ownership And Destruction

The extended result owns:

- the embedded base analysis result under existing ownership rules;
- the record array;
- every non-null string in every record;
- the edge array; and
- every non-null string in every edge.

No public metadata string aliases compiler context storage, AST storage, symbol
storage, another public string, or a string in the embedded base result. This
deliberate no-alias rule makes freeing and consumer copying unambiguous.

`fisics_free_analysis_result_ex` must:

1. accept null;
2. call the existing base-result destructor for `analysis`;
3. free each record string, then the record array;
4. free each edge string, then the edge array;
5. zero both metadata pointers and counts; and
6. preserve `abi_version` and `struct_size` so a caller can safely inspect or
   reuse its initialized container.

Calling the extended destructor twice on the same initialized result must be
safe. Passing the embedded `analysis` member separately to
`fisics_free_analysis_result` and then calling the extended destructor is also
safe because the base destructor already nulls its owned fields.

## 10. CLI Surface

The exact translation-unit metadata export CLI option is:

```text
--emit-semantic-metadata-json <path>
```

The translation-unit metadata export CLI rules:

- the option is valid only with exactly one C input translation unit;
- `--overlay=semantic-metadata` must still be supplied explicitly;
- the path must be a filesystem path; `-` is rejected in translation-unit metadata export;
- the option may accompany `-c`, `--dump-sema`, and existing diagnostic JSON
  options;
- it does not suppress or replace normal compiler output;
- it does not assemble or resolve a multi-translation-unit project graph; and
- it never writes a semantic-metadata artifact for a failed/fatal compile, an
  invalid-only metadata input, or a projection/serialization failure.

If requested for a successful compile with no accepted metadata, the compiler
writes a valid empty artifact. This differs intentionally from the frontend
capability rule: an explicitly requested file is a useful deterministic receipt,
whereas an unpopulated in-memory optional lane remains unadvertised.

The writer must serialize to a temporary file in the destination directory,
flush and close it successfully, and atomically replace the destination only
after the entire document is valid. Any failure removes the temporary file and
leaves an existing destination unchanged.

For multi-input invocation, the driver fails before compilation and recommends
one output path per independent single-TU invocation. Project aggregation
belongs to the semantic project graph and must not be approximated through
filename suffixing in translation-unit export.

The implemented semantic project graph consumer contract is
`docs/semantic_metadata_project_graph_contract.md`. It leaves this schema and
frontend ABI unchanged and consumes complete schema-v1 files through a
dedicated graph-only manifest and CLI mode.

## 11. JSON Document Shape

JSON uses UTF-8, two-space indentation, LF newlines, no trailing whitespace,
and exactly one final newline. Object key order is normative for the v1 writer.

The top-level shape and key order are:

```json
{
  "schema": "fisiCs.semantic_metadata",
  "schema_version": 1,
  "producer": {
    "name": "fisiCs",
    "version": "0.6.0"
  },
  "analysis_contract": {
    "id": "fisiCs.analysis.contract",
    "version": "1.9.0"
  },
  "source": {
    "path": "/absolute/path/example.c",
    "hash_fnv1a64": "0123456789abcdef",
    "length_bytes": 123
  },
  "complete": true,
  "records": [],
  "edges": []
}
```

Rules:

- `producer.version` comes from the active compiler producer version; the value
  above is illustrative, not frozen to one compiler release.
- `analysis_contract.version` comes from the active analysis contract and must
  be at least `1.9.0` for schema v1.
- `source.path` is the normalized absolute input identity.
- `hash_fnv1a64` is exactly 16 lowercase hexadecimal digits representing the
  existing frontend source hash.
- `length_bytes` is the analyzed source byte length.
- `complete` is always `true` in an emitted translation-unit metadata export artifact because incomplete or
  fatal metadata artifacts are not written.

Record object key order is:

```json
{
  "source_identity": "method.cabin.read",
  "authored_id": "method.cabin.read",
  "symbol_stable_id": "123456789",
  "symbol_name": "read_cabin",
  "symbol_kind": "function",
  "source": {
    "file": "/absolute/path/example.c",
    "start_line": 3,
    "start_column": 1,
    "end_line": 7,
    "end_column": 2
  },
  "domain": "thermal",
  "role": "sensor.read"
}
```

`authored_id`, `symbol_stable_id`, `domain`, and `role` are JSON `null` when
absent. A stable ID is encoded as a decimal string, not a JSON number, so
JavaScript and other consumers cannot lose 64-bit precision. `symbol_kind` is
`function` or `file_variable`.

Edge object key order is:

```json
{
  "source_record_index": 0,
  "kind": "produces",
  "target_id": "signal.cabin.temperature",
  "resolution": "external",
  "target_record_index": null,
  "occurrence": {
    "file": "/absolute/path/example.c",
    "start_line": 4,
    "start_column": 1,
    "end_line": 4,
    "end_column": 59
  }
}
```

`kind` is `produces` or `consumes`. `resolution` is `resolved` or `external`.
`target_record_index` is a non-negative JSON integer only for a resolved edge;
otherwise it is null.

All strings use normal JSON escaping. No path, identity, or source spelling is
silently Unicode-normalized by the writer; determinism is over the exact
normalized path and accepted UTF-8/ASCII values supplied by the compiler.

## 12. Frontend And JSON Equivalence

For one successful input, the frontend and CLI projections must agree on:

- record and edge counts;
- array ordering;
- source identities;
- authored IDs, domain, and role presence;
- stable symbol ID presence and decimal value;
- symbol kinds and source ranges;
- edge kinds, targets, resolution states, and target indices; and
- producer, source hash, and source length provenance.

The JSON document is not a memory dump and is not required to be byte-identical
to an in-memory representation. One shared ordered projection builder must make
semantic divergence impossible.

`--dump-sema` remains a human/debug surface. Its text is not schema v1 and must
not be parsed by tooling when the translation-unit metadata export JSON option is available.

## 13. Required Negative Guarantees

The translation-unit metadata export must not:

- modify or append to `FisicsAnalysisResult`;
- change the legacy analysis/free function signatures;
- advertise metadata through the legacy result;
- expose internal `Symbol*`, `ASTNode*`, or compiler-context pointers;
- export conflicted, invalid, or rejected metadata;
- infer target categories from canonical-ID prefixes;
- claim external targets are unresolved errors;
- create project-global identity or relationship claims;
- combine multiple independently compiled translation units;
- export units inside the metadata schema;
- add hardware, driver, CAN, simulation, runtime, or physical-state facts;
- alter code generation or object bytes; or
- make `semantic-metadata` part of `--overlay=all`.

## 14. Acceptance Matrix

Implementation is complete only after all of these are proven:

| Case | Frontend result | CLI artifact |
| --- | --- | --- |
| Legacy API, valid metadata source | existing base result only; no metadata capability | not applicable |
| Extended API, overlay off, no syntax | null arrays; capability absent | option requires overlay and is rejected if requested |
| Extended API, overlay on, no syntax | null arrays; capability absent | valid deterministic empty artifact |
| Extended API, valid singular metadata | records populated; capability present | equivalent populated document |
| Extended API, relationship-only source | source record plus edge; capability present | equivalent populated document |
| Resolved in-TU target | target index present and valid | numeric target index and `resolved` |
| External target | no target index | null target index and `external` |
| Invalid-only metadata | fatal base diagnostics; lane absent | no artifact; destination preserved |
| Same-owner duplicate relationship | diagnostic `4209`; lane absent | no artifact; destination preserved |
| Projection allocation failure | API false, fatal/partial, owned base releasable | no artifact; destination preserved |
| Unsupported extended ABI version/size | API false; no out-of-bounds write | not applicable |
| Two identical runs | structurally identical arrays | byte-identical JSON |
| Ordinary C object with/without JSON request | identical object bytes | empty metadata receipt when requested |
| Multi-input CLI request | not applicable | fail before compile; no artifact |

Ownership tests must cover empty, records-only, records-plus-edges, every
nullable string combination, failure during each allocation layer, double
destruction, and destruction after base-result destruction.

## 15. Translation-Unit Metadata Export Completion Statement

The translation-unit metadata export is complete only when this statement is true:

> Without changing the existing frontend result or legacy entry points,
> `fisiCs` provides a versioned extended frontend result and one atomic,
> deterministic single-translation-unit JSON artifact derived from the same
> accepted semantic-metadata projection; advertises that in-memory lane only
> when valid records are present; preserves source, producer, stable-symbol,
> relationship, resolution, and occurrence provenance; owns and frees every
> exported byte explicitly; fails closed for invalid, fatal, incompatible, or
> allocation-failure cases; and makes no project, units, hardware, runtime,
> simulation, code-generation, or physical-acceptance claim.

This completion statement is satisfied by the implementation evidence recorded
in `semantic_metadata_implementation_plan.md`. It does not advance or imply
project aggregation.
