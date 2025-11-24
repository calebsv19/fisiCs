# Semantic Analysis

Runs post-parse validation: builds symbol tables, checks scopes, and reports diagnostics. The pass walks the AST and dispatches into specialised analyzers for declarations, statements, and expressions.

## Phase overview

- `semantic_pass.h` / `semantic_pass.c`
  - Public entry `analyzeSemantics(ASTNode* root)` bootstraps the global scope, kicks off the analysis, and flushes accumulated errors.

- `analyze_core.h` / `analyze_core.c`
  - Central dispatcher `analyze(ASTNode*, Scope*)` creates new scopes for blocks/functions, routes nodes to declaration/statement/expression analyzers, and ensures child lists are traversed.

- `analyze_decls.h` / `analyze_decls.c`
  - Registers typedefs, variables, functions, structs, and enums into the current scope via `analyzeDeclaration()`, guarding against redefinitions.
  - Function declarations/definitions copy their parameter lists (and variadic flag) into the owning `Symbol`, so later passes have a full prototype for argument checking.
  - Struct/union/enum tags are now tracked with lightweight layout fingerprints so forward declarations can be matched against later definitions and conflicting layouts produce diagnostics instead of silently succeeding.
  - Variable and array declarations validate initializer shape: scalars require at most one element, aggregates demand brace-enclosed lists, array initializer counts are checked (including designated indices and char-array-as-string cases), and diagnostics fire for too many/few elements.
  - Typedef aliases and struct/union/enum tags are now also stored in the shared `CompilerContext`, so later strict parsing (casts, sizeof) recognises user-defined types discovered during analysis.
  - Walks initializer expressions (including designated/compound forms and array literals) so every identifier is resolved during bring-up.

- `analyze_stmt.h` / `analyze_stmt.c`
  - Validates control-flow structures (`if`, `for`, `while`, `switch`, `case`, `return`, etc.) and ensures nested scopes are established where needed.
- `control_flow.h` / `control_flow.c`
  - Lightweight flow-sensitive pass that runs after semantic binding to ensure non-void functions return on every path, flag unreachable statements after terminating control transfers, and warn about switch cases that fall through without an explicit `break`.

- `analyze_expr.h` / `analyze_expr.c`
  - Confirms identifiers resolve, checks operand shapes for binary/unary operations, and walks nested expressions (`array`, `call`, `member`, ternary, etc.).
  - Function-call analysis now matches argument counts/types against the stored prototype, emitting diagnostics for missing/excess arguments, qualifier drops, and incompatible conversions while applying default promotions to variadic arguments.
  - Enum values participate in arithmetic/comparison rules as integers while still carrying their tag metadata for compatibility checks.

- `type_checker.h` / `type_checker.c`
  - Primitive type-compatibility checks: `typesAreEqual()` and `canAssignTypes()`. Currently conservative but provides hooks for richer rules.

## Supporting infrastructure

- `scope.h` / `scope.c`
  - Hierarchical scope objects with `createScope`, `destroyScope`, `addToScope`, and `resolveInScopeChain`.
- `symbol_table.h` / `symbol_table.c`
  - Simple hash table storing `Symbol` objects (name, kind, `ParsedType`, AST definition). Shared by scopes.
- `syntax_errors.h` / `syntax_errors.c`
  - Aggregates diagnostics (`addError`, `reportErrors`, `freeErrorList`) so the pass can emit multiple issues in one run.

## Testing

Run `make semantic-typedef`, `make semantic-initializer`, `make semantic-undeclared`, `make semantic-bool` (or simply `make tests`) to validate typedef-driven casts, initializer analysis, boolean literals, and undeclared identifier reporting alongside the parser fixtures.

## Typical call flow

```
Scope* global = createScope(NULL);
analyze(programNode, global);         // analyze_core.c
// inside analyze():
//   -> analyzeDeclaration / analyzeStatement / analyzeExpression
//   -> scope utilities manage nested blocks
reportErrors();
destroyScope(global);
```
