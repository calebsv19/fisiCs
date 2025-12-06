# Preprocessor

Macro handling, conditional skipping, and include resolution live here. This is a full C99-style preprocessing stage that transforms the lexer’s raw token buffer before the parser sees it.

## Files

- `macro_table.h/.c`
  - Owning structure for macros. Supports both object-like and function-like entries, tracks definition ranges, clones body tokens, and keeps a stack of active expansions so diagnostics can cite spelling vs expansion sites and guard against recursion.
  - API surface: `macro_table_create/destroy`, `macro_table_define_*`, `macro_table_lookup`, `macro_table_undef`, plus `macro_table_push_expansion` / `macro_table_pop_expansion` helpers.
- `macro_expander.h/.c`
  - Expansion engine that walks token streams, collects arguments, performs substitution, stringification (`#`), token pasting (`##`), and variadic handling (`__VA_ARGS__`). Each emitted token records both the macro body location and the call site so diagnostics can reference both.
- `pp_expr.h/.c`
  - Tiny expression evaluator for `#if` / `#elif`. Parses the limited preprocessor grammar (unary, multiplicative/additive, shifts, comparisons/equality, bitwise/logical ops, ternary), caps arithmetic to 32-bit results, folds `defined(name)` queries via the shared macro table, and treats all other identifiers as `0`.
- `preprocessor.h/.c`
  - High-level driver that scans the lexer’s token buffer, consumes `#define`/`#undef` directives into the macro table, evaluates `#if/#elif/#else/#endif` with a conditional stack, and flushes active chunks through the expander before handing them to the parser. A `--preserve-pp` flag or `PRESERVE_PP_NODES=1` keeps lightweight directive AST nodes when tooling needs them; otherwise the parser never sees `#` tokens.
- `include_resolver.h/.c`
  - Resolves quoted vs. system includes using `INCLUDE_PATHS` (make variable, defaults to `include`), caches file contents + mtimes, tracks `#pragma once` and classic guards, and records dependency edges for JSON emission.
  - Write the include graph to JSON via `include_graph_write_json`; surfaced to users with `--emit-deps-json <file>` or `EMIT_DEPS_JSON=path`.

## Usage notes
- All tokens carry spelling, macro call-site, and macro definition ranges; AST nodes inherit this so diagnostics can cite both sites.
- Unknown identifiers in `#if` expressions evaluate to `0`; `defined(name)` consults the shared macro table.
- The preprocessor owns include caching; the lexer only tokenizes a resolved file once. The include graph is attached to `CompilerContext` for downstream tooling.
- Dependency graph can be dumped to JSON for build tools/IDEs (`--emit-deps-json` or `EMIT_DEPS_JSON`).

## Tests
Run `make preprocessor-tests` to exercise stringify/paste, variadics, nested expansion, conditional skipping, include resolution (quoted vs. system, pragma once), directive preservation, and the `pp_expr` evaluator.

As new behaviours land, extend this README with their responsibilities and external APIs so downstream phases know how to call into the preprocessor layer.
