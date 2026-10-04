# Semantic Project Graph Contract

Status: implemented and validated

Optional detached operation: [Frozen Offline Graph Inputs](frozen_graph_inputs.md)
preserves source identity while verifying receipts against captured source bytes.
The default remains live-source checking; snapshot mode never falls back to it.

Project manifest contract ID: `fisiCs.semantic_project_manifest`

Project graph contract ID: `fisiCs.semantic_project_graph`

Schema version: `1`

Input translation-unit contract: `fisiCs.semantic_metadata` schema `1`

Last audited against source: 2026-09-02

This document defines the implemented strict, multi-translation-unit semantic
graph boundary for `fisiCs`. The semantic project graph consumes complete translation-unit artifacts,
assembles one deterministic project graph, enforces project-wide authored
identity, and changes a missing target from a permissive single-file
`external` classification into either an explicitly permitted external target
or a project error.

The semantic project graph is an offline graph-assembly operation. It does not parse C, compile,
link, generate objects, mutate source, query hardware, or run a simulation.

## 1. Ownership Decision

The semantic project graph uses a dedicated semantic-project manifest. It does not add fields to the
existing `fisiCs.project` version `0` build manifest.

The separation is intentional:

- the existing build manifest owns source, object, linker, and build execution
  topology;
- the semantic-project manifest owns already-emitted translation-unit artifact membership
  and the explicit external-target boundary;
- the aggregator owns artifact validation, stale-source detection, global
  identity validation, strict edge resolution, ordering, and publication; and
- the declaration and relationship model plus its export projection remain authoritative for the
  meaning of each individual record and edge.

The aggregator must not reconstruct metadata from `--dump-sema`, reparse source
attributes, infer IDs from filenames, or keep several compiler semantic models
alive as an accidental project model.

## 2. Exact CLI Mode

The frozen command surface is:

```text
./fisics \
  --semantic-project-manifest <manifest-path> \
  --emit-semantic-project-graph-json <output-path>
```

Both options are required together. Each path must name a filesystem path;
`-` is rejected. This is a terminal graph-only mode and accepts no C or object
inputs.

The canonical output path must not equal the semantic-project manifest, any
input metadata artifact, or any verified source path. Such a collision is
rejected before a temporary output file is created.

The mode must reject combination with:

- `--build-manifest`, `--dry-run`, or `--emit-compile-db`;
- `-c`, `-o`, linker inputs, or compile/link options;
- `--emit-semantic-metadata-json`;
- AST, semantic-model, or IR dump options; or
- any option whose purpose requires a compiler context.

The mode does not require `--overlay=semantic-metadata`: its inputs are already
complete, versioned receipts proving that the overlay was active during each
translation-unit analysis. Supplying an overlay option in this graph-only mode
is rejected rather than silently ignored.

On success, the process writes exactly one graph artifact and exits zero. On
any manifest, input, validation, resolution, allocation, serialization, flush,
close, or replacement error, it exits nonzero and does not replace the output.

## 3. Semantic-Project Manifest V1

The manifest is UTF-8 JSON with this exact v1 shape:

```json
{
  "schema": "fisiCs.semantic_project_manifest",
  "schema_version": 1,
  "project_id": "van.control",
  "translation_units": [
    {
      "metadata_path": "build/semantic/controller.json"
    },
    {
      "metadata_path": "build/semantic/sensors.json"
    }
  ],
  "allowed_external_targets": [
    "device.maxxfan.deluxe",
    "signal.weather.outside_temperature"
  ]
}
```

Manifest rules:

- `schema` must equal `fisiCs.semantic_project_manifest` byte-for-byte.
- `schema_version` must be the JSON integer `1`.
- `project_id` must obey the existing canonical semantic-name grammar: at most
  255 ASCII bytes, lowercase letter at the start of each dot-separated segment,
  then lowercase letters, digits, or underscores.
- `project_id` is only canonical semantic identity. Its spelling does not
  assign a hardware, simulation, deployment, product, or filesystem category.
- `translation_units` must contain at least one object and every object has
  exactly one non-empty `metadata_path` string.
- Relative metadata paths resolve against the directory containing the
  semantic-project manifest. Absolute paths are accepted.
- Paths resolve relative to the manifest directory and then use canonical
  absolute filesystem identity before comparison and output. The implementation
  must not use the process working directory as a hidden identity input.
- Two entries resolving to the same normalized metadata path are an error.
- `allowed_external_targets` is required but may be empty. Every entry must
  obey the same canonical-name grammar, and duplicate entries are an error.
