# Compiler Context

This module owns global-type metadata the parser, preprocessor, and semantic analyzers consult while recognising declarations.

- `compiler_context.h` / `compiler_context.c`
  - `CompilerContext` tracks typedef names, struct/union/enum tags (with layout fingerprints), baked-in builtin-type names, include graph, and target triple/data-layout strings.
  - `cc_create()`/`cc_destroy()` manage the container; `cc_seed_builtins()` initialises the builtin list (includes common typedef widths like `size_t`, `ptrdiff_t`, `intptr_t`, etc.).
  - `cc_is_typedef()` / `cc_add_typedef()` answer whether an identifier has been introduced via `typedef`.
  - `cc_has_tag()` / `cc_add_tag()` mirror the same logic for tag namespaces (`struct`, `union`, `enum`), storing layout fingerprints and cached size/align when known.
  - `cc_set_target_triple()` / `cc_set_data_layout()` let CLI flags thread target info to codegen; `cc_set_include_graph()` exposes the preprocessor’s dependency graph for diagnostics or artifacts.
  - `cc_is_builtin_type()` checks the shared builtin table so the parser can prefer type interpretations during lookahead.

Internally the module uses growable sets/tables for interning names and tag metadata; the implementation is intentionally simple and can be swapped later without changing call sites.
