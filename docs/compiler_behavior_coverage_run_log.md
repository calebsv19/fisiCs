# Compiler Behavior Coverage Run Log

This is the rolling execution log extracted from
`docs/compiler_behavior_coverage_checklist.md` to keep the checklist compact.

## Bucket Run Log

Use this section as the current campaign tracker. Add dated entries as buckets
are worked.

Entries are historical snapshots at the time they were recorded; older entries
may describe blockers that were resolved later.

Suggested entry format:

```md
### 2026-03-11 — Bucket: diagnostics-recovery (12)

- Scope: tighten diagnostics/recovery oracle to fail closed on parser-error paths
- Harness change:
  enabled `capture_frontend_diag: true` for malformed recovery fixtures that had
  been asserting only `"Semantic analysis: no issues found."`
  (`12__bad_paren_expr`, `12__bad_decl_recovery`, `12__extra_paren_recovery`,
  `12__typedef_recovery`, `12__decl_init_recovery`,
  `12__recovery__if_missing_rparen_then_followup_stmt`,
  `12__recovery__while_missing_lparen_then_followup_stmt`,
  `12__recovery__do_while_missing_semicolon_then_followup_stmt`,
  `12__recovery__goto_missing_label_token_then_followup_label`,
  `12__recovery__case_outside_switch_then_next_stmt_ok`)
- Updated expectations:
  regenerated `.diag` outputs for those cases and refreshed
  `12__missing_semicolon.diag` drift.
- Added probe coverage:
  `12__probe_while_missing_lparen_reject`,
  `12__probe_do_while_missing_semicolon_reject`,
  `12__probe_for_header_missing_semicolon_reject`,
  plus diagnostics-JSON probes
  `12__probe_diagjson_while_missing_lparen`,
  `12__probe_diagjson_do_while_missing_semicolon`,
  `12__probe_diagjson_for_header_missing_semicolon`.
- Current 12-probe status:
  `blocked=2`, `resolved=7`, `skipped=0`.
- Active blocker details:
  parser errors are visible in CLI output for malformed `while`/`do-while`, but
  `--emit-diags-json` returns empty diagnostics (`diag_count=0`) on those paths.
- Bucket sweep:
  all 46 `12__*` active tests currently pass in targeted smoke.

### 2026-03-02 — Bucket: lexer

- Scope: identifiers, keywords, integer literals
- Added tests: `02__identifiers__ascii_valid`, `02__keywords__identifier_reject`
- Passing: 12
- Failing: 4
- Unsupported: 1 (`universal character names`)
- Main failures:
  - integer suffix parsing rejects valid `ULL`
  - invalid octal literal accepted silently
- Fix batch status: pending
```

### 2026-03-11 — Bucket: diagnostics-recovery (12) follow-up

- Scope: close diagnostics JSON export blockers for statement-recovery parser errors
- Fix:
  `parseWhileLoop` and `parseDoWhileLoop` now record parser diagnostics through
  `compiler_report_diag` (code `CDIAG_PARSER_GENERIC`) when missing `(` after
  `while` and missing `;` after `do-while` are encountered.
- Verification:
  `PROBE_FILTER=12__probe_diagjson_` now resolves all three probes.
- Probe status after fix:
  full sweep is now `blocked=0`, `resolved=98`, `skipped=0`.
- Bucket sweep:
  `12` manifest smoke remains green (`46/46` passing).
- Promotion:
  promoted recovered JSON-export paths into active suite as
  `12__parserdiag__recovery_while_missing_lparen` and
  `12__parserdiag__recovery_do_while_missing_semicolon`
  (`tests/final/meta/12-diagnostics-recovery-wave7-parserdiag.json`).
- Updated bucket sweep:
  `12` manifest smoke now `48/48` passing.

### 2026-03-02 — Bucket: lexer

- Scope: baseline audit of all current lexer coverage in `tests/final`
- Existing formal lexer tests run:
  `02__int_literals`, `02__float_literals`, `02__char_literals`,
  `02__string_concat`, `02__operator_edges`, `02__int_suffix_order`,
  `02__float_exponent_forms`, `02__char_unicode_escapes`,
  `02__invalid_escape_char`, `02__binary_literals`,
  `02__float_hex_no_exponent`, `02__literal_semantics`,
  `02__int_overflow_diag`
- Passing existing tests: 13
- Failing existing tests: 0
- Manual probes confirmed:
  - long identifiers tokenize correctly
  - digraphs are recognized
  - keyword misuse as identifiers is rejected
- Manual probes found concrete gaps:
  - UCN identifiers are not decoded (`\\u0061` becomes identifier `u0061`)
  - trigraphs are not translated (`??-` lexes as `?`, `?`, `-`)
- Main bucket issue: coverage density is still incomplete even though the
  current represented slice passes
- Fix batch status: pending formal test additions for identifiers, keywords,
  comments, digraphs/trigraphs, and preprocessing-token edge cases

### 2026-03-02 — Bucket: lexer (first expansion batch)

- Harness improvement:
  `tests/final/run_final.py` now supports opt-in front-end `Error` and
  `Warning` capture for `.diag` expectations
- Added formal lexer tests:
  `02__long_identifier`, `02__digraph_tokens`,
  `02__keyword_identifier_reject`, `02__trigraph_reject`
- Current formal lexer tests passing: 17
- Current formal lexer tests failing: 0
- Explicitly tracked unsupported behavior:
  trigraph spelling is rejected and now has a formal negative test
- Still unresolved concrete bug:
  UCN identifiers are not decoded correctly and still need a formal policy test
- Main bucket issue now:
  many Phase 1 checklist areas still lack formal coverage even though the
  current represented slice is stable
- Fix batch status: next expansion should target identifier matrix, comments,
  wide/UTF strings, and unresolved UCN behavior

### 2026-03-02 — Bucket: lexer (broader suite smoke)

- Full `tests/final` suite run after the lexer work
- Result:
  lexer additions stayed green and the wider suite is reduced to 2 unrelated
  non-lexer mismatches
- Remaining non-lexer mismatches:
  - `04__array_param_qualifiers`
  - `09__goto_cross_init`
- Bucket conclusion:
  lexer work is stable enough to continue, and the remaining failures should be
  handled in their respective buckets instead of being folded into lexer scope

### 2026-03-02 — Bucket: lexer (second expansion batch)

- Added formal lexer tests:
  `02__comments_and_splice`, `02__wide_utf_strings`
