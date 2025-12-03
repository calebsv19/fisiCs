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
- `parsed_type.h` / `parsed_type.c`
  - Data structure describing a fully-qualified C type: kind (primitive/struct/enum/etc.), modifiers, pointer depth, function-pointer metadata. Helper routines (`parsedTypeAddPointerDepth`, `parsedTypeSetFunctionPointer`, `parsedTypeFree`) manage ownership of sub-structures.
  - `getTokenTypeName()` maps lexer tokens back to readable strings for diagnostics.
  - `parseType(Parser*)` parses declarator contexts; `parseTypeCtx(Parser*, TypeContext)` toggles between declaration-friendly mode and strict mode used by lookaheads/casts to avoid treating unknown identifiers as types.
- `designated_init.h` / `designated_init.c`
  - Builders for designated initialisers used in aggregate declarations (`createDesignatedInit`, `createIndexedInit`, `createCompoundInit`) plus `freeDesignatedInit`.

## How the parser uses them

`parser_main.c` and friends call the lookahead predicates before committing to declaration vs expression branches, and they rely on `ParsedType` to thread type information through AST construction. Function-pointer parsing creates clone parsers so invasive probes don't disturb the real token stream.
