# Semantic Analysis

Runs post-parse validation: builds symbol tables, checks scopes, and reports diagnostics. The pass walks the AST and dispatches into specialised analyzers for declarations, statements, and expressions. With the declarator overhaul, semantics treats the `ParsedType` derivation list as the authoritative description of every pointer/array/function suffix.

## Phase overview

- `semantic_pass.h` / `semantic_pass.c`
  - Public entry `analyzeSemantics(ASTNode* root)` bootstraps the global scope, kicks off the analysis, and flushes accumulated errors.

- `analyze_core.h` / `analyze_core.c`
  - Central dispatcher `analyze(ASTNode*, Scope*)` creates new scopes for blocks/functions, routes nodes to declaration/statement/expression analyzers, and ensures child lists are traversed. Declarations are always `AST_VARIABLE_DECLARATION`; declarator shapes come from `ParsedType` derivations.

- `analyze_decls.h` / `analyze_decls.c`
  - `analyze_decls.c` is now the declaration coordinator: it dispatches variable/function/typedef/tag nodes through `analyzeDeclaration()`, keeps static-assert handling local, and wires the smaller declaration support lanes together.
  - `analyze_decls_types.c` owns typedef alias canonicalization, structural compatibility checks, layout fingerprints, array-bound evaluation, and enum-value helper logic.
  - `analyze_decls_signature.c` owns function parameter/signature normalization, storage/linkage validation, and interop attribute application.
  - `analyze_decls_aggregate_init.c` owns aggregate designated-initializer field lookup and recursive nested-field validation.
  - `analyze_decls_initializers.c` owns variable/array initializer validation, function-pointer initializer compatibility, constant-expression checks for static storage, and bitfield-width validation.

- `analyze_stmt.h` / `analyze_stmt.c`
  - Validates control-flow structures (`if`, `for`, `while`, `switch`, `case`, `return`, etc.) and ensures nested scopes are established where needed.
- `control_flow.h` / `control_flow.c`
  - Flow-sensitive pass to ensure non-void functions return on every path, flag unreachable statements, and warn about fallthrough cases.

- `analyze_expr.h` / `analyze_expr.c`
  - `analyze_expr.c` now owns the scalar/core expression coordinator: identifiers and literals, assignment, arithmetic/logical/bitwise binaries, unary operators, casts, and `sizeof`/`alignof`.
  - `analyze_expr_calls_aggregates.c` owns callable and aggregate-heavy expression semantics: function calls, `va_*` validation, member/array/deref access, compound literals, statement expressions, and ternary merge rules.
  - `analyze_expr_effects.c` owns optional unsequenced access/modification warnings and shared access-path tracking.
  - `analyze_expr_records.c` owns struct/union field lookup, bitfield/layout lookup, and `offsetof` path evaluation.
  - `analyze_expr_shared.c` owns cross-file expression utilities used by both the coordinator and aggregate/call lanes.

- `const_eval.h` / `const_eval.c`
  - Integer constant evaluator for literals, enum refs, sizeof/alignof (when complete), arithmetic/bitwise/logical/ternary, and casts. Drives enum folding, case labels, array sizes, and static initializers.

- `type_checker.h` / `type_checker.c` / `type_checker_arithmetic.c`
  - Builds `TypeInfo` by replaying derivations; handles pointer/array/function equality and qualifier comparison across pointer chains.
  - `type_checker.c` owns type replay, compatibility, and assignment checks.
  - `type_checker_arithmetic.c` owns integer/default promotions and usual arithmetic conversions.

- `layout.h` / `layout.c`
  - Computes size/align for structs/unions/arrays via ABI profiles (LP64 default, LLP64 optional), honors packed/aligned attributes, and exposes helpers to semantics/codegen. VLAs are treated as runtime-sized (layout queries fail for them so sizeof is lowered dynamically).

## Supporting infrastructure

- `scope.h` / `scope.c`
  - Hierarchical scopes with `createScope`, `destroyScope`, `addToScope`, and `resolveInScopeChain`.
- `symbol_table.h` / `symbol_table.c`
  - Hash table storing `Symbol` objects (name, kind, `ParsedType`, storage/linkage flags, const values). Shared by scopes.
- `semantic_model.[ch]` / `semantic_model_details.*`
  - Captures the analysed state (struct metadata, resolved types, diagnostics, include graph hooks) and feeds codegen.
- `syntax_errors.h` / `syntax_errors.c`
  - Aggregates diagnostics (`addError`, `addErrorWithRanges`, `reportErrors`) so the pass can emit multiple issues in one run.

## Testing

Run `make tests` or focused targets such as `make semantic-bitfields`, `semantic-const-expr`, `semantic-incomplete`, `semantic-layout-align`, `semantic-sizeof-layout`, `semantic-switch`, `semantic-varargs`, `semantic-storage`. Fixtures under `tests/spec/goldens/syntax/` assert on AST dumps and semantic diagnostics; refresh with `UPDATE_GOLDENS=1` after intentional output changes.
