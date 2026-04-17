# Lexer

Transforms the raw source buffer into `Token` objects with line tracking, ready for the parser.

## Files & responsibilities

- `lexer.h`, `lexer.c`, `lexer_token_handlers.c`, `lexer_internal.h`
  - `initLexer(Lexer*, const char*, const char*)` seeds the scanning state with both the raw buffer and the logical file path so span metadata can report filenames.
  - `lexer.c` keeps the lexer runtime coordinator: source translation, logical-location diagnostics, `getNextToken()`, `skipWhitespace()`, `isEOF()`, and token range construction.
  - `lexer_token_handlers.c` owns the token-family handlers: identifiers/keywords, numbers, strings, chars, preprocessor directives, comments, operators, punctuation, unknown-token recovery, and keyword-to-token mapping.
  - `lexer_internal.h` is the private bridge for handler modules to share `LexerMark`, token construction, and diagnostic helpers without widening the public lexer API.
- `tokens.h`
  - Declares the `TokenType` enum covering keywords, literals, operators, punctuation, and pseudo tokens.
  - Defines `SourceLocation`, `SourceRange`, and the full `Token` payload (`type`, interned `value`, legacy `line`, plus range metadata for diagnostics/macro tracing).
- `token_buffer.h` / `token_buffer.c`
  - Owns the one-time lexed stream (`TokenBuffer`) and exposes helpers to append tokens, peek with arbitrary lookahead, and fill the buffer directly from a `Lexer`.
- `keyword_lookup.h` / `keyword_lookup.c`
  - Thin wrapper around the gperf table; exposes `in_keyword_set()` used both by lookup and parser passes.
- `keywords.gperf`
  - Canonical list of C/C++ keywords fed into gperf to generate perfect-hash lookup code.

## How it fits

`main.c` reads the source via `Utils/readFile()`, then `initLexer()` seeds the cursor before filling a `TokenBuffer`. The parser consumes that buffered stream (rather than driving the lexer directly), so future preprocessing passes can rewrite/augment tokens without re-lexing the source. Keyword classification still happens up front, so higher phases can rely on `TokenType` alone for most control-flow decisions.