- Added issue-targeted repro files (not yet active in suite):
  `02__issue_c11_keywords`, `02__issue_ucn_identifier`
- Current formal lexer tests passing: 19
- Current formal lexer tests failing: 0
- New confirmed stable areas:
  - comment stripping
  - backslash-newline splicing
  - wide/UTF string tokenization
- New confirmed issue cluster:
  - `_Alignof` is recognized
  - `_Static_assert` is tokenized as `TOKEN_UNKNOWN`
  - `_Thread_local` and `_Atomic` are tokenized as identifiers
  - UCN identifiers still lex as literal spellings (`u0061`)
- Full `tests/final` smoke after the second batch:
  still only 2 unrelated non-lexer mismatches remain
- Fix batch status:
  next lexer step should convert the C11 keyword and UCN repros into explicit
  policy tests once support or unsupported behavior is chosen

### 2026-03-02 — Bucket: lexer/declarations (C11 keyword policy pass)

- Added active suite coverage:
  `02__c11_keyword_tokens`,
  `04__static_assert_pass`,
  `04__static_assert_fail`,
  `04__static_assert_nonconst`,
  `04__thread_local_unsupported`,
  `04__atomic_unsupported`
- `_Static_assert` now parses into a real `STATIC_ASSERT` AST node and enforces
  integer-constant-expression semantics
- `_Thread_local` and `_Atomic` now lex as reserved keywords, emit explicit
  unsupported diagnostics, and fail closed before semantic/codegen
- Build-system hazard fixed:
  the makefiles now emit and include header dependency files so token enum
  changes no longer require a manual clean rebuild to avoid stale-object ABI
  mismatches
- Follow-up from this decision cluster:
  UCN identifiers were the next unresolved item and are now covered by the
  explicit unsupported-policy pass below
- Full `tests/final` suite result after this pass:
  still only the same 2 unrelated mismatches remain

### 2026-03-02 — Bucket: lexer (UCN unsupported policy)

- Added active suite coverage:
  `02__ucn_identifier_unsupported`
- `\u` / `\U` identifier spellings now emit a dedicated unsupported diagnostic
  during lexing and fail closed before parse/codegen
- This closes the prior silent-mislex behavior where `\u0061` degraded into
  identifier `u0061`

### 2026-03-02 — Bucket: lexer (matrix expansion batch)

- Added active suite coverage:
  `02__identifier_matrix`,
  `02__keyword_matrix`,
  `02__float_edge_forms`,
  `02__digraph_pp_tokens`,
  `02__wide_utf_chars`
- New stable coverage added for:
  basic identifier shapes, broader keyword recognition, float boundary forms,
  digraph plus `%:` directive flow, and wide/UTF character literal prefixes
- Full `tests/final` suite result after the batch:
  still only the same 2 unrelated non-lexer mismatches remain

### 2026-03-02 — Bucket: lexer (comments/operators breadth batch)

- Added active suite coverage:
  `02__comment_nested_like`,
  `02__comment_macro_context`,
  `02__string_escape_matrix`,
  `02__operator_boundary_matrix`
- New stable coverage added for:
  nested-like block comments, comments inside macro bodies, string escape
  spelling, and denser adjacent operator/punctuator boundaries
- Fix-first repros added but intentionally kept out of active metadata:
  `02__issue_invalid_dollar`,
  `02__issue_malformed_float_forms`,
  `02__issue_unterminated_comment`
- New concrete lexer bugs preserved for later repair:
  `$` silently drops out of tokenization, malformed float spellings are
  accepted, and unterminated block comments fail open
- Full `tests/final` suite result after the batch:
  still only the same 2 unrelated non-lexer mismatches remain

### 2026-03-02 — Bucket: lexer (fail-closed diagnostics batch)

- Lexer fixes landed for:
  invalid characters, malformed floating exponents, unterminated block
  comments, and unterminated strings
- Added active suite coverage:
  `02__invalid_dollar_reject`,
  `02__malformed_float_reject`,
  `02__unterminated_comment_reject`,
  `02__unterminated_string_reject`
- The corresponding lexer failures now stop compilation before preprocessor or
  parse, instead of degrading into silent token drops or parser fallout
- Full `tests/final` suite result after the batch:
  still only the same 2 unrelated non-lexer mismatches remain

### 2026-03-02 — Bucket: suite cleanup after lexer hardening

- Tightened `02__float_hex_no_exponent` to strict standard behavior:
  hex-float spellings without `p` now reject instead of tokenizing as valid
  literals
- Updated stale expectations for:
  `04__array_param_qualifiers` and `09__goto_cross_init`
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-02 — Bucket: lexer (char/suffix coverage batch)

- Added active suite coverage:
  `02__empty_char_reject`,
  `02__unterminated_char_reject`,
  `02__float_suffix_matrix`
- Malformed character literals now fail closed in the lexer instead of
  degrading into parser errors
- Float token coverage now explicitly includes `f`, `F`, `L`, leading-dot
  decimal, and hex-float suffix forms
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (escape/operator breadth batch)

- Added active suite coverage:
  `02__char_invalid_hex_escape_reject`,
  `02__char_invalid_u16_escape_reject`,
  `02__char_invalid_u32_escape_reject`,
  `02__invalid_at_reject`,
  `02__operator_relational_matrix`
- Character-literal bad-escape coverage now explicitly includes malformed `\x`,
  short `\u`, and short `\U` cases
- Invalid-character rejection is now anchored by both `$` and `@`
- Operator token coverage now explicitly includes logical, relational, shift,
  arrow, and ellipsis forms
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (numeric/punctuator hardening batch)

- Added active suite coverage:
  `02__int_invalid_suffix_reject`,
  `02__octal_invalid_digit_reject`,
  `02__binary_invalid_digit_reject`,
  `02__invalid_backtick_reject`,
  `02__punctuator_matrix`
- Integer literal failures that previously tokenized ambiguously now fail closed:
  invalid suffix tails, invalid octal digits, and invalid binary digits
- Invalid-character rejection is now also anchored by a backtick case
- Punctuator coverage now explicitly includes brackets, parens, commas, braces,
  dot, and digraph-normalized bracket/brace spellings
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (numeric conformance tightening batch)

- Added active suite coverage:
  `02__hex_float_missing_mantissa_reject`,
  `02__radix_prefix_missing_digits_reject`
- Hex-float spellings with no mantissa digits (for example `0x.p1`) now fail
  closed in the lexer instead of being accepted as valid float literals
