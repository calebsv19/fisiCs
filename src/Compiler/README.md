# Compiler Context

This module owns global-type metadata the parser and semantic analyzers consult while recognising declarations.

- `compiler_context.h` / `compiler_context.c`
  - `CompilerContext` tracks three namespaces: typedef names, struct/union/enum tags, and baked-in builtin-type names.
  - `cc_create()`/`cc_destroy()` manage the container; `cc_seed_builtins()` initialises the builtin list (currently static literals).
  - `cc_is_typedef()` / `cc_add_typedef()` answer whether an identifier has been introduced via `typedef`.
  - `cc_has_tag()` / `cc_add_tag()` mirror the same logic for tag namespaces (`struct`, `union`, `enum`).
  - `cc_is_builtin_type()` checks the shared builtin table so the parser can prefer type interpretations during lookahead.

Internally the module uses a simple growable `CCStringSet` for interning names; the implementation is intentionally O(n) for clarity and can be swapped for a hash set later without touching call sites.
