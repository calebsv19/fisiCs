# Lexer Bucket Baseline Report

Date:

- March 2, 2026

Scope:

- First full baseline pass for the `lexer` bucket
- Current goal: establish what is already covered, what currently passes, what
  is unsupported, and what still lacks formal tests before any lexer fixes

This report is the first precedent-setting bucket report for the compiler test
upgrade effort.

## Current Suite Result

The existing `tests/final` lexer cases all pass under the current harness after
the fail-closed diagnostics batch.

Executed using the current metadata-driven suite (`tests/final/run_final.py`)
filtered per lexer test id.

Passing existing lexer cases:

- `02__int_literals`
- `02__float_literals`
- `02__char_literals`
- `02__string_concat`
- `02__operator_edges`
- `02__int_suffix_order`
- `02__float_exponent_forms`
- `02__char_unicode_escapes`
- `02__invalid_escape_char`
- `02__binary_literals`
- `02__float_hex_no_exponent`
- `02__literal_semantics`
- `02__int_overflow_diag`
- `02__long_identifier`
- `02__digraph_tokens`
- `02__keyword_identifier_reject`
- `02__trigraph_reject`
- `02__comments_and_splice`
- `02__wide_utf_strings`
- `02__c11_keyword_tokens`
- `02__identifier_matrix`
- `02__keyword_matrix`
- `02__float_edge_forms`
- `02__digraph_pp_tokens`
- `02__wide_utf_chars`
- `02__ucn_identifier_unsupported`
- `02__comment_nested_like`
- `02__comment_macro_context`
- `02__string_escape_matrix`
- `02__operator_boundary_matrix`
- `02__invalid_dollar_reject`
- `02__malformed_float_reject`
- `02__unterminated_comment_reject`
- `02__unterminated_string_reject`
- `02__empty_char_reject`
- `02__unterminated_char_reject`
- `02__float_suffix_matrix`
- `02__char_invalid_hex_escape_reject`
- `02__char_invalid_u16_escape_reject`
- `02__char_invalid_u32_escape_reject`
- `02__invalid_at_reject`
- `02__operator_relational_matrix`
- `02__int_invalid_suffix_reject`
- `02__octal_invalid_digit_reject`
- `02__binary_invalid_digit_reject`
- `02__invalid_backtick_reject`
- `02__punctuator_matrix`
- `02__hex_float_missing_mantissa_reject`
- `02__radix_prefix_missing_digits_reject`
- `02__string_invalid_hex_escape_reject`
- `02__string_invalid_u16_escape_reject`
- `02__string_invalid_u32_escape_reject`
- `02__radix_case_matrix`
- `02__binary_float_forms_reject`
- `02__int_suffix_case_matrix`
- `02__string_invalid_escape_reject`
- `02__escape_question_mark`
- `02__string_embedded_null`
- `02__long_string`
- `02__operator_misc_matrix`
- `02__hex_bad_exponent_tail_reject`
- `02__punctuator_oddities`
- `02__macro_identifier_boundary`
- `02__percent_hashhash_digraph`
- `02__hex_suffix_tail_reject`
- `02__macro_keyword_boundary`

Current failure count in existing lexer suite:

- `0`

Conclusion:

- The current lexer slice is stable for the cases already represented.
- The current lexer suite now includes 66 formal lexer cases.
- The bucket is not complete; the main issue is missing coverage density, not
  failing existing expectations.

## What The Existing Lexer Suite Actually Covers

Current validated areas:

- Integer literal base forms
- Integer suffix ordering
- Integer overflow diagnostics
- Decimal and hex float tokenization
- Float exponent forms
- Character literals and escape tokenization
- Unicode escape spelling inside character literals
- String literal concatenation
- Operator boundary handling for a limited set of operators
- Long identifier tokenization
- Basic identifier matrix coverage
- Digraph token normalization
- Digraph plus `%:` directive token flow
- Broader keyword recognition across control-flow/storage/C11 spellings
- Keyword misuse rejection in a declarator context
- Explicit rejection behavior for unsupported trigraph spelling
- Comment stripping and backslash-newline splicing
- Wide and UTF-prefixed string tokenization
- Wide and UTF-prefixed character literal tokenization
- Binary literal tokenization
- Strict rejection for hex-float spellings that omit the required `p` exponent
- Float boundary-form tokenization (`.5`, `5.`, `5e+3`, `0x1.fp3`)
- Some literal semantic behavior (folding, splice handling, UTF string prefixes)
- Explicit unsupported rejection for UCN identifiers
- Nested-like comment tokenization
- Macro-body comment handling during expansion token flow
- String escape spelling for `\0`, `\n`, `\x`, `\t`, and trailing `\\`
- Denser adjacent operator and punctuator boundaries
- Invalid-character rejection for `$`
- Malformed floating-exponent rejection
- Unterminated block comment rejection
- Unterminated string rejection
- Empty character literal rejection
- Unterminated character literal rejection
- Floating suffix matrix coverage (`f`, `F`, `L`, leading-dot, hex-float suffix)
- Invalid `\x`, short `\u`, and short `\U` character-literal rejection
- Invalid-character rejection anchored for `$`, `@`, and backtick
- Logical, relational, shift, arrow, and ellipsis token coverage
- Invalid integer suffix rejection
- Invalid octal digit rejection for `0`-prefixed integer literals
- Invalid binary digit rejection for `0b` integer literals
- Broader punctuator coverage for brackets, parens, commas, braces, dot, and
  digraph-normalized punctuators