- Bare radix prefixes (`0x`, `0X`, `0b`, `0B`) now fail closed instead of
  tokenizing as incomplete number literals
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (string escape and radix-case batch)

- Added active suite coverage:
  `02__string_invalid_hex_escape_reject`,
  `02__string_invalid_u16_escape_reject`,
  `02__string_invalid_u32_escape_reject`,
  `02__radix_case_matrix`
- String literals now fail closed for malformed `\x`, short `\u`, and short
  `\U` escapes instead of leaking into invalid-character or UCN-identifier
  fallout
- Positive token coverage now explicitly anchors uppercase `0X`, uppercase
  `0B`, and uppercase `P` hex-float exponent spellings
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (mixed-radix and suffix stress batch)

- Added active suite coverage:
  `02__binary_float_forms_reject`,
  `02__int_suffix_case_matrix`
- Binary-prefixed literals with fractional or decimal-exponent spellings (for
  example `0b1.0` and `0b1e2`) now fail closed instead of being accepted as
  float literals
- Positive token coverage now includes a broader valid integer suffix matrix
  across uppercase/lowercase and ordering boundaries
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (generic escape strictness batch)

- Tightened existing coverage:
  `02__invalid_escape_char` now expects a fail-closed diagnostic instead of a
  permissive tokenized char literal
- Added active suite coverage:
  `02__string_invalid_escape_reject`,
  `02__escape_question_mark`
- Unknown escape sequences like `\q` now fail closed in both character and
  string literals
- Standard `\?` is explicitly preserved as a valid escape so the stricter
  policy does not over-reject
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (string stress and operator completion batch)

- Tightened existing coverage:
  `02__binary_float_forms_reject` now treats `0b1p2` and `0b1P2` as the same
  binary-float policy error instead of a generic invalid-suffix fallback
- Added active suite coverage:
  `02__string_embedded_null`,
  `02__long_string`,
  `02__operator_misc_matrix`
- String coverage now includes dedicated embedded-`\0` and long-literal anchors
- Operator coverage now explicitly adds prefix `~`, prefix `!`, divide,
  modulo, unary `*`, and unary `&`
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (hex-tail and boundary oddities batch)

- Tightened existing coverage:
  malformed floating exponents now consume trailing identifier junk before
  diagnosing, so bad tokens like `0x1.pz` and `0x1p+z` are reported in full
- Added active suite coverage:
  `02__hex_bad_exponent_tail_reject`,
  `02__punctuator_oddities`,
  `02__macro_identifier_boundary`
- Token boundary coverage now explicitly includes `::`, `?.`, and macro-pasted
  identifier formation (`a##b -> hello`)
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (final small-edge anchors)

- Added active suite coverage:
  `02__percent_hashhash_digraph`,
  `02__hex_suffix_tail_reject`,
  `02__macro_keyword_boundary`
- Token boundary coverage now explicitly includes `%:%:` digraph spelling and
  macro token-paste that forms a reserved keyword (`return`)
- Hex-float suffix garbage after a valid exponent now has an explicit negative
  oracle (`0x1p2foo`, `0x1.P+2bar`)
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-03 — Bucket: preprocessor (prep baseline)

- Lexer is now treated as the first completed major-pass baseline
- The final suite metadata is now sharded by bucket under `tests/final/meta/*.json`
- Current preprocessor manifest:
  `tests/final/meta/03-preprocessor.json`
- Current active preprocessor bucket result:
  20 / 20 passing
- Added a dedicated preprocessor prep report:
  `docs/compiler_preprocessor_bucket_report.md`
- Updated the preprocessor checklist rows to reflect current anchor coverage
  versus still-missing negative and edge cases
- Next step:
  expand the preprocessor bucket, collect the full failure set, then fix by
  behavior cluster

### 2026-03-03 — Bucket: preprocessor (first expansion wave)

- Added active suite coverage:
  `03__macro_to_nothing`,
  `03__line_continuation`,
  `03__empty_fn_args`,
  `03__recursive_block`,
  `03__include_missing`,
  `03__partial_token_reject`,
  `03__ifdef_ifndef`,
  `03__pragma_once`,
  `03__skip_missing_include`,
  `03__nested_conditionals`,
  `03__else_without_if`,
  `03__endif_without_if`,
  `03__unterminated_if_reject`,
  `03__elif_after_else`,
  `03__include_cycle`
- The active preprocessor manifest is now:
  35 / 35 passing
- Preprocessor coverage now explicitly includes:
  `#ifdef` / `#ifndef`, `#pragma once`, include-cycle diagnostics, dead-branch
  include skipping, malformed directive-stack diagnostics, and partial-token
  rejection during preprocessing
- Added a fix-first issue repro for macro arity mismatch
  (later fixed and promoted as `03__macro_arg_count_reject`)
- Current open preprocessor gap at this point in the run:
  too-few-argument function-like macro calls still leak to semantic analysis
  instead of failing closed in preprocessing
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-03 — Bucket: preprocessor (arity + include split + `#if` edges)

- Fixed a real preprocessor leak:
  too-few-argument function-like macro calls now fail in preprocessing instead
  of leaking into semantic analysis
- Promoted active suite coverage:
  `03__macro_arg_count_reject`,
  `03__quote_include_prefers_local`,
  `03__angle_include_uses_include_path`,
  `03__pp_if_short_circuit`,
  `03__pp_if_numeric_edges`
- The active preprocessor manifest is now:
  40 / 40 passing
- Include resolution now has an explicit quote-vs-angle split oracle
- `#if` behavior now has dedicated short-circuit and safe numeric-edge anchors
- Added a new fix-first issue repro:
  `03__issue_pp_if_large_values`
- Current open preprocessor gap:
  a larger unsigned `#if` expression still selects the wrong branch, pointing to
  a likely `pp_expr` evaluation bug
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-03 — Bucket: preprocessor (`pp_expr` large-value fix)

- Fixed a real `pp_expr` correctness bug:
  large unsigned preprocessor expressions were being forced through signed
  32-bit behavior and could select the wrong `#if` branch
- Updated `pp_expr` to use an internal intmax/uintmax-style value model for
  arithmetic, shifts, comparisons, and logical evaluation
- Promoted active suite coverage:
  `03__pp_if_large_values`
- The active preprocessor manifest is now:
  41 / 41 passing
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-03 — Bucket: preprocessor (`#if` semantics expansion)

- Promoted active suite coverage:
  `03__pp_if_mixed_signedness`,
  `03__pp_if_ternary_short_circuit`,
  `03__pp_if_unsigned_shift_boundary`
