# Preprocessor

Macro handling, conditional skipping, and include resolution live here. This is a full C99-style preprocessing stage that transforms the lexer’s raw token buffer before the parser sees it.

## Files

- `macro_table.h/.c`
  - Owning structure for macros. Supports both object-like and function-like entries, tracks definition ranges, clones body tokens, and keeps a stack of active expansions so diagnostics can cite spelling vs expansion sites and guard against recursion.
  - API surface: `macro_table_create/destroy`, `macro_table_define_*`, `macro_table_lookup`, `macro_table_undef`, plus `macro_table_push_expansion` / `macro_table_pop_expansion` helpers.
- `macro_expander.h/.c`
  - Expansion coordinator that walks token streams, performs substitution, stringification (`#`), token pasting (`##`), variadic handling (`__VA_ARGS__`), and builtin macro emission. Each emitted token records both the macro body location and the call site so diagnostics can reference both.
- `macro_expander_support.c` + `macro_expander_internal.h`
  - Private support seam for macro argument parsing/packing, token-buffer ownership helpers, and recursive argument expansion. Keeps the high-risk substitution/orchestration logic in `macro_expander.c` while isolating the lower-coupled storage and argument-prep machinery.
- `preprocessor_external.h/.c`
  - Private helper for hybrid/system-include external preprocessing. Builds the subprocess command, appends include search paths, captures tool output, strips stray directives that should not reach the parser, and hands filtered text back for re-lexing.
- `preprocessor_include_support.h/.c`
  - Private include-handling support seam for literal operand parsing, suspicious include-name filtering, classic guard detection, include-summary action/probe classification, and replay/profiling helpers used by recursive include dispatch. Keeps `preprocessor_directives.c` focused on directive execution while isolating the high-churn include-summary analysis lane.
- `preprocessor_driver_support.c`
  - Private driver-support seam for directive-line cloning, `_Pragma` skipping, chunk flush/expansion diagnostics, token-cost counting, and layout-debug gating shared by the main driver and summary replay lanes.
- `preprocessor_builtins.c`
  - Private builtin/bootstrap seam for predefined macro registration and CLI `-D` normalization/tokenization. Keeps `preprocessor.c` focused on lifecycle setup while isolating the target-profile and command-line macro lane.
- `preprocessor_summary_replay.c`
  - Private replay seam for include-summary router/scaffold/full replay, raw-range direct-clone rules, and replay-time conditional orchestration. Keeps the summary fast path out of the main token-scanning host.
- `pp_expr.h/.c`
  - Tiny expression evaluator for `#if` / `#elif`. Parses the limited preprocessor grammar (unary, multiplicative/additive, shifts, comparisons/equality, bitwise/logical ops, ternary), caps arithmetic to 32-bit results, folds `defined(name)` queries via the shared macro table, and treats all other identifiers as `0`.
- `preprocessor.h/.c`
  - High-level driver host that scans the lexer’s token buffer, routes directives and conditionals, and flushes active chunks through the expander before handing them to the parser. Builtin/bootstrap setup, shared flush helpers, and include-summary replay now live in sibling private seams so the owner file stays on lifecycle and orchestration. A `--preserve-pp` flag or `PRESERVE_PP_NODES=1` keeps lightweight directive AST nodes when tooling needs them; otherwise the parser never sees `#` tokens.
- `preprocessor_directives.c`
  - Directive handlers for `#define`, `#undef`, `#include`, `#pragma`, and `#line`. Include handling expands operands, resolves recursive/include-next flows, delegates operand/summary analysis to `preprocessor_include_support`, and delegates hybrid external system-header preprocessing to `preprocessor_external`.
- `include_resolver.h/.c`
  - Resolves quoted vs. system includes using `INCLUDE_PATHS` (make variable, defaults to `include`) plus optional `SYSTEM_INCLUDE_PATHS`, caches file contents + mtimes, tracks `#pragma once` and classic guards, records dependency edges for JSON emission, and understands `#include_next` by skipping the directory that produced the current header.
  - Write the include graph to JSON via `include_graph_write_json`; surfaced to users with `--emit-deps-json <file>` or `EMIT_DEPS_JSON=path`.

## Usage notes
- All tokens carry spelling, macro call-site, and macro definition ranges; AST nodes inherit this so diagnostics can cite both sites.
- Unknown identifiers in `#if` expressions evaluate to `0`; `defined(name)` consults the shared macro table.
- The preprocessor owns include caching; the lexer only tokenizes a resolved file once. The include graph is attached to `CompilerContext` for downstream tooling.
- Dependency graph can be dumped to JSON for build tools/IDEs (`--emit-deps-json` or `EMIT_DEPS_JSON`).

## Tests
Run `make preprocessor-tests` to exercise stringify/paste, variadics, nested expansion, conditional skipping, include resolution (quoted vs. system, pragma once), directive preservation, and the `pp_expr` evaluator.

As new behaviours land, extend this README with their responsibilities and external APIs so downstream phases know how to call into the preprocessor layer.