- Hex-float mantissa rejection when no hex digits appear before or after `.`
- Radix-prefix rejection for bare `0x` / `0X` / `0b` / `0B` spellings
- String-literal malformed `\x`, short `\u`, and short `\U` escape rejection
- Uppercase radix prefix and uppercase hex-float exponent token coverage
- Binary-literal rejection for invalid float-like spellings
- Broader valid integer suffix ordering/case coverage
- Generic unknown-escape rejection for both character and string literals
- Positive standard-escape anchor for `\?`
- Embedded-null string token coverage
- Long-string stress coverage
- Broader unary and arithmetic operator token coverage
- Full-token malformed hex-exponent rejection coverage
- Odd punctuator boundary token coverage (`::`, `?.`)
- Macro-pasted identifier boundary coverage
- `%:%:` digraph spelling boundary coverage
- Hex-float suffix-tail rejection after valid exponents
- Macro-pasted keyword boundary coverage

This is a useful starting point, but it only covers part of the Phase 1
checklist.

## Manual Spot-Probe Results

I ran additional direct lexer probes with the current compiler to sample
uncovered checklist features.

### Confirmed Working

- Long ASCII identifiers tokenize correctly
- Digraphs are recognized and normalized (`<:` and `:>` became `[` and `]`)
- Keyword misuse as identifiers is rejected with diagnostics
- Single-line and multi-line comments are stripped correctly in token streams
- Backslash-newline splicing is applied correctly in token streams
- Wide and UTF string prefixes produce stable token spellings (`W|...`, `U8|...`)

### Confirmed Gap Or Unsupported Behavior

- Universal character names in identifiers are now explicitly rejected
- Example: `int \\u0061 = 1;` now emits a dedicated unsupported diagnostic and
  the compile fails closed before parse/codegen
- Full UCN decoding support is still not implemented

- Trigraphs are not translated during lexing
- Example: `??-` is not translated to `~` and is currently rejected
- This is now explicitly tracked by a formal negative test
  (`02__trigraph_reject`), so the unsupported behavior is documented instead of
  being implicit

- C11 keyword tokenization is now covered by an active suite case
- `_Static_assert`, `_Thread_local`, `_Atomic`, and `_Alignof` now lex as
  reserved keywords in the token stream
- Semantic policy is intentionally split:
  `_Static_assert` is supported, while `_Thread_local` and `_Atomic` are
  explicitly diagnosed as unsupported in the declarations bucket
- Unsupported declaration keywords now fail closed after their explicit
  diagnostic instead of degrading into normal declarations

Recent lexer fail-closed fixes landed in this batch:

- `$` is now rejected instead of being silently dropped
- Malformed exponent spellings (`1e+`, `1e`, `0x1.p`) now emit lexer errors
- Unterminated block comments now emit lexer errors
- Unterminated strings now emit lexer errors
- Empty and unterminated character literals now emit lexer errors

The remaining lexer gaps are now mostly coverage breadth and standards-policy
gaps, not these specific fail-open paths.

## Historical Repro Cases Retained

Historical repro files are still kept on disk as pre-fix records:

- `tests/final/cases/02__issue_c11_keywords.c`
- `tests/final/cases/02__issue_ucn_identifier.c`
- `tests/final/cases/02__issue_invalid_dollar.c`
- `tests/final/cases/02__issue_malformed_float_forms.c`
- `tests/final/cases/02__issue_unterminated_comment.c`

These are no longer the active oracles for the fixed behavior:

- They preserve the original failing inputs for historical reference
- They are not yet registered in `tests/final/meta/index.json`
- Active suite coverage now lives in dedicated reject-policy tests where
  applicable

## Harness Improvement Landed In This Bucket

The `tests/final` harness was improved during this lexer pass:

- `tests/final/run_final.py` can now capture front-end `Error` and `Warning`
  lines that appear before the `Semantic Analysis:` section