- `pp_expr` coverage now explicitly includes:
  mixed signed/unsigned comparisons, ternary selected-branch evaluation, and
  unsigned high-bit shift boundary behavior
- The active preprocessor manifest is now:
  44 / 44 passing
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-03 — Bucket: preprocessor (malformed `#if` fail-closed)

- Fixed a real preprocessor safety gap:
  malformed `#if` expressions were silently acting like false and skipping code
- `#if` / `#elif` expression evaluation now fails closed with explicit
  preprocessing diagnostics when the expression is invalid
- Promoted active suite coverage:
  `03__pp_if_missing_rhs_reject`,
  `03__pp_if_missing_colon_reject`,
  `03__pp_if_unmatched_paren_reject`,
  `03__pp_if_div_zero_reject`
- Restored `defined`-via-macro behavior while keeping the stricter fail-closed
  rule by preserving `defined` operands during `pp_prepare_expr_tokens`
- The active preprocessor manifest is now:
  48 / 48 passing
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-05 — Bucket: preprocessor (directive strictness hardening)

- Fixed fail-open behavior for unknown active preprocessing directives:
  they now fail closed in preprocessing with explicit diagnostics.
- Fixed fail-open behavior for trailing tokens after conditional directives:
  `#else`, `#endif`, `#ifdef`, and `#ifndef` now fail closed.
- Added explicit unsupported-policy behavior for GNU `, ##__VA_ARGS__`:
  macro definition now emits a stable preprocessing diagnostic instead of
  unstable expansion failure.
- Promoted active suite coverage:
  `03__issue_unknown_directive_reject`,
  `03__issue_endif_extra_tokens_reject`,
  `03__issue_ifdef_extra_tokens_reject`,
  `03__issue_ifndef_extra_tokens_reject`,
  `03__issue_else_extra_tokens_reject`,
  `03__gnu_comma_vaargs`,
  `03__unknown_directive_inactive_branch`
- The active preprocessor manifest is now:
  87 / 87 passing
- Full `tests/final` suite result:
  263 / 263 passing

### 2026-03-05 — Bucket: preprocessor (`#line` + nested strictness + include order)

- Hardened `#line` parsing to fail closed on trailing tokens after the optional
  filename operand.
- Promoted `#line` negative coverage:
  `03__line_directive_missing_number_reject`,
  `03__line_directive_non_number_reject`,
  `03__line_directive_extra_tokens_reject`
- Added deeper nested strictness anchors:
  `03__nested_inner_ifdef_extra_tokens_reject`,
  `03__nested_inner_else_extra_tokens_reject`,
  `03__nested_inactive_skips_unknown_directive`
- Added explicit four-level include-path search-order anchor:
  `03__include_next_order_chain`
- The active preprocessor manifest is now:
  94 / 94 passing
- Full `tests/final` suite result:
  270 / 270 passing

### 2026-03-05 — Bucket: preprocessor (deeper stress follow-up)

- Added `#line` macro-edge fail-closed anchor:
  `03__line_directive_macro_trailing_reject`
- Added deeper nested strictness anchors:
  `03__nested_deep_inner_else_extra_tokens_reject`,
  `03__nested_deep_active_bad_elif_reject`,
  `03__nested_deep_inactive_skips_unknown_directive`
- Added mixed-delimiter include-order anchor:
  `03__include_next_mixed_delims_order`
- The active preprocessor manifest is now:
  99 / 99 passing
- Full `tests/final` suite result:
  275 / 275 passing

### 2026-03-05 — Bucket: preprocessor (strict `#line` + collision matrix follow-up)

- Tightened `#line` number parsing policy:
  line numbers must be positive decimal digit sequences.
- Promoted strict `#line` numeric negatives:
  `03__line_directive_zero_reject`,
  `03__line_directive_hex_number_reject`,
  `03__line_directive_suffix_number_reject`
- Added deeper depth-5 conditional anchors:
  `03__nested_depth5_duplicate_else_reject`,
  `03__nested_depth5_inactive_skips_bad_if_expr`
- Added include-collision quote/angle entry anchors:
  `03__include_collision_quote_chain`,
  `03__include_collision_angle_chain`
- The active preprocessor manifest is now:
  106 / 106 passing
- Full `tests/final` suite result:
  282 / 282 passing

### 2026-03-05 — Bucket: preprocessor (range/depth/collision-long follow-up)

- Promoted strict `#line` range/shape negatives:
  `03__line_directive_out_of_range_reject`,
  `03__line_directive_macro_out_of_range_reject`,
  `03__line_directive_charconst_reject`
- Added deeper depth-6 conditional anchors:
  `03__nested_depth6_duplicate_else_reject`,
  `03__nested_depth6_inactive_skips_bad_elif_expr`
- Added longer include-collision chain anchors:
  `03__include_collision_quote_chain_long`,
  `03__include_collision_angle_chain_long`
- The active preprocessor manifest is now:
  113 / 113 passing
- Full `tests/final` suite result:
  289 / 289 passing

### 2026-03-06 — Bucket: preprocessor (directive operand-shape hardening)

- Fixed fail-open directive operand-shape gaps:
  `#define`, `#undef`, `#ifdef`, and `#ifndef` now require same-line macro
  identifiers and fail closed otherwise.
- Fixed fail-open trailing-token behavior for `#undef`:
  trailing tokens now emit stable preprocessing diagnostics.
- Promoted active suite coverage:
  `03__issue_ifdef_missing_identifier_reject`,
  `03__issue_ifndef_missing_identifier_reject`,
  `03__issue_define_missing_identifier_reject`,
  `03__issue_define_non_identifier_reject`,
  `03__issue_undef_extra_tokens_reject`,
  `03__issue_undef_missing_identifier_reject`,
  `03__macro_expansion_limit_chain_ok`
- The active preprocessor manifest is now:
  120 / 120 passing
- Full `tests/final` suite result:
  296 / 296 passing

### 2026-03-06 — Bucket: preprocessor (recursion/define-shape/include-mix stress)

- Added deeper expansion-limit boundary anchors:
  `03__macro_expansion_limit_chain6_ok`,
  `03__macro_expansion_limit_chain6_reject`
- Added malformed function-like `#define` parameter-list fail-closed anchors:
  `03__issue_define_param_missing_comma_reject`,
  `03__issue_define_param_trailing_comma_reject`,
  `03__issue_define_param_unclosed_reject`