- Array order is not semantic. The aggregator sorts normalized inputs and the
  external-target set before validation/output.
- Duplicate object keys, unknown fields, trailing non-whitespace bytes,
  malformed JSON, invalid UTF-8, unsupported escapes, and wrong JSON types are
  errors in schema v1.
- Globs, directories, URLs, network fetches, inline source, inline metadata,
  environment-variable expansion, and recursive includes are not supported.

The manifest does not name C source files separately. Each translation-unit metadata export artifact owns
its analyzed source identity and digest, and semantic project graph verifies that provenance
against the current source bytes.

## 4. Accepted Translation-Unit Artifacts

Every `metadata_path` must decode as an translation-unit metadata export JSON artifact satisfying all of
these conditions:

1. `schema == "fisiCs.semantic_metadata"` and `schema_version == 1`;
2. `producer.name == "fisiCs"`;
3. `analysis_contract.id == "fisiCs.analysis.contract"` with a compatible
   major version of `1` and a version at least `1.9.0`;
4. `complete == true`;
5. every required field, enum, null rule, range, index, and canonical identity
   is valid under the translation-unit metadata export contract;
6. every locally `resolved` edge points to a record in that same artifact whose
   authored ID equals the edge target;
7. every locally `external` edge has a null target index;
8. every record source path and edge occurrence path equals the artifact's
   normalized `source.path`;
9. the artifact contains no duplicate record identity or duplicate canonical
   edge key after translation-unit metadata export ordering;
10. the normalized `source.path` is unique within the project input set; and
11. the current file at `source.path` exists, has exactly `length_bytes`, and
   hashes to the artifact's `hash_fnv1a64` value.

The source-byte check is mandatory in semantic project graph v1. There is no stale-artifact
bypass. A future packaging or remote-analysis mode requires a separate
contract; it must not weaken this local, source-grounded graph.

Input object-key order, indentation, and array order are not trusted as
semantic evidence. Valid inputs are normalized into the project ordering.
Source path, length, and hash are the project graph's content provenance; raw
manifest or metadata-artifact byte hashes are deliberately excluded because
JSON whitespace and input array order are not semantic.

The translation-unit metadata export objects are schema-exact inputs: duplicate keys, unknown fields, wrong
types, invalid enum strings, invalid numeric ranges, and trailing bytes are
rejected even when a permissive general-purpose JSON reader could decode them.
Record and edge indices must fit the implementation's `size_t`; source
coordinates must be positive and fit the translation-unit metadata export signed integer fields.

An empty translation-unit metadata export artifact is valid and remains a project member. It contributes no
records or edges but still receives a translation-unit index and provenance.

Initial resource ceilings are part of the fail-closed v1 reader:

- semantic-project manifest: 1 MiB;
- one translation-unit metadata export artifact: 64 MiB;
- total translation-unit metadata export artifact bytes: 256 MiB;
- translation units: 4,096;
- project records: 65,536;
- project edges: 262,144; and
- allowed external targets: 65,536.

Crossing a ceiling is a diagnosed input failure, never truncation. A later
implementation may raise a ceiling without changing schema meaning, but must
not silently lower one for the same published compiler version.

## 5. Project Identity And Duplicate Policy

The semantic project graph keeps three identities distinct:

- `project_id` identifies this semantic graph;
- normalized `source.path` identifies one translation unit; and
- each record's `source_identity` identifies one declaration record.

An authored ID is project-global in semantic project graph. If two input records contain the same
non-null `authored_id`, assembly fails, even if their symbol spelling, kind,
domain, role, or edges happen to match.

This strict rule is deliberate for v1. declaration relationship model already merges compatible
redeclarations within one translation unit. Cross-translation-unit declaration
coalescing would require exported C linkage and declaration/definition facts
that translation-unit metadata export schema v1 does not contain. semantic project graph must not guess that two same-named
symbols are the same C entity.

Records without an authored ID keep their opaque declaration relationship model/translation-unit metadata export fallback
`source_identity`. They are valid graph sources and remain distinct by their
source identity, but they cannot satisfy a relationship target. Relationship
targets resolve only to project-global authored IDs.

Duplicate fallback `source_identity` values are also errors. No project node
identity is based on an input array index, pointer, allocation order, basename,
or compiler symbol-table bucket.

## 6. Strict Relationship Resolution

The semantic project graph recalculates every edge against the complete project authored-ID index.
The translation-unit metadata export `resolution` value is translation-unit evidence, not the final project
answer.

