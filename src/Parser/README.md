# Parser

Builds the compiler's AST from the lexer token stream. The parser mixes recursive-descent structure for declarations/statements with a pluggable expression engine (Pratt or legacy chain).

## Declarators & Parsed Types

Every declaration funnels through a shared `ParsedType` structure that records storage specifiers, qualifiers, the base type, and a derivation stack that captures pointer layers, array suffixes (with dimension expressions / VLA flags), and function signatures. Instead of emitting bespoke AST nodes for arrays or function pointers, the parser now records the full declarator grammar inside `ParsedType` so later passes (semantics/codegen) only need to replay the derivations.

The shared declarator engine now lives in a dedicated module so `AST_VARIABLE_DECLARATION` stays the single node shape for globals, locals, parameters, typedefs, and struct fields without forcing every declaration-routing path into one oversized source file.

## Module map

- `parser.h` — Core `Parser` struct (rolling lookahead tokens, mode flag, shared `CompilerContext`) plus the public `parse(Parser*)` entrypoint.
- `parser.c` — Thin shim that dispatches to `parseProgram()` defined in `parser_main.c`.
- `parser_main.h` / `parser_main.c`
  - Top-level orchestration: `parseProgram()` walks translation units, `parseBlock()` and `parseStatement()` route to specialised handlers.
  - Maintains the statement vector backing `AST_PROGRAM` and block nodes.
- `parser_stmt.h` / `parser_stmt.c`
  - Recognises control-flow statements (`if`, `for`, `while`, `switch`, `goto`, `return`, etc.), typedef/struct/enum handling, and general assignment forms via `parseAssignment()`.
- `parser_func.h` / `parser_func.c`
  - Function declarations/definitions (`parseFunctionDefinition`), parameter list assembly, function calls, and (legacy) function-pointer declarators. These will eventually call the shared declarator helper.
- `parser_array.h` / `parser_array.c`
  - Array initializer parsing helpers (designators, nested braces). Declarators themselves now build arrays directly via `ParsedType` derivations.
- `parser_declarator_core.c`
  - Shared declarator engine: pointer-chain parsing, grouped declarator normalization, function-suffix parsing, and the public `parserParseDeclarator` / `parserDeclaratorDestroy` helpers reused by declaration, function, statement, and expression parsing.
- `parser_switch.h` / `parser_switch.c`
  - Detailed parsing of `switch`/`case`/`default` constructs including fallthrough chains.
- `parser_preproc.h` / `parser_preproc.c`
  - (Removed) Preprocessor directive parsing helpers. Directives are now handled in the dedicated preprocessor stage before parsing.
- `parser_decl.h` / `parser_decl.c`
  - Handles declaration routing and tag bodies: type-specifier sequences, variable declaration lists, struct/union/enum/typedef parsing, designated initialisers, and attribute capture (`__attribute__`, `[[gnu::...]]`, `__declspec`), while delegating declarator mechanics to `parser_declarator_core.c`.
- `parser_config.h`
  - Defines `ParserMode` enum toggling between recursive and Pratt expression parsing.
- GNU statement expressions (`({ ... })`) can be enabled by setting `ENABLE_GNU_STATEMENT_EXPRESSIONS=1` before invoking the compiler or spec harness; when enabled they are parsed into dedicated `AST_STATEMENT_EXPRESSION` nodes that wrap a block and yield the last expression. When disabled the parser emits a diagnostic instead of silently consuming the construct.

### Recovery

- Panic-mode sync keeps parsing after an error: statement failures advance to the next `;`/`}` while declaration failures advance to the next `;` or likely type starter. The `tests/parser/recovery.c` fixture exercises this path to ensure multiple diagnostics appear from a single pass.

### Expression engine

- `Expr/` subdirectory (see its README) houses both the Pratt implementation (`parseExpressionPratt`) and the legacy recursive stack; `parseExpression()` in `Parser/Expr/parser_expr.c` selects the strategy.

### Helper stack

- `Helpers/` subdirectory provides cloning/advance utilities, lookahead predicates (`looksLikeTypeDeclaration`, `looksLikeCastType`, …), parsed-type structures, and designated-initialiser builders shared across all parsing files.
  - `parseTypeCtx` offers strict vs declaration contexts so speculative probes (`sizeof`, casts, lookahead) only treat known typedefs/tags/builtins as types.

## Testing

The root `tests/` directory contains small C snippets that exercise tricky parser paths (typedef chains, union declarations, designated initialisers, cast/sizeof expressions). Run `make tests` to execute every parser fixture or target them individually (e.g. `make typedef-chain`). Add a new test script alongside any new grammar feature to keep the AST output stable.

## External surface

- Entry: `initParser(&parser, &tokenBuffer, mode, ctx)`, then `parse(&parser)` returns an `ASTNode*` program.
- Statement-level helpers such as `handleControlStatements`, `parseAssignment`, and `parseStatement` are exported for reuse across modules.
- Declarator results are `ASTNode*` trees built via constructors defined in `AST/ast_node.c`. The `node->varDecl.declaredTypes` array holds per-declarator clones so semantics/codegen can read the exact `ParsedType` (including pointer/array/function derivations).