- Added local-entry mixed quote/angle include-collision chain anchor:
  `03__include_collision_mixed_local_angle_chain`
- The active preprocessor manifest is now:
  126 / 126 passing
- Full `tests/final` suite result:
  302 / 302 passing

### 2026-03-06 — Bucket: preprocessor (redef policy + depth7 + include-gap)

- Added macro redefinition policy anchors for cross-kind and conflicting
  function-like redefinitions:
  `03__macro_redef_obj_to_function_tokens`,
  `03__macro_redef_function_to_object_tokens`,
  `03__macro_redef_function_conflict_tokens`
- Added deeper nested malformed-conditional anchors:
  `03__nested_depth7_active_bad_elif_reject`,
  `03__nested_depth7_inactive_skips_bad_if_expr`
- Added missing-mid-chain include resolver anchor:
  `03__include_next_missing_mid_chain`
- The active preprocessor manifest is now:
  132 / 132 passing
- Full `tests/final` suite result:
  308 / 308 passing

### 2026-03-06 — Bucket: preprocessor (defined/operator-shape + depth8 follow-up)

- Fixed `defined` operand consistency for preprocessor identifiers:
  keyword-token macro names are now accepted by `defined` in both paren and
  no-paren forms.
- Promoted `defined` operand-shape anchors:
  `03__defined_keyword_name`,
  `03__defined_keyword_name_no_paren`,
  `03__defined_non_identifier_reject`
- Promoted non-identifier directive-operand fail-closed anchors:
  `03__issue_ifdef_non_identifier_reject`,
  `03__issue_ifndef_non_identifier_reject`,
  `03__issue_undef_non_identifier_reject`
- Added depth-8 malformed conditional anchors:
  `03__nested_depth8_active_bad_elif_reject`,
  `03__nested_depth8_skips_bad_elif_taken_branch`
- The active preprocessor manifest is now:
  140 / 140 passing
- Full `tests/final` suite result:
  316 / 316 passing

### 2026-03-06 — Bucket: preprocessor (include operand strictness follow-up)

- Tightened include-operand parsing to fail closed on trailing tokens after
  parsed header operands for both `#include` and `#include_next`.
- Promoted include strictness anchors:
  `03__include_extra_tokens_reject`,
  `03__include_next_extra_tokens_reject`,
  `03__include_macro_non_header_operand_reject`,
  `03__include_macro_trailing_tokens_reject`,
  `03__include_next_macro_trailing_tokens_reject`
- The active preprocessor manifest is now:
  145 / 145 passing
- Full `tests/final` suite result:
  321 / 321 passing

### 2026-03-08 — Bucket: declarations (Wave 1 storage/qualifier baseline)

- Added storage class baseline anchors:
  `04__storage__static`,
  `04__storage__extern`,
  `04__storage__auto`,
  `04__storage__register`
- Added qualifier baseline anchors:
  `04__qualifiers__const`,
  `04__qualifiers__volatile`,
  `04__qualifiers__restrict`
- `const` now has explicit negative coverage in baseline via assignment to
  const-qualified object.
- Storage and qualifier rows in Phase 3 are now marked `in_progress` with
  baseline valid coverage checked.
- Full `tests/final` suite result:
  328 / 328 passing

### 2026-03-08 — Bucket: declarations (Wave 1 negative expansion)

- Added storage parse-negative anchor:
  `04__storage__missing_declarator`
- Added qualifier negatives:
  `04__qualifiers__const_pointer_assign`,
  `04__qualifiers__missing_type`
- Updated Phase 3 matrix rows with newly covered negatives and explicit
  spec-gap notes for storage-class legality and non-pointer `restrict`.
- Full `tests/final` suite result:
  331 / 331 passing

### 2026-03-08 — Bucket: declarations (storage/qualifier fix pass + strict negatives)

- Enforced storage-class legality in semantic declaration analysis:
  file-scope `auto/register` now reject, conflicting storage-class specifiers
  now reject, and `typedef` mixed with other storage classes now rejects.
- Enforced `restrict` legality: non-pointer top-level `restrict` declarations
  now reject.
- Added strict negative anchors:
  `04__storage__file_scope_auto_reject`,
  `04__storage__file_scope_register_reject`,
  `04__storage__conflict_static_auto_reject`,
  `04__storage__conflict_extern_register_reject`,
  `04__storage__typedef_extern_reject`,
  `04__qualifiers__restrict_non_pointer_reject`
- Full `tests/final` suite result:
  337 / 337 passing

### 2026-03-08 — Bucket: declarations (primitive integer baseline start)

- Added primitive integer baseline anchor:
  `04__primitive__integers`
- Phase 3 primitive integer row now has valid coverage marked `in_progress`.
- Full `tests/final` suite result:
  338 / 338 passing

### 2026-03-08 — Bucket: declarations (primitive floating/bool baseline start)

- Added primitive baseline anchors:
  `04__primitive__floating`,
  `04__primitive__bool`
- Phase 3 floating and `_Bool` rows now have valid coverage marked
  `in_progress`.
- Full `tests/final` suite result:
  340 / 340 passing

### 2026-03-08 — Bucket: declarations (primitive specifier legality fix + negatives)

- Enforced primitive specifier legality in semantic declaration analysis:
  signed/unsigned conflict rejects, short/long conflict rejects, and invalid
  integer modifiers on `float`, `_Bool`, and `void` now reject.
- Added strict primitive-negative anchors:
  `04__primitive__signed_unsigned_conflict_reject`,
  `04__primitive__short_long_conflict_reject`,
  `04__primitive__unsigned_float_reject`,
  `04__primitive__signed_bool_reject`,
  `04__primitive__unsigned_void_reject`
- Phase 3 primitive rows now include negative coverage for integer/floating/
  `_Bool` combinations.
- Full `tests/final` suite result:
  345 / 345 passing

### 2026-03-08 — Bucket: declarations (char + enum follow-up)

- Added char baseline and negative anchors:
  `04__primitive__char_signedness`,
  `04__primitive__long_char_reject`
- Added enum duplicate-negative anchor:
  `04__enum__duplicate`
- Updated Phase 3 rows:
  char row now `in_progress` with valid+negative coverage; duplicate enum row
  now has negative coverage.
- Confirmed remaining gap:
  duplicate field names in `struct`/`union` are still accepted without
  diagnostics and remain tracked as unstarted.
- Full `tests/final` suite result:
  348 / 348 passing

### 2026-03-10 — Bucket: torture-differential (phase-10 baseline)