For each edge, exactly one result is allowed:

1. **resolved**: exactly one project record has `authored_id == target_id`;
   the output edge carries that record's global index;
2. **external**: no project record has the target ID and the target appears in
   `allowed_external_targets`; the output target index is null; or
3. **missing required**: no project record has the target ID and it is not in
   the allowlist; assembly fails and no graph is published.

A target defined in another translation unit therefore changes from translation-unit metadata export
`external` to semantic project graph `resolved`. A target that was locally resolved must still
resolve to that same source record after global indexing; otherwise the input
artifact is inconsistent and assembly fails.

Self-edges and cycles are valid. `produces` and `consumes` are authored semantic
relationships, not execution dependencies, so graph topology alone must not
impose acyclicity, scheduling, causality, or runtime ordering.

Allowlisting a target means only that the project intentionally delegates that
semantic identity outside the assembled source graph. It does not prove that a
device, signal, driver, service, model, network peer, or runtime resource
exists. Unused allowlist entries are accepted and retained so manifests can be
reviewed independently of current edge use.

## 7. Deterministic Project Ordering

Translation units are ordered by:

1. normalized source path;
2. source hash; and
3. normalized metadata-artifact path.

Records are globally ordered by the existing translation-unit metadata export record keys:

1. normalized source file path;
2. start line and column;
3. symbol kind;
4. symbol name; and
5. source identity.

Each record receives its global `record_index` only after that sort. Its origin
contains the final translation-unit index and its canonical translation-unit metadata export record index
after applying the translation-unit metadata export record ordering, not its raw position in input JSON.

Edges are globally ordered by:

1. global source record index;
2. kind (`produces` before `consumes`);
3. target ID;
4. occurrence file;
5. occurrence start line and column; and
6. occurrence end line and column.

Each edge's source and resolved target indices are rewritten to the global
record index space. Its `local_edge_index` is likewise assigned after the
canonical translation-unit metadata export edge ordering. The external-target array is byte-sorted.
Filesystem enumeration order, manifest array order, JSON object order, locale,
hash-table iteration, addresses, and process scheduling must not affect output
bytes.

## 8. Project Graph JSON V1

The output uses UTF-8, two-space indentation, LF newlines, no trailing
whitespace, and exactly one final newline. Object key order is normative for
the v1 writer.

The top-level shape and key order are:

```json
{
  "schema": "fisiCs.semantic_project_graph",
  "schema_version": 1,
  "producer": {
    "name": "fisiCs",
    "version": "0.6.0"
  },
  "project": {
    "id": "van.control",
    "manifest_path": "/absolute/path/semantic-project.json"
  },
  "complete": true,
  "translation_units": [],
  "allowed_external_targets": [],
  "records": [],
  "edges": []
}
```

The producer version is illustrative and comes from the active compiler.
`complete` is always `true` because failed or partial graphs are not published.

Each translation-unit object has this key order:

```json
{
  "translation_unit_index": 0,
  "source": {
    "path": "/absolute/path/controller.c",
    "hash_fnv1a64": "0123456789abcdef",
    "length_bytes": 123
  },
  "metadata_artifact": {
    "path": "/absolute/path/controller.json"
  },
  "producer": {
    "name": "fisiCs",
    "version": "0.6.0"
  },
  "analysis_contract": {
    "id": "fisiCs.analysis.contract",
    "version": "1.9.0"
  },
  "record_count": 2,
  "edge_count": 1
}
```

Each project record retains the complete translation-unit metadata export record payload and adds its
global index and origin before those fields:

```json
{
  "record_index": 0,
  "origin": {
    "translation_unit_index": 0,
    "local_record_index": 0
  },
  "source_identity": "method.cabin.read",
  "authored_id": "method.cabin.read",
  "symbol_stable_id": "123456789",
  "symbol_name": "read_cabin",
  "symbol_kind": "function",
  "source": {
    "file": "/absolute/path/controller.c",
    "start_line": 3,
    "start_column": 1,
    "end_line": 7,
    "end_column": 2
  },
  "domain": "thermal",
  "role": "sensor.read"
}
```

Each project edge carries global indices and its original occurrence:

```json
{
  "source_record_index": 0,
  "kind": "produces",
  "target_id": "signal.cabin.temperature",
  "resolution": "resolved",
  "target_record_index": 2,
  "origin": {
    "translation_unit_index": 0,
    "local_edge_index": 0
  },
  "occurrence": {
    "file": "/absolute/path/controller.c",
    "start_line": 4,
    "start_column": 1,
    "end_line": 4,
    "end_column": 59
  }
}
```

