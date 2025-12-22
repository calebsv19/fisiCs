# Semantic Analysis

Runs post-parse validation: builds symbol tables, checks scopes, and reports diagnostics. The pass walks the AST and dispatches into specialised analyzers for declarations, statements, and expressions. With the declarator overhaul, semantics treats the `ParsedType` derivation list as the authoritative description of every pointer/array/function suffix.

## Phase overview

- `semantic_pass.h` / `semantic_pass.c`
  - Public entry `analyzeSemantics(ASTNode* root)` bootstraps the global scope, kicks off the analysis, and flushes accumulated errors.

- `analyze_core.h` / `analyze_core.c`
  - Central dispatcher `analyze(ASTNode*, Scope*)` creates new scopes for blocks/functions, routes nodes to declaration/statement/expression analyzers, and ensures child lists are traversed. Declarations are always `AST_VARIABLE_DECLARATION`; declarator shapes come from `ParsedType` derivations.

- `analyze_decls.h` / `analyze_decls.c`
  - Registers typedefs, variables, functions, structs, and enums into the current scope via `analyzeDeclaration()`.
  - Storage/linkage rules: extern/static/register, tentative definitions, internal vs external linkage, duplicate-definition checks.
  - Struct/union/enum tags use layout fingerprints; the layout engine computes size/align (packed/aligned attributes, ABI profile). Array fields resolve constant lengths; VLAs inside structs are rejected.
  - Variable/array declarations validate initializer shape by replaying derivations: scalars require a single expression, aggregates demand brace-enclosed lists, arrays check length/designators (including string literal shortcuts), VLAs barred at static storage.
  - Enum members fold constant values; typedefs/tags update `CompilerContext`.

- `analyze_stmt.h` / `analyze_stmt.c`
  - Validates control-flow structures (`if`, `for`, `while`, `switch`, `case`, `return`, etc.) and ensures nested scopes are established where needed.
- `control_flow.h` / `control_flow.c`
  - Flow-sensitive pass to ensure non-void functions return on every path, flag unreachable statements, and warn about fallthrough cases.

- `analyze_expr.h` / `analyze_expr.c`
  - Resolves identifiers, checks operands, enforces deep pointer qualifier compatibility, pointer arithmetic rules, and lvalue/decay. Function calls match argument counts/types, apply default promotions for varargs, and validate `va_start`. Switch cases require integer const-eval; duplicates/default uniqueness are checked. Unary/deref restore pointee categories for arrays/struct members. Emits optional warnings for unsequenced modifications (e.g., `i = i++ + i`) and best-effort `restrict` aliasing violations on function calls.

- `const_eval.h` / `const_eval.c`
  - Integer constant evaluator for literals, enum refs, sizeof/alignof (when complete), arithmetic/bitwise/logical/ternary, and casts. Drives enum folding, case labels, array sizes, and static initializers.

- `type_checker.h` / `type_checker.c`
  - Builds `TypeInfo` by replaying derivations; handles pointer/array/function equality and qualifier comparison across pointer chains. Exposes `typesAreEqual()` / `canAssignTypes()` / arithmetic conversions.

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
