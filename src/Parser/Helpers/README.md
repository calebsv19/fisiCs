# Parser Helpers

Utilities shared by every parser module to keep the recursive-descent code lean.

## Files & roles

- `parser_helpers.h` / `parser_helpers.c`
  - Parser lifecycle: `initParser()`, `advance()`, and lightweight parser snapshot helpers (`cloneParserWithFreshLexer`, `freeParserClone`) to support speculative lookahead on the shared token buffer.
  - Token lookahead: `peekNextToken`, `peekTwoTokensAhead`, `peekThreeTokensAhead`.
  - Classification helpers: `isPrimitiveTypeToken`, `isModifierToken`, `isStorageSpecifier`, `isValidExpressionStart`, `getOperatorString`, `isAssignmentOperator`, `isPreprocessorToken`.
  - Error reporting via `printParseError(expected, parser)`.
- `parser_lookahead.h` / `parser_lookahead.c`
  - Predictive probes that inspect upcoming tokens without consuming them: `looksLikeTypeDeclaration`, `looksLikeCastType`, `looksLikeCompoundLiteral`.
  - `printParserState()` dumps the current token window during debugging.
- `parsed_type.h`, `parsed_type.c`, `parsed_type_ops.c`
  - `parsed_type.c` keeps parser-facing type-name parsing, aggregate/type-specifier handling, and declaration-vs-strict parsing policy.
  - `parsed_type_ops.c` owns the reusable `ParsedType` model helpers: cloning/freeing, derivation append/reset/query helpers, structural comparisons, function-pointer normalization, and array/pointer/function target transforms.
  - `getTokenTypeName()` maps lexer tokens back to readable strings for diagnostics, and `parseType(Parser*)` / `parseTypeCtx(Parser*, TypeContext)` remain the parser entry points.
- `designated_init.h` / `designated_init.c`
  - Builders for designated initialisers used in aggregate declarations (`createDesignatedInit`, `createIndexedInit`, `createCompoundInit`) plus `freeDesignatedInit`.

## How the parser uses them

`parser_main.c` and friends call the lookahead predicates before committing to declaration vs expression branches, and they rely on `ParsedType` to thread type information through AST construction. Function-pointer parsing creates clone parsers so invasive probes don't disturb the real token stream.