An external edge uses `"resolution": "external"` and a null
`target_record_index`. Missing-required edges have no JSON representation
because they prevent publication.

## 9. Failure And Publication Contract

The graph builder is fail-closed. It publishes no partial graph and no
diagnostics-only graph. Failures include:

- invalid, duplicate, or unsupported manifest content;
- unreadable, duplicate, malformed, incomplete, or incompatible translation-unit metadata export inputs;
- missing or changed source bytes;
- invalid local indices, ranges, enum spellings, null rules, or identities;
- duplicate project authored IDs or fallback source identities;
- a missing required target;
- allocation or internal consistency failure; and
- output temporary-file, flush, close, or atomic-replacement failure.

The destination path is also rejected if it aliases the manifest, an input
artifact, or a verified source file.

Diagnostics must identify the manifest or artifact path and the failing field.
Identity and reference failures must include the canonical ID and, when known,
both source origins. Error text is not itself the machine contract in semantic project graph v1;
stable diagnostic codes may be added only through a later separately frozen
diagnostic schema.

Publication follows the translation-unit metadata export atomic rule: write a uniquely named temporary
file in the destination directory, finish serialization, flush and close,
atomically replace the destination, and remove temporary files on failure. An
existing destination remains byte-for-byte unchanged after any failed run.

## 10. Required Negative Guarantees

The semantic project graph must not:

- change the translation-unit metadata export JSON schema or public frontend ABI;
- extend or reinterpret the existing build-manifest v0 schema;
- compile source, emit objects, link, execute, flash, deploy, or contact a
  network service;
- accept `--dump-sema` text as input;
- trust stale source provenance or provide a bypass;
- infer project membership from directories, includes, linker symbols, or
  filename patterns;
- merge duplicate authored IDs across translation units by symbol-name guess;
- let fallback source identities satisfy authored relationship targets;
- infer target category or behavior from ID prefixes;
- infer relationships, units, hardware facts, physical placement, electrical
  constraints, device existence, runtime availability, or causality;
- treat the external allowlist as proof that an external entity exists;
- change code generation or object bytes; or
- overwrite a manifest, translation-unit artifact, or source file; or
- begin physical composition or hardware compatibility validation.

## 11. Acceptance Matrix

The semantic project graph implementation is complete only after focused tests prove:

| Case | Required outcome |
| --- | --- |
| Two valid artifacts, cross-TU target | one resolved edge with correct global target index |
| Missing target in allowlist | external edge with null target index |
| Missing target not in allowlist | nonzero exit; destination preserved |
| Duplicate authored ID across TUs | nonzero exit with both origins; destination preserved |
| Two records without authored IDs | distinct fallback nodes remain valid graph sources |
| Stale source length or hash | reject before graph publication |
| Missing source | reject before graph publication |
| Locally resolved edge with bad index/ID | reject malformed input artifact |
| Incomplete or wrong-version artifact | reject before aggregation |
| Duplicate source or artifact path | reject manifest/input set |
| Empty valid TU artifact | retain TU receipt with zero counts |
| Manifest and artifact arrays permuted | byte-identical graph output |
| Two identical runs | byte-identical graph output |
| Self-edge or cycle | accepted and represented without scheduling meaning |
| Unused external allowlist entry | accepted and retained |
| Output replacement failure | nonzero exit; old destination preserved; no temp residue |
| Output aliases manifest, artifact, or source | reject before any output mutation |
| Graph-only mode plus compile/build option | reject before compilation or output mutation |
| Ordinary compile before/after semantic project graph | unchanged object bytes and status |
| Existing build manifest v0 | behavior and schema unchanged |
| translation-unit metadata export frontend and JSON gates | unchanged and green |

## 12. Implementation Record And Stop Conditions

Implemented ownership seams are:

- `semantic_project_manifest.[ch]` owns the dedicated manifest model, parsing,
  path resolution, limits, and destruction;
- `semantic_metadata_json_reader.[ch]` owns strict translation-unit metadata export artifact ingestion and
  source-provenance verification;
- `semantic_project_graph.[ch]` owns global indexing, duplicate detection,
  strict relationship resolution, and graph destruction;
- `semantic_project_graph_json.[ch]` owns only deterministic serialization and
  atomic publication; and
- `main.c` owns only paired-option parsing, incompatibility checks, terminal
  dispatch, and user-facing failure reporting.