- That behavior is opt-in per test via metadata
- This makes negative lexer tests for parser-adjacent front-end failures
  testable in the metadata-driven harness
- The harness also suppresses the misleading `Semantic analysis: no issues
  found.` line when front-end errors were already emitted and capture is enabled

This is an infrastructure improvement, not just added test files, and it sets a
better precedent for later buckets.

## Broader Suite Result

After the lexer fixes, strict hex-float policy change, the char/suffix coverage
batch, the escape/operator breadth batch, and targeted golden
cleanup:

- All 66 formal lexer cases pass
- The full `tests/final` suite now passes

## Coverage Gaps Against The Checklist

The current suite is still missing formal tests for these major lexer areas:

### Identifiers

- Broader invalid-character coverage beyond the current `$`, `@`, and backtick
  anchors
- More identifier stress beyond the current long-identifier anchor
- Macro-driven identifier boundary interactions

### Keywords

- Full standard keyword recognition matrix by standard level
- Standard-level split for C99 versus supported C11/C17 additions
- Broader reserved-word misuse matrix beyond the current anchor tests

### Integer Literals

- Broader invalid suffix combination matrix beyond the current anchor case
- More boundary value coverage
- Broader octal edge coverage beyond the current invalid-digit anchor

### Floating Literals

- Broader suffix matrix beyond the current `f/F/L` anchors
- Broader malformed-float matrix beyond missing exponent digits and missing
  mantissa digits
- More invalid mixed-radix numeric combinations beyond the current binary-float
  anchors
- More punctuator completion beyond the current operator matrices and oddity
  anchors
- Additional preprocessor-token / digraph normalization can still be expanded,
  but the current lexer bucket now has a strong stopping baseline
- More positive radix-case coverage beyond the current uppercase anchor matrix

### Character And String Literals

- Wider UTF-prefixed character and string matrix coverage
- More malformed-character and malformed-string coverage beyond the current
  `\\x`, `\\u`, `\\U`, and generic unknown-escape active diagnostics
- Extremely long string stress case

### Operators And Punctuators

- Full unary and binary operator token matrix
- Wider digraph surface beyond brackets
- Wider trigraph surface beyond one explicit rejection case
- Wider punctuator boundary coverage beyond the current matrices

### Comments And Translation Phases

- Preprocessing-token stream before macro expansion

## Root-Cause Assessment

For this first bucket pass, the dominant issue is:

- Missing formal test coverage

Not:

- Existing golden mismatches
- Broad regressions in the current lexer cases

Concrete implementation gaps already observed:

1. UCN identifier decoding is not implemented; the current policy is explicit
   unsupported rejection
2. Trigraph translation is not implemented; it is now explicitly tracked as
   unsupported rather than silently untested
3. Identifier and keyword coverage density is still sparse beyond the current
   policy-anchor tests
4. The broader hex-float conformance surface still needs more matrix coverage
   beyond the current positive and negative anchors

These should become explicit tracked cases in the lexer suite instead of staying
as ad hoc observations.

## Recommended Next Implementation Batch

The next lexer expansion batch should focus on converting the biggest remaining
missing areas into formal `tests/final` cases.

Recommended order:

1. Add identifier and keyword matrix tests
   This closes the largest obvious blind spot in the current bucket.

2. Expand identifier and keyword matrix coverage around the new policy anchors
   `_Thread_local`, `_Atomic`, `_Static_assert`, and UCN identifiers now have
   defined behavior, but the surrounding matrix is still thin.

3. Continue expanding operator and string matrices
   The new breadth cases help, but the full operator and string surface is still
   not closed.

## Current Bucket Status

Current major-pass bucket status:

- `complete`

Reason:

- The first major lexer pass is now complete and stable
- Existing lexer tests are green
- Core fail-open behaviors have been converted to explicit fail-closed
  diagnostics where needed
- Unsupported behaviors are explicitly tracked instead of remaining implicit
- The lexer suite is now broad enough to serve as the baseline for later
  buckets
- Remaining lexer work is now incremental expansion, not a blocking gap

## Exit Condition For Closing The Lexer Bucket

The lexer bucket has satisfied the current major-pass exit condition:

- Core checklist areas now have formal anchors across valid, negative, and edge
  behavior
- UCN identifier handling is explicitly marked unsupported
- Trigraph handling is explicitly marked unsupported
- Comments and line-splicing have dedicated stable tests
- Malformed-literal and invalid-character classes have explicit policy tests
- Core malformed integer suffix and radix-digit failures have explicit
  fail-closed tests
- The full formal lexer bucket passes

Future lexer work can continue, but it should be treated as spot refinement,
not as a blocker for opening the next bucket.
