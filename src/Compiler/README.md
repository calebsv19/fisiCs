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
- `compiler_context.h` / `compiler_context.c`
  - `CompilerContext` tracks typedef names, struct/union/enum tags (with layout fingerprints), baked-in builtin-type names, include graph, and target triple/data-layout strings.
  - `cc_create()`/`cc_destroy()` manage the container; `cc_seed_builtins()` initialises the builtin list (includes common typedef widths like `size_t`, `ptrdiff_t`, `intptr_t`, etc.).
  - `cc_is_typedef()` / `cc_add_typedef()` answer whether an identifier has been introduced via `typedef`.
  - `cc_has_tag()` / `cc_add_tag()` mirror the same logic for tag namespaces (`struct`, `union`, `enum`), storing layout fingerprints and cached size/align when known.
  - `cc_set_target_triple()` / `cc_set_data_layout()` let CLI flags thread target info to codegen; `cc_set_include_graph()` exposes the preprocessor’s dependency graph for diagnostics or artifacts.
  - `cc_is_builtin_type()` checks the shared builtin table so the parser can prefer type interpretations during lookahead.

Internally the module uses growable sets/tables for interning names and tag metadata; the implementation is intentionally simple and can be swapped later without changing call sites.
