# Lexer Correctness

## Scope
Tokenization for all C99 literals, operators, and alternative tokens.

## Validate
- Integer and floating literal forms and suffixes.
- Character escapes, multi-char constants.
- String literal concatenation and prefixes.
- Digraphs and operator edge cases.

## Acceptance Checklist
- Integer literal bases and suffixes lex as a single token.
- Floating literal forms (decimal, hex) lex correctly with suffixes.
- Character literals handle escapes and multi-char constants.
- String literal concatenation produces the expected token sequence.
- Operator tokens are not split or merged incorrectly.

## Test Cases (initial list)
1) `02__int_literals`
   - Decimal, octal, hex, and suffixes.
2) `02__float_literals`
   - Decimal and hex floats with suffixes.
3) `02__char_literals`
   - Escapes, octal/hex escapes, multi-char constants.
4) `02__string_concat`
   - Adjacent string literal concatenation.
5) `02__operator_edges`
   - >>, >>=, ..., ->, ##, # token boundaries.
6) `02__int_suffix_order`
   - Mixed ordering of u/U and l/ll suffixes.
7) `02__float_exponent_forms`
   - Exponents with +/- and suffixes.
8) `02__char_unicode_escapes`
   - \\u and \\U escapes in character literals.
9) `02__invalid_escape_char`
   - Invalid escape sequences should still tokenize deterministically.
10) `02__binary_literals`
   - Binary literal tokenization (0b/0B).
11) `02__float_hex_no_exponent`
   - Hex float without exponent handling.

## Expected Outputs
- Token stream expectations (`.tokens`) for all cases.