- Added runtime+differential torture anchors:
  `15__torture__long_expr`,
  `15__torture__deep_nesting`,
  `15__torture__many_decls`,
  `15__torture__large_struct`
- Added compile-surface pathological declarator anchor:
  `15__torture__pathological_decl`
- Added malformed-input robustness anchor:
  `15__torture__malformed_no_crash`
- Current known gap tracked:
  function-pointer runtime path for this declarator family still crashes in
  probe form, so pathological declarator is currently compile-surface only.

### 2026-03-10 — Bucket: diagnostics-recovery (targeted gap expansion)

- Added active diagnostics test:
  `12__diag_type_mismatch`
- Probe failures logged (not yet promoted as passing anchors):
  `12__diag_incompatible_ptr` candidate (`int* = double*`) currently emits no
  diagnostic.
  `12__diag_illegal_cast` candidate (`(int)(struct S){1}`) currently emits no
  diagnostic and lowers to a bitcast in IR.
- Possible causes:
  assignment compatibility checks are not enforcing pointer constraint
  diagnostics; cast semantic validation is not rejecting non-scalar source
  casts to scalar destinations.

### 2026-03-10 — Bucket: torture-differential (stress expansion wave 2)

- Added runtime stress anchor:
  `15__torture__many_globals`
- Added malformed robustness anchor:
  `15__torture__malformed_unclosed_comment_no_crash`
- Logged blocked probe:
  `15__torture__deep_switch` currently mismatches clang runtime output
  (`fisics=15`, `clang=60`) and is not promoted as a passing anchor.
- Result:
  new active stress anchors pass and differential-check cleanly where
  applicable;
  malformed unclosed-comment path fails closed with deterministic lexer
  diagnostics and no crash.

### 2026-03-10 — Bucket: torture-differential (stress expansion wave 3)

- Added runtime stress anchors:
  `15__torture__many_params`,
  `15__torture__large_array_stress`
- Added malformed robustness anchor:
  `15__torture__malformed_unbalanced_block_no_crash`
- Logged blocked probe:
  `15__torture__path_decl_nested` currently segfaults at runtime (`exit 139`)
  in function-pointer call path; not promoted as passing anchor.

### 2026-03-10 — Bucket: runtime-surface (harness + control-flow expansion)

- Added runtime harness-behavior anchors:
  `14__runtime_args_env`,
  `14__runtime_stdin`
- Added runtime control-flow anchor:
  `14__runtime_switch_simple`
- Logged blocked control-flow probe:
  looped-switch variant (`15__torture__deep_switch`) still mismatches clang
  output and remains outside active passing anchors.

### 2026-03-10 — Bucket: runtime-surface (float edge expansion)

- Added active floating-point infinity anchor:
  `14__runtime_float_infinity`
- Logged blocked floating-point probe:
  `14__float__nan` currently mismatches clang on NaN self-inequality.

### 2026-03-10 — Bucket: runtime-surface (calls/memory stress expansion)

- Added active runtime+differential anchors:
  `14__runtime_variadic_calls`,
  `14__runtime_deep_recursion`,
  `14__runtime_struct_layout_alignment`
- Phase-8 rows promoted to `in_progress` for variadic calls, deep recursion,
  struct layout correctness, and alignment correctness.

### 2026-03-10 — Probe Fixtures (pre-fix lock-in set)

- Added non-manifest probe fixtures under:
  `tests/final/probes/runtime/` and `tests/final/probes/diagnostics/`
- Added probe runner:
  `python3 tests/final/probes/run_probes.py`
- Current snapshot (`2026-03-10`):
  blocked=7, resolved=0, skipped=0
- Blocked runtime probes:
  `14__probe_unsigned_wrap`,
  `14__probe_float_nan`,
  `15__probe_switch_loop_lite`,
  `15__probe_switch_loop_mod5`,
  `15__probe_path_decl_nested_runtime`
- Blocked diagnostics probes:
  `12__probe_incompatible_ptr_assign`,
  `12__probe_illegal_struct_to_int_cast`

### 2026-03-10 — Probe Fixtures (declarator + shift-width expansion)

- Added runtime boundary probe:
  `04__probe_deep_declarator_call_only`
- Added deep declarator compile-time hang probe:
  `04__probe_deep_declarator_codegen_hang`
- Added diagnostics probe:
  `12__probe_invalid_shift_width`
- Updated probe runner with deterministic compile/runtime timeouts to avoid
  hanging the full probe sweep.
- Current snapshot (`2026-03-10`):
  blocked=3, resolved=7, skipped=0
- Blocked probes:
  `04__probe_deep_declarator_call_only` (compiler crash `-11`),
  `04__probe_deep_declarator_codegen_hang` (compile timeout),
  `12__probe_invalid_shift_width` (missing diagnostic).

### 2026-03-11 — Bucket: expressions (wave 3 surface expansion)

- Added expression-surface anchors:
  `05__unary__bitwise_not`,
  `05__unary__sizeof_ambiguity`,
  `05__binary__arithmetic`,
  `05__binary__bitwise`,
  `05__binary__logical`,
  `05__binary__relational`,
  `05__binary__equality`,
  `05__binary__invalid_shift_width`,
  `05__ternary__nested`,
  `05__casts__explicit`,
  `05__casts__ambiguity`
- Promoted negative shift-width behavior from probe-only to active suite:
  `05__binary__invalid_shift_width`
- Bucket sweep (`05__*`) currently passes with 39/39 green.
- Added expressions diagnostics probes:
  `05__probe_shift_width_large_reject`,
  `05__probe_bitwise_float_reject`,
  `05__probe_relational_struct_reject`,
  `05__probe_equality_struct_reject`,
  `05__probe_add_void_reject`
- Probe runner snapshot after wave-3 follow-up:
  blocked=0, resolved=50, skipped=0.

### 2026-03-11 — Bucket: expressions (probe expansion follow-up)

- Added runtime probes:
  `05__probe_precedence_runtime`,
  `05__probe_unsigned_arith_conv_runtime`,
  `05__probe_nested_ternary_runtime`,
  `05__probe_nested_ternary_outer_true_runtime`,
  `05__probe_nested_ternary_false_chain_runtime`
- Added diagnostics probes:
  `05__probe_address_of_rvalue_reject`,
  `05__probe_deref_non_pointer_reject`,
  `05__probe_sizeof_incomplete_type_reject`,
  `05__probe_sizeof_function_reject`,
  `05__probe_logical_void_operand_reject`,
  `05__probe_relational_void_reject`,
  `05__probe_ternary_struct_condition_reject`,
  `05__probe_cast_int_to_struct_reject`
