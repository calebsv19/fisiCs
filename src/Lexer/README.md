# Lexer

Transforms the raw source buffer into `Token` objects with line tracking, ready for the parser.

## Files & responsibilities

- `lexer.h` / `lexer.c`
  - `initLexer(Lexer*, const char*)` seeds the scanning state.
  - `getNextToken()` drives the tokenisation loop: delegates to specialised handlers for identifiers/keywords, literals, directives, comments, operators, and punctuation. Debug logging is enabled by default to trace each decision.
  - `skipWhitespace()`, `isEOF()` form the low-level cursor control layer.
  - Handler helpers: `handleIdentifierOrKeyword`, `handleNumber`, `handleStringLiteral`, `handleCharLiteral`, `handlePreprocessorDirective`, `handleComment`, `handleOperator`, `handlePunctuation`, and `handleUnknownToken`.
  - Keyword lookup consults `lookupKeyword()` which wraps the gperf-generated trie (see below).
- `tokens.h`
  - Declares the `TokenType` enum covering keywords, literals, operators, punctuation, and pseudo tokens.
  - Defines the `Token` struct (`type`, interned `value`, `line`).
- `keyword_lookup.h` / `keyword_lookup.c`
  - Thin wrapper around the gperf table; exposes `in_keyword_set()` used both by lookup and parser passes.
- `keywords.gperf`
  - Canonical list of C/C++ keywords fed into gperf to generate perfect-hash lookup code.

## How it fits

`main.c` reads the source via `Utils/readFile()`, then `initLexer()` seeds the cursor. The parser owns a `Lexer*` and repeatedly calls `getNextToken()`, optionally peeking ahead via `Parser/Helpers`. Keyword classification happens before the parser sees the lexeme, so higher phases can rely on `TokenType` alone for most control-flow decisions.
