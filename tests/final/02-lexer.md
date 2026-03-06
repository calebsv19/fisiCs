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
   - Invalid character-literal escape sequences now fail closed in the lexer.
10) `02__binary_literals`
   - Binary literal tokenization (0b/0B).
11) `02__float_hex_no_exponent`
   - Strict rejection for hex-float spellings that omit the required `p` exponent.
12) `02__literal_semantics`
   - Literal value folding (hex/octal), multi-char constants, backslash-newline splicing, UTF prefixes.
13) `02__int_overflow_diag`
   - Integer literal overflow diagnostics.
14) `02__long_identifier`
   - Extremely long identifier tokenization.
15) `02__digraph_tokens`
   - Digraph token normalization (`<:`, `:>`).
16) `02__keyword_identifier_reject`
   - Keyword misuse rejected in a declarator context.
17) `02__trigraph_reject`
   - Explicit current rejection path for unsupported trigraph spelling.
18) `02__comments_and_splice`
   - Single-line comments, multi-line comments, and backslash-newline splicing.
19) `02__wide_utf_strings`
   - Wide and UTF-prefixed string tokenization.
20) `02__c11_keyword_tokens`
   - `_Static_assert`, `_Thread_local`, `_Atomic`, and `_Alignof` keyword tokenization.
21) `02__ucn_identifier_unsupported`
   - Explicit unsupported-policy rejection for UCN identifiers.
22) `02__identifier_matrix`
   - Basic ASCII, leading underscore, multi-underscore, and mixed alnum identifiers.
23) `02__keyword_matrix`
   - Broader keyword recognition matrix across control-flow, storage, and C11 spellings.
24) `02__float_edge_forms`
   - `.5`, `5.`, `5e+3`, and hex-float tokenization.
25) `02__digraph_pp_tokens`
   - Digraph punctuators plus `%:` directive spelling in token flow.
26) `02__wide_utf_chars`
   - Wide and UTF-prefixed character literal tokenization.
27) `02__comment_nested_like`
   - Nested-like block-comment patterns must tokenize deterministically.
28) `02__comment_macro_context`
   - Comments inside macro bodies should not corrupt expansion token flow.
29) `02__string_escape_matrix`
   - String escape spelling matrix for `\0`, `\n`, `\x`, `\t`, and trailing `\\`.
30) `02__operator_boundary_matrix`
   - Dense adjacent operator and punctuator boundary coverage.
31) `02__invalid_dollar_reject`
   - Invalid characters now fail closed with a lexer diagnostic.
32) `02__malformed_float_reject`
   - Malformed exponent forms now fail closed instead of tokenizing as numbers.
33) `02__unterminated_comment_reject`
   - Unterminated block comments now fail closed in the lexer.
34) `02__unterminated_string_reject`
   - Unterminated strings now fail closed in the lexer.
35) `02__empty_char_reject`
   - Empty character literals now fail closed in the lexer.
36) `02__unterminated_char_reject`
   - Unterminated character literals now fail closed in the lexer.
37) `02__float_suffix_matrix`
   - Floating suffix matrix for `f`, `F`, `L`, leading-dot, and hex-float suffix forms.
38) `02__char_invalid_hex_escape_reject`
   - Invalid `\x` character-literal escapes now fail closed in the lexer.
39) `02__char_invalid_u16_escape_reject`
   - Short `\u` character-literal escapes now fail closed in the lexer.
40) `02__char_invalid_u32_escape_reject`
   - Short `\U` character-literal escapes now fail closed in the lexer.
41) `02__invalid_at_reject`
   - Invalid `@` characters now fail closed with a lexer diagnostic.
42) `02__operator_relational_matrix`
   - Logical, relational, shift, arrow, and ellipsis token matrix coverage.
43) `02__int_invalid_suffix_reject`
   - Invalid integer suffix combinations now fail closed in the lexer.
44) `02__octal_invalid_digit_reject`
   - Invalid `8`/`9` digits in `0`-prefixed octal literals now fail closed.
45) `02__binary_invalid_digit_reject`
   - Invalid digits in `0b` binary literals now fail closed.
46) `02__invalid_backtick_reject`
   - Invalid backtick characters now fail closed with a lexer diagnostic.
47) `02__punctuator_matrix`
   - Brackets, parens, commas, braces, dot, and digraph punctuator coverage.
48) `02__hex_float_missing_mantissa_reject`
   - Hex floats with no mantissa digits now fail closed in the lexer.
49) `02__radix_prefix_missing_digits_reject`
   - `0x` / `0X` / `0b` / `0B` without digits now fail closed.
50) `02__string_invalid_hex_escape_reject`
   - Invalid `\x` string escapes now fail closed in the lexer.
51) `02__string_invalid_u16_escape_reject`
   - Short `\u` string escapes now fail closed in the lexer.
52) `02__string_invalid_u32_escape_reject`
   - Short `\U` string escapes now fail closed in the lexer.
53) `02__radix_case_matrix`
   - Uppercase radix prefixes and uppercase hex-float exponent spellings tokenize correctly.
54) `02__binary_float_forms_reject`
   - Binary literals with fraction or decimal exponent spellings now fail closed.
55) `02__int_suffix_case_matrix`
   - Broader valid integer suffix ordering and case combinations tokenize correctly.
56) `02__string_invalid_escape_reject`
   - Unknown string-literal escape sequences now fail closed in the lexer.
57) `02__escape_question_mark`
   - Standard `\?` escape remains accepted in both char and string literals.
58) `02__string_embedded_null`
   - Embedded `\0` string escapes preserve stable token spellings.
59) `02__long_string`
   - Long string literals tokenize as a single stable string token.
60) `02__operator_misc_matrix`
   - Additional unary and arithmetic operator coverage for `~`, `!`, `/`, `%`, `*`, and `&`.
61) `02__hex_bad_exponent_tail_reject`
   - Malformed hex-float exponents with trailing junk fail closed and capture the full bad token.
62) `02__punctuator_oddities`
   - Odd token boundaries like `::` and `?.` are tokenized deterministically.
63) `02__macro_identifier_boundary`
   - Macro token pasting that forms identifiers preserves stable token boundaries.
64) `02__percent_hashhash_digraph`
   - `%:%:` tokenizes deterministically as two `#` tokens in the current frontend.
65) `02__hex_suffix_tail_reject`
   - Hex-float suffix garbage after a valid exponent fails closed as an invalid numeric suffix.
66) `02__macro_keyword_boundary`
   - Macro token pasting that forms a keyword preserves stable reserved-token boundaries.

## Known Issue Repros (not yet active in suite)
67) `02__issue_c11_keywords`
   - Mixed C11 keyword behavior probe kept for manual investigation of
     supported (`_Static_assert`) versus explicitly unsupported
     (`_Thread_local`, `_Atomic`) semantics.
68) `02__issue_ucn_identifier`
   - Historical repro kept as the pre-policy record; active coverage now lives in
     `02__ucn_identifier_unsupported`.
69) `02__issue_invalid_dollar`
   - Historical pre-fix repro; active coverage now lives in
     `02__invalid_dollar_reject`.
70) `02__issue_malformed_float_forms`
   - Historical pre-fix repro; active coverage now lives in
     `02__malformed_float_reject`.
71) `02__issue_unterminated_comment`
   - Historical pre-fix repro; active coverage now lives in
     `02__unterminated_comment_reject`.

## Expected Outputs
- Token stream expectations (`.tokens`) for positive lexing cases.
- Diagnostic expectations (`.diag`) for explicit rejection or unsupported cases.