- Probe runner snapshot:
  blocked=2, resolved=61, skipped=0.
- Blocked runtime probes:
  `05__probe_nested_ternary_runtime`,
  `05__probe_nested_ternary_false_chain_runtime`
  (both mismatch clang: `fisics` exit `1`, clang exit `0`).

### 2026-03-11 — Bucket: expressions (probe expansion follow-up 2)

- Added runtime probes:
  `05__probe_short_circuit_and_runtime`,
  `05__probe_short_circuit_or_runtime`,
  `05__probe_comma_eval_runtime`
- Added diagnostics probes:
  `05__probe_alignof_void_reject`,
  `05__probe_alignof_incomplete_reject`
- Probe runner snapshot:
  blocked=3, resolved=65, skipped=0.
- New blocked diagnostics probe:
  `05__probe_alignof_void_reject`
  (`_Alignof(void)` accepted without diagnostic).

### 2026-03-11 — Bucket: expressions (final probe sweep before fixes)

- Added runtime probes:
  `05__probe_vla_sizeof_side_effect_runtime`,
  `05__probe_ternary_side_effect_runtime`,
  `05__probe_nested_ternary_deep_false_chain_runtime`,
  `05__probe_compound_literal_array_runtime`
- Added diagnostics probes:
  `05__probe_unary_bitnot_float_reject`,
  `05__probe_unary_plus_struct_reject`,
  `05__probe_unary_minus_pointer_reject`,
  `05__probe_sizeof_void_reject`,
  `05__probe_alignof_expr_reject`
- Probe runner snapshot:
  blocked=7, resolved=70, skipped=0.
- New blocked runtime probes:
  `05__probe_vla_sizeof_side_effect_runtime`,
  `05__probe_nested_ternary_deep_false_chain_runtime`
  (both mismatch clang: `fisics` exit `1`, clang exit `0`).
- New blocked diagnostics probes:
  `05__probe_sizeof_void_reject`,
  `05__probe_alignof_expr_reject`
  (invalid operand forms accepted without diagnostic).

### 2026-03-11 — Post-fix probe baseline refresh

- Current probe runner snapshot:
  `blocked=0, resolved=77, skipped=0`
- Expressions (`05`) was promoted to active stable baseline with wave 4:
  runtime differential and diagnostics probe coverage moved into active suite.
- Previously stale open-probe notes were cleared in:
  `07-types-conversions.md`,
  `08-initializers-layout.md`,
  `09-statements-controlflow.md`,
  `10-scopes-linkage.md`,
  `12-diagnostics-recovery.md`,
  `15-torture-differential.md`.
- Coverage matrix rows updated to reflect resolved probe state for:
  expressions invalid `sizeof/_Alignof` diagnostics,
  nested ternary runtime,
  scopes file-scope extern-array consistency,
  runtime unsigned-wrap / NaN probes,
  switch-loop differential probes,
  and diagnostics incompatible-pointer / illegal-cast probes.

### 2026-03-11 — Bucket: statements-controlflow (wave 7 + wave 8)

- Promoted resolved diagnostics probes into active suite (wave 7):
  `09__diag__goto_into_vla_scope_reject`,
  `09__diag__switch_float_condition_reject`,
  `09__diag__switch_pointer_condition_reject`,
  `09__diag__switch_string_condition_reject`,
  `09__diag__switch_double_condition_reject`,
  `09__diag__if_void_condition_reject`,
  `09__diag__if_struct_condition_reject`,
  `09__diag__while_void_condition_reject`,
  `09__diag__do_struct_condition_reject`,
  `09__diag__for_void_condition_reject`,
  `09__diag__for_struct_condition_reject`.
- Added runtime+differential control-flow anchors (wave 8):
  `09__runtime__switch_fallthrough_order`,
  `09__runtime__nested_loop_switch_control`,
  `09__runtime__while_for_side_effects`,
  `09__runtime__goto_forward_backward`,
  `09__runtime__switch_default_selection`.
- Active `09__*` bucket sweep is green with these promotions.

### 2026-03-11 — Bucket: statements-controlflow (do-while runtime fix + wave 9)

- Fixed `do-while` runtime executable codegen crash by correcting `AST_WHILE_LOOP`
  lowering for `isDoWhile=true` in `codegenWhileLoop`.
- Added runtime+differential active anchor:
  `09__runtime__do_while_side_effects` (`09-statements-controlflow-wave9.json`).
- Added/kept dedicated runtime probe fixture:
  `09__probe_do_while_runtime_codegen_crash`.
- Probe runner snapshot now:
  `blocked=0, resolved=78, skipped=0`.
- Active `09__*` bucket remains green after wave 9 promotion.

### 2026-03-11 — Bucket: diagnostics-recovery (wave A + wave B)

- Wave A promotions completed into active suite:
  `12__diag_incompatible_ptr`,
  `12__diag_illegal_cast`.
- Wave B recovery expansion added:
  `12__recovery__if_missing_rparen_then_followup_stmt`,
  `12__recovery__while_missing_lparen_then_followup_stmt`,
  `12__recovery__for_header_missing_semicolon_then_followup_decl`,
  `12__recovery__do_while_missing_semicolon_then_followup_stmt`,
  `12__recovery__switch_case_missing_colon_then_followup_case`,
  `12__recovery__switch_bad_case_expr_then_default`,
  `12__recovery__goto_missing_label_token_then_followup_label`,
  `12__recovery__case_outside_switch_then_next_stmt_ok`.
- Active `12__*` bucket sweep is green after wave 2 additions.
- Next diagnostics-recovery target is Wave C:
  diagnostic line-anchor stability, primary-vs-follow-on ordering, and cascade
  suppression bounds.

### 2026-03-11 — Bucket: diagnostics-recovery (wave C)

- Added wave-C active diagnostics anchors:
  `12__diag_line_anchor__undeclared_calls`,
  `12__diag_order__nonvoid_return_then_unreachable`,
  `12__diag_cascade_bound__single_unreachable_region`,
  `12__diag_line_anchor__ptr_then_cast`,
  `12__diag_line_anchor__macro_two_callsites`.
- Added manifest:
  `tests/final/meta/12-diagnostics-recovery-wave3.json`.
- Active `12__*` bucket sweep is green after wave 3 additions.
- Next diagnostics-recovery target was parser diagnostic export path.

### 2026-03-11 — Bucket: diagnostics-recovery (wave 4 parser export)

