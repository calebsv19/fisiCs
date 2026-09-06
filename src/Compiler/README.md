# Compiler Context

This module owns global-type metadata the parser, preprocessor, and semantic analyzers consult while recognising declarations.

- `pipeline.h` / `pipeline.c`
  - Thin frontend/translation-unit coordinators for lex -> preprocess -> parse -> semantic analysis and optional codegen.
  - `compiler_run_frontend_internal()`, `compiler_run_frontend()`, and `compile_translation_unit()` now stay focused on orchestration, context seeding, include-path merging, forced-include wrapping, and final result ownership.
- `pipeline_internal.h`
  - Private bridge shared by the split pipeline support modules. It exposes the internal artifact/io helper entry points while keeping them out of the public `pipeline.h` surface.
- `pipeline_artifacts.c`
  - Owns IDE-facing compiler artifacts captured during frontend execution: top-level symbol harvesting, semantic-symbol expansion (including struct fields, enum members, and macros), and token-span classification/capture.
- `pipeline_io.c`
  - Owns file/command support for the pipeline: path/file loading helpers plus the external-preprocessor command builder/runner used by CLI compile mode.
- `build_graph.h` / `build_graph.c`
  - Owns Phase 1 project/tooling graph emission. The current v0 surface writes
    source-level `fisiCs.build_graph` JSON for `--emit-build-graph-json`,
    reusing `CompilerContext` include graph state plus compile-shape metadata
    from the CLI. It also writes manifest-backed dry-run graph/plan JSON from
    the local `fisiCs.project` parser and manifest-backed
    `compile_commands.json`.
- `build_manifest.h` / `build_manifest.c`
  - Owns the local-only `fisiCs.project` manifest parser. The current
    v0 parser is strict JSON readback for local project description metadata,
    translation units, defaults, optional link metadata, and resolved local
    paths. Minimal manifest execution currently lives in the CLI orchestration
    layer because it composes frontend compilation, object emission, and the
    host linker rather than changing compiler artifact serialization.
- `semantic_project_json.h` / `semantic_project_json.c`
  - Owns the small strict JSON DOM/parser shared only by the semantic-project
    manifest and translation-unit metadata readers. It rejects duplicate keys, malformed
    UTF-8/escapes, trailing data, and excessive nesting.
- `semantic_project_manifest.h` / `semantic_project_manifest.c`
  - Owns the dedicated `fisiCs.semantic_project_manifest` v1 model, strict
    parsing, canonical path resolution, explicit external-target allowlist,
    resource limits, and destruction. It is independent of build-manifest v0.
- `semantic_metadata_json_reader.h` / `semantic_metadata_json_reader.c`
  - Strictly ingests complete `fisiCs.semantic_metadata` schema-v1 artifacts into owned state,
    validates indices and invariants, and verifies source path, length, and
    digest against current bytes before aggregation.
- `semantic_project_graph.h` / `semantic_project_graph.c`
  - Owns deterministic translation-unit/record indexing, project-global
    authored-ID uniqueness, strict relationship resolution, resource limits,
    and graph destruction.
- `semantic_project_graph_json.h` / `semantic_project_graph_json.c`
  - Serializes the validated `fisiCs.semantic_project_graph` v1 model with
    normative ordering and atomic destination replacement. It cannot overwrite
    the manifest, input artifacts, or analyzed sources.
- `units_metadata_json.h` / `units_metadata_json.c`
  - Serializes deterministic single-translation-unit `fisiCs.units_metadata`
    v1 evidence with source provenance, registry fingerprint, stable binding
    identity, Dim8/unit data, and atomic destination replacement.
- `physical_project_manifest.h` / `physical_project_manifest.c`
  - Owns the strict `fisiCs.physical_project_manifest` v1 parser, canonical
    input paths, coverage/evidence policy selection, limits, and destruction.
- `physical_project_graph.h` / `physical_project_graph.c`
  - Strictly composes a semantic project graph, fresh units receipts, and physical
    catalogs; validates quantity/unit/reference/form/frame facets; preserves
    unbound and unverified evidence; and computes deterministic conversion
    recipes without executing hardware or simulation behavior.
- `physical_project_graph_json.h` / `physical_project_graph_json.c`
  - Publishes the validated `fisiCs.physical_project_graph` v1 in normative
    order with scoped truth states, exact binary64 strings, collision checks,
    and atomic destination preservation.
- `hardware_project_manifest.h` / `hardware_project_manifest.c`
  - Owns the strict `fisiCs.hardware_project_manifest` v1 parser, canonical
    input paths, evidence/required-port policy selection, alias rejection,
    resource limits, and destruction.
- `hardware_project_graph.h` / `hardware_project_graph.c`
  - Strictly consumes a physical composition graph, reusable component catalogs, and authored
    topology; resolves profiles and ownership; validates capabilities, driver
    contracts, allocation collisions, and physical-signal bindings; and keeps
    every runtime and real-hardware state unassessed.
- `hardware_project_graph_json.h` / `hardware_project_graph_json.c`
  - Publishes deterministic `fisiCs.hardware_project_graph` v1 state in
    normative order with raw input receipts and atomic destination
    preservation.
- `diagnostic_metadata.h` / `diagnostic_metadata.c`
  - Owns stable diagnostic metadata lookups for the diagnostics lane:
    severity/category/code/stage names plus reserved build/link/lexer/codegen
    code ranges. Writers can share this mapping without duplicating string
    tables in compiler, driver, or future JSON emitters.
- `compiler_context.h` / `compiler_context.c`
  - `CompilerContext` tracks typedef names, struct/union/enum tags (with layout fingerprints), baked-in builtin-type names, include graph, and target triple/data-layout strings.
  - `cc_create()`/`cc_destroy()` manage the container; `cc_seed_builtins()` initialises the builtin list (includes common typedef widths like `size_t`, `ptrdiff_t`, `intptr_t`, etc.).
  - `cc_is_typedef()` / `cc_add_typedef()` answer whether an identifier has been introduced via `typedef`.
  - `cc_has_tag()` / `cc_add_tag()` mirror the same logic for tag namespaces (`struct`, `union`, `enum`), storing layout fingerprints and cached size/align when known.
  - `cc_set_target_triple()` / `cc_set_data_layout()` let CLI flags thread target info to codegen; `cc_set_include_graph()` exposes the preprocessor’s dependency graph for diagnostics or artifacts.
  - `cc_is_builtin_type()` checks the shared builtin table so the parser can prefer type interpretations during lookahead.

Internally the module uses growable sets/tables for interning names and tag metadata; the implementation is intentionally simple and can be swapped later without changing call sites.
