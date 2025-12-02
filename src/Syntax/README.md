# Semantic Analysis

Runs post-parse validation: builds symbol tables, checks scopes, and reports diagnostics. The pass walks the AST and dispatches into specialised analyzers for declarations, statements, and expressions. With the declarator overhaul, semantics treats the `ParsedType` derivation list as the authoritative description of every pointer/array/function suffix.

## Phase overview

- `semantic_pass.h` / `semantic_pass.c`
  - Public entry `analyzeSemantics(ASTNode* root)` bootstraps the global scope, kicks off the analysis, and flushes accumulated errors.

- `analyze_core.h` / `analyze_core.c`
  - Central dispatcher `analyze(ASTNode*, Scope*)` creates new scopes for blocks/functions, routes nodes to declaration/statement/expression analyzers, and ensures child lists are traversed. Declarations are now always `AST_VARIABLE_DECLARATION`; removed array/function-pointer node types are handled by inspecting the `ParsedType` derivations.

- `analyze_decls.h` / `analyze_decls.c`
  - Registers typedefs, variables, functions, structs, and enums into the current scope via `analyzeDeclaration()`.
  - Function prototypes copy their parameter lists (and variadic flag) into the owning `Symbol` so later passes have the full signature.
  - Struct/union/enum tags use lightweight layout fingerprints; array fields infer their shapes directly from the `ParsedType` derivations.
  - Variable/array declarations validate initializer shape by replaying the derivation stack: scalars require a single expression, aggregates demand brace-enclosed lists, arrays check length/designators (including string literal shortcuts), and variable-length arrays trigger diagnostics when declared at static storage.
  - Typedef aliases and tags update the shared `CompilerContext` so strict parsing (casts, sizeof) recognises newly analysed types.
  - Attributes are preserved on AST nodes and `ParsedType` entries so future passes can inspect them.

- `analyze_stmt.h` / `analyze_stmt.c`
  - Validates control-flow structures (`if`, `for`, `while`, `switch`, `case`, `return`, etc.) and ensures nested scopes are established where needed.
- `control_flow.h` / `control_flow.c`
  - Lightweight flow-sensitive pass that runs after semantic binding to ensure non-void functions return on every path, flag unreachable statements, and warn about fallthrough cases.

- `analyze_expr.h` / `analyze_expr.c`
  - Resolves identifiers, checks operand shapes for expressions, and leverages `typeInfoFromParsedType` to understand pointer/array/function combinations by replaying derivations. Function-call analysis matches argument counts/types, emitting diagnostics for too few/many arguments, qualifier drops, and incompatible conversions while applying default promotions to variadic arguments.

- `type_checker.h` / `type_checker.c`
  - Builds `TypeInfo` objects by replaying derivations, handles pointer/array/function equality without relying on `pointerDepth`, and exposes `typesAreEqual()` / `canAssignTypes()` for downstream use.

## Supporting infrastructure

- `scope.h` / `scope.c`
  - Hierarchical scope objects with `createScope`, `destroyScope`, `addToScope`, and `resolveInScopeChain`.
- `symbol_table.h` / `symbol_table.c`
  - Simple hash table storing `Symbol` objects (name, kind, `ParsedType`, AST definition). Shared by scopes.
- `semantic_model.[ch]`
  - Captures the analysed state and provides visitors used by codegen (struct metadata, resolved types, diagnostics).
- `syntax_errors.h` / `syntax_errors.c`
  - Aggregates diagnostics (`addError`, `reportErrors`, `freeErrorList`) so the pass can emit multiple issues in one run.

## Testing

Run `make semantic-typedef`, `make semantic-initializer`, `make semantic-undeclared`, `make semantic-bool` (or simply `make tests`) to validate declarator-driven type binding, initializer analysis, and diagnostics. Fixtures under `tests/spec/goldens/syntax/` assert on both AST dumps and semantic diagnostics; refresh them with `UPDATE_GOLDENS=1` after intentional output changes.