- Added parser-diagnostic export assertions in active suite:
  `12__parserdiag__missing_semicolon`,
  `12__parserdiag__for_missing_paren`,
  `12__parserdiag__bad_paren_expr`,
  `12__parserdiag__stray_else_recovery`.
- Added manifest:
  `tests/final/meta/12-diagnostics-recovery-wave4-parserdiag.json`.
- Extended final harness with parser-diagnostic expectation support (`.pdiag`),
  sourced from `--emit-diags-json` and normalized to stable parser diagnostic tuples.
- Active `12__*` bucket sweep is green after wave 4 additions.

### 2026-03-11 — Buckets: parser-diag expansion wave set (04/05/09/11/12)

- Added parser-diagnostic manifests:
  `04-declarations-wave6-parserdiag.json`,
  `05-expressions-wave5-parserdiag.json`,
  `09-statements-controlflow-wave10-parserdiag.json`,
  `11-functions-calls-wave2-parserdiag.json`,
  `12-diagnostics-recovery-wave5-parserdiag.json`.
- Added parser-focused malformed fixtures:
  `05__parserdiag__*` and `11__parserdiag__*` new case files.
- Promoted parser-diag coverage on existing negative fixtures for declarations,
  statements/control-flow, and diagnostics-recovery recovery paths.
- Fixed diagnostics export for zero-diagnostic translation units so
  `--emit-diags-json` now emits a valid empty payload instead of failing.
- Targeted new-wave IDs all pass, and full active sweep across
  `04__*`, `05__*`, `09__*`, `11__*`, and `12__*` remains green.

### 2026-03-11 — Buckets: parser-diag expansion wave set B (04/05/09/11/12)

- Added parser expansion note:
  `tests/final/parser_wave_b_expansion_plan.md`.
- Added parser-diagnostic manifests:
  `04-declarations-wave7-parserdiag.json`,
  `05-expressions-wave6-parserdiag.json`,
  `09-statements-controlflow-wave11-parserdiag.json`,
  `11-functions-calls-wave3-parserdiag.json`,
  `12-diagnostics-recovery-wave6-parserdiag.json`.
- Added malformed parser fixtures for:
  deeper control-flow recovery shapes (`09`),
  declaration/declarator grammar failures (`04`),
  postfix/ternary malformed forms (`05`),
  prototype/parameter-list malformed forms (`11`),
  and mixed parser+semantic ordering files (`12`).
- New wave IDs are all green and the full active sweep across
  `04__*`, `05__*`, `09__*`, `11__*`, and `12__*` remains green.

### 2026-03-11 — Bucket: statements-controlflow (09 parserdiag lane + probe pass)

- Added 09 parser statement-shape probes:
  `09__probe_if_missing_then_stmt_reject`,
  `09__probe_else_missing_stmt_reject`,
  `09__probe_switch_missing_rparen_reject`,
  `09__probe_for_missing_first_semicolon_reject`,
  `09__probe_goto_undefined_label_nested_reject`.
- Added probe-runner subset filter:
  `PROBE_FILTER=...` in `tests/final/probes/run_probes.py`.
- 09-only probe sweep (`PROBE_FILTER=09__probe_`) result:
  `blocked=0, resolved=17, skipped=0`.
- Promoted resolved parser-diag cases into new standardized lane manifest:
  `tests/final/meta/09-statements-controlflow-parserdiag.json`
  with active tests:
  `09__parserdiag__if_missing_then_stmt_reject`,
  `09__parserdiag__else_missing_stmt_reject`,
  `09__parserdiag__switch_missing_rparen_reject`,
  `09__parserdiag__for_missing_first_semicolon_reject`.

### 2026-03-11 — Bucket: statements-controlflow (09 nested-label promotion)

- Promoted resolved probe `09__probe_goto_undefined_label_nested_reject`
  into active suite as:
  `09__goto_undefined_label_nested_reject`
  under `tests/final/meta/09-statements-controlflow-wave5.json`.
- Targeted test pass confirms promotion is green.

### 2026-03-11 — Bucket: declarations (04 parserdiag lane start)

- Added 04 parser-shape declaration probes:
  `04__probe_struct_member_missing_type_reject`,
  `04__probe_bitfield_missing_colon_reject`,
  `04__probe_enum_missing_rbrace_reject`,
  `04__probe_typedef_missing_identifier_reject`,
  `04__probe_declarator_unbalanced_parens_reject`.
- 04-only probe sweep (`PROBE_FILTER=04__probe_`) result:
  `blocked=0, resolved=16, skipped=0`.
- Promoted resolved parser-diag cases into standardized lane manifest:
  `tests/final/meta/04-declarations-parserdiag.json`
  with active tests:
  `04__parserdiag__struct_member_missing_type_reject`,
  `04__parserdiag__bitfield_missing_colon_reject`,
  `04__parserdiag__enum_missing_rbrace_reject`,
  `04__parserdiag__typedef_missing_identifier_reject`,
  `04__parserdiag__declarator_unbalanced_parens_reject`.
- 04 active bucket sweep (`04__*`) is green after promotion.

### 2026-03-11 — Bucket: declarations (04 union-overlap + complex legality follow-up)

- Added new 04 probes:
  - runtime: `04__probe_union_overlap_runtime`
  - diagnostics: `04__probe_complex_int_reject`,
    `04__probe_complex_unsigned_reject`,
    `04__probe_complex_missing_base_reject`
- Probe result for this batch:
  `blocked=1, resolved=3, skipped=0`
  - blocked: `04__probe_complex_int_reject`
- Promoted resolved cases into new standardized lane manifest:
  `tests/final/meta/04-declarations-runtime-semantic.json`
  with active tests:
  `04__union__overlap`,
  `04__primitive__complex_unsigned_reject`,
  `04__primitive__complex_missing_base_reject`.
- Full `04__*` active sweep remains green after promotion.

### 2026-03-11 — Bucket: declarations (04 complex-int fix + promotion)

- Fixed declaration-specifier validation to reject non-floating complex bases:
  `_Complex int` now fails closed with a semantic diagnostic.
- Re-ran targeted complex probes:
  `PROBE_FILTER=04__probe_complex_` now reports
  `blocked=0, resolved=3, skipped=0`.
- Promoted the previously blocked probe into active suite:
  `04__primitive__complex_int_reject`
  (added to `tests/final/meta/04-declarations-runtime-semantic.json`).
- Full `04__*` active sweep remains green after this fix.