The manifest and artifact readers may share one small compiler-owned JSON
cursor/tokenizer, but neither may depend on the Shape library's vendored JSON
copy or enlarge the stable build-manifest v0 parser as a side effect. Any move
to a repository-wide JSON dependency is a separate dependency decision and
must be reviewed before semantic project graph.

The semantic project graph was implemented in the following order:

1. **[complete] Fixture and negative-behavior lock.** Added two valid translation-unit metadata export artifacts,
   cross-TU resolution, an external allowlist case, missing-required case,
   duplicate-ID case, stale-source case, exact graph golden, permuted-input
   golden, and preserved-destination sentinel. Stop if any fixture relies on
   fields absent from the frozen schemas.
2. **[complete] Dedicated manifest parser.** Added an owned parser/model/free path
   for `fisiCs.semantic_project_manifest` v1. Prove duplicate keys, unknown
   fields, wrong types, path normalization, duplicates, and canonical-name
   errors before loading artifacts. Do not alter `FisicsBuildManifest`.
3. **[complete] translation-unit metadata export artifact reader and verifier.** Parses into owned input
   structures, validate every field/index/range/version/null invariant, and
   verify current source length/hash. Stop before global
   resolution.
4. **[complete] Deterministic project index.** Sorts translation units and records,
   assign global indices, build the authored-ID index, and reject duplicate
   authored or fallback identity with both origins. Stop before publishing.
5. **[complete] Strict edge resolver.** Revalidates local resolution, resolves
   cross-TU authored IDs, classify explicit externals, and reject missing
   required targets. Prove cycles and self-edges remain accepted.
6. **[complete] Serializer and atomic publication.** Emits schema v1 solely from the
   validated owned graph, with normative key/order rules and destination
   preservation. No serializer may read compiler internals or input JSON
   fragments directly.
7. **[complete] CLI terminal-mode integration.** Adds the exact paired options,
   reject incompatible modes before compiler initialization, and make graph
   assembly the only effect of the invocation.
8. **[complete] Focused and broad proof.** Runs parser/manifest/artifact tests,
   exact golden and parity tests, repeatability, stale/failure/atomic tests,
   ordinary-object identity, existing build-manifest and translation-unit metadata export regression gates,
   then the repository confidence tier justified by the touched shared code.
9. **[complete] Documentation closeout.** Marks the contract implemented
   from passing focused and broad evidence. physical composition graph was pending at this closeout
   and is now implemented under its separate contract; hardware compatibility graph is likewise now
   implemented under its own static hardware contract.

Stop and return to contract review if implementation discovers a
need for any of the following: cross-TU C linkage merging, a changed translation-unit metadata export
schema, source-unavailable aggregation, generated metadata, wildcard discovery,
units lookup, hardware catalogs, diagnostic JSON, or runtime behavior. Each is
a new semantic boundary, not an incidental semantic project graph patch.

### Implementation evidence

The implementation was closed on 2026-09-02 with these repository gates:

```sh
make -j2
make integration-semantic-project-graph
make BUILD_PROFILE=sanitized -j2 integration-semantic-project-graph
make frontend-contract-test integration-cli-semantic-metadata \
  integration-semantic-project-graph build-manifest-contract-test \
  integration-build-manifest-dry-run-json \
  integration-build-manifest-compile-db integration-build-manifest-execute
make test
```

The focused graph gate covers the acceptance matrix above, including exact
golden bytes, repeatability, input permutation, source provenance, malformed
and oversized input, duplicate identity/path rejection, strict missing-target
behavior, self-edges, injected allocation/publication failures, destination
preservation, terminal CLI isolation, and ordinary-object identity. The broad
gate verifies that the existing compiler, translation-unit metadata export, and build-manifest surfaces
remain green. These are local implementation results, not release, deployed,
simulation, or physical-hardware acceptance evidence.

## 13. Completion Boundary

The semantic project graph is complete when a local project can assemble fresh
translation-unit metadata artifacts into one deterministic, strict semantic graph and
can prove every non-project target was explicitly allowed.

That completion establishes graph identity and reference integrity only. It
does not establish physical truth, dimensional compatibility, hardware
availability, driver correctness, simulation fidelity, deployment readiness,
or runtime acceptance.

The separately frozen consumer of this graph is
`docs/semantic_metadata_physical_composition_contract.md`. It composes semantic
project
identity with current-source units evidence and declared physical meaning
without changing this schema or retroactively assigning physical semantics to
semantic project graph target names.
