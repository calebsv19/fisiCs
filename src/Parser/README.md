# Parser

Builds the compiler's AST from the lexer token stream. The parser mixes recursive-descent structure for declarations/statements with a pluggable expression engine (Pratt or legacy chain).

## Module map

- `parser.h` — Core `Parser` struct (rolling lookahead tokens, mode flag, shared `CompilerContext`) plus the public `parse(Parser*)` entrypoint.
- `parser.c` — Thin shim that dispatches to `parseProgram()` defined in `parser_main.c`.
- `parser_main.h` / `parser_main.c`
  - Top-level orchestration: `parseProgram()` walks translation units, `parseBlock()` and `parseStatement()` route to specialised handlers.
  - Maintains the statement vector backing `AST_PROGRAM` and block nodes.
- `parser_stmt.h` / `parser_stmt.c`
  - Recognises control-flow statements (`if`, `for`, `while`, `switch`, `goto`, `return`, etc.), typedef/struct/enum handling, and general assignment forms via `parseAssignment()`.
- `parser_func.h` / `parser_func.c`
  - Function declarations/definitions (`parseFunctionDefinition`), parameter list assembly, function calls, and function-pointer declarators.
- `parser_array.h` / `parser_array.c`
  - Array declarators and array literal parsing helpers.
- `parser_switch.h` / `parser_switch.c`
  - Detailed parsing of `switch`/`case`/`default` constructs including fallthrough chains.
- `parser_preproc.h` / `parser_preproc.c`
  - Converts preprocessor directives into dedicated AST nodes (`#include`, `#define`, `#ifdef`, `#pragma once`, etc.) so later passes can reason about them.
- `parser_decl.h` / `parser_decl.c`
  - Handles type-specifier sequences, variable declarator lists, struct/union/enum bodies, and integrates designated initialisers.
- `parser_config.h`
  - Defines `ParserMode` enum toggling between recursive and Pratt expression parsing.
- GNU statement expressions (`({ ... })`) can be enabled by setting `ENABLE_GNU_STATEMENT_EXPRESSIONS=1` before invoking the compiler or spec harness; when enabled they are parsed into dedicated `AST_STATEMENT_EXPRESSION` nodes that wrap a block and yield the last expression.

### Expression engine

- `Expr/` subdirectory (see its README) houses both the Pratt implementation (`parseExpressionPratt`) and the legacy recursive stack; `parseExpression()` in `Parser/Expr/parser_expr.c` selects the strategy.

### Helper stack

- `Helpers/` subdirectory provides cloning/advance utilities, lookahead predicates (`looksLikeTypeDeclaration`, `looksLikeCastType`, …), parsed-type structures, and designated-initialiser builders shared across all parsing files.
  - `parseTypeCtx` offers strict vs declaration contexts so speculative probes (`sizeof`, casts, lookahead) only treat known typedefs/tags/builtins as types.

## Testing

The root `tests/` directory contains small C snippets that exercise tricky parser paths (typedef chains, union declarations, designated initialisers, cast/sizeof expressions). Run `make tests` to execute every parser fixture or target them individually (e.g. `make typedef-chain`). Add a new test script alongside any new grammar feature to keep the AST output stable.

## External surface

- Entry: `initParser(&parser, &lexer, mode, ctx)`, then `parse(&parser)` returns an `ASTNode*` program.
- Statement-level helpers such as `handleControlStatements`, `parseAssignment`, and `parseStatement` are exported for reuse across modules.
- Declarator results are `ASTNode*` trees built via constructors defined in `AST/ast_node.c`.
