# Preprocessor

Macro handling and (soon) conditional/include logic live here. The directory gradually evolves into a full C99-style preprocessor that transforms the lexer’s raw token buffer before the parser sees it.

## Files

- `macro_table.h/.c`
  - Owning structure for macros. Supports both object-like and function-like entries, tracks definition ranges, clones body tokens, and keeps a stack of active expansions so diagnostics can cite spelling vs expansion sites and guard against recursion.
  - API surface: `macro_table_create/destroy`, `macro_table_define_*`, `macro_table_lookup`, `macro_table_undef`, plus `macro_table_push_expansion` / `macro_table_pop_expansion` helpers.
- `macro_expander.h/.c`
  - Expansion engine that walks token streams, collects arguments, performs substitution, stringification (`#`), token pasting (`##`), and variadic handling (`__VA_ARGS__`). Each emitted token records both the macro body location and the call site so diagnostics can reference both.
- `preprocessor.h/.c`
  - High-level driver that scans the lexer’s token buffer, consumes `#define`/`#undef` directives into the macro table, and flushes the remaining token chunks through the expander before handing them to the parser. A future flag will keep lightweight directive AST nodes when tooling needs them.

## Roadmap

- `macro_expander.*` (pending): argument collection, substitution, stringification (`#`), token pasting (`##`), variadic handling (`__VA_ARGS__`, `__VA_OPT__` stubs), and recursion control. Will consume tokens via the table API and emit expanded sequences.
- `pp_expr.*` (pending): preprocessor expression parser/evaluator used by `#if`/`#elif`.
- Include resolver & guard tracker: resolves `#include` search paths, caches headers, and records dependency edges for build integration.

As new modules land, extend this README with their responsibilities and external APIs so downstream phases know how to call into the preprocessor layer.
