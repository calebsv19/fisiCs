# Compiler Behavior Coverage Checklist

This document is the working execution checklist for the compiler test upgrade.

Use it as the per-bucket validation tracker while migrating the suite toward
phase-complete C99 to C17 behavior coverage. The workflow is:

1. Pick one bucket.
2. Add or run all tests for that bucket.
3. Record which cases pass, fail, or are unsupported.
4. Collect all failures for that bucket before fixing behavior.
5. Fix the bucket as a group.
6. Re-run the bucket and close it only when the full section is stable.

This is the operational companion to:

- `docs/compiler_test_system_rearchitecture_context.md`
- `docs/compiler_test_coverage_blueprint.md`

## How To Use This Checklist

Every checklist item below is a feature family, not a single test.

Each feature family must eventually have:

- At least one valid test
- At least one invalid or negative test where applicable
- At least one edge or stress variant

As tests are implemented, track them in this format:

| Field | Meaning |
| --- | --- |
| Feature | Language or compiler behavior being validated |
| Bucket | Test harness bucket (`lexer`, `preprocessor`, `runtime`, etc.) |
| Valid | Positive test coverage exists and passes |
| Negative | Invalid-input or rejection coverage exists and passes |
| Edge | Boundary or stress variant exists and passes |
| Status | `unstarted`, `in_progress`, `blocked`, `passing`, `unsupported` |
| Oracle | `tokens`, `diag`, `ast`, `ir`, `runtime`, `differential` |
| Planned Tests | Intended test ids or filenames |
| Failures Seen | Current observed compiler gaps or regressions |
| Notes | Extra constraints, UB tags, ABI sensitivity, or follow-up |

Recommended pass discipline per bucket:

- Do not mark a bucket complete because a few representative tests pass.
- Run the whole bucket and log all failures first.
- Fix behavior only after the failure set is visible.
- Re-run the full bucket after fixes, not just the changed case.

## Tracking Conventions

- Mark unsupported features explicitly. Do not leave them ambiguous.
- Tag UB and implementation-defined tests so they can be excluded from strict
  differential comparison.
- Diagnostics should be judged by category and location, not full text.
- Runtime behavior is preferred over textual IR matching when both are viable.
- Until concrete test files exist, use planned ids in the form
  `tests/final/cases/<bucket>__<feature>__<variant>.c`.

## Phase 1 — Lexical Analysis (Tokenization)

Primary bucket:

- `lexer`

Primary oracles:

- `tokens`
- `diag`

### 1.1 Identifiers

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Basic ASCII identifiers | `lexer` | [x] | [ ] | [ ] | `in_progress` | `tokens` | `02__identifier_matrix` | | Negative/error boundary still missing |
| Leading underscore | `lexer` | [x] | [ ] | [ ] | `in_progress` | `tokens` | `02__identifier_matrix` | | |
| Multiple underscores | `lexer` | [x] | [ ] | [ ] | `in_progress` | `tokens` | `02__identifier_matrix` | | |
| Extremely long identifiers | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__long_identifier` | | Still missing a negative boundary/error case |
| Keywords vs identifiers | `lexer` | [x] | [x] | [ ] | `in_progress` | `tokens`, `diag` | `02__c11_keyword_tokens`, `02__keyword_identifier_reject` | Full identifier-boundary matrix still missing | |
| Universal character names | `lexer` | [ ] | [x] | [ ] | `unsupported` | `diag` | `02__ucn_identifier_unsupported` | Explicit unsupported diagnostic now emitted during lexing and compilation stops before parse/codegen | Full decoding support still absent |
| Identifier shadowing via macro expansion | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens`, `diag` | `02__macro_identifier_boundary` | Macro token-paste identifier formation now has an active boundary anchor; broader shadowing cases still missing | Cross-phase with preprocessor |

### 1.2 Keywords (C99 + C11/C17)

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| All standard keywords recognized | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__c11_keyword_tokens`, `02__keyword_matrix`, `02__macro_keyword_boundary` | Coverage now spans control-flow, storage, selected C11 spellings, and macro-pasted keyword formation, but still not the full standard matrix | Split by standard level if needed |
| Misuse as identifiers rejected | `lexer` | [x] | [x] | [ ] | `in_progress` | `diag` | `02__keyword_identifier_reject` | Wider reserved-word misuse matrix still missing | |

### 1.3 Integer Literals

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Decimal, octal, hexadecimal | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__int_literals`, `02__literal_semantics` | Core base-form coverage is active; broader matrix splits are still missing | |
| `0`-prefixed octal edge cases | `lexer` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `02__int_literals`, `02__octal_invalid_digit_reject` | | |
| Hex uppercase and lowercase | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__int_literals`, `02__radix_case_matrix` | Uppercase `0X` forms now have an explicit anchor; broader split matrix is still missing | |
| Suffix combinations | `lexer` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `02__int_literals`, `02__int_suffix_order`, `02__int_suffix_case_matrix`, `02__int_invalid_suffix_reject` | | `U`, `L`, `LL`, `UL`, `ULL`; broader invalid matrix still missing |
| Overflow detection | `lexer` | [ ] | [x] | [x] | `in_progress` | `diag` | `02__int_overflow_diag` | | Diagnostic vs accepted wrap must be explicit |
| Large boundary constants | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__int_literals__boundaries` | | `INT_MAX`, `UINT_MAX` equivalents |
| Invalid suffix combinations | `lexer` | [ ] | [x] | [x] | `in_progress` | `diag` | `02__int_invalid_suffix_reject` | | |

### 1.4 Floating Literals

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Decimal floats | `lexer` | [x] | [ ] | [ ] | `in_progress` | `tokens` | `02__float_literals`, `02__float_edge_forms` | | |
| Scientific notation | `lexer` | [x] | [ ] | [ ] | `in_progress` | `tokens` | `02__float_exponent_forms`, `02__float_edge_forms` | | |
| Hex floats | `lexer` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `02__float_literals`, `02__float_edge_forms`, `02__float_hex_no_exponent`, `02__hex_float_missing_mantissa_reject` | Non-standard hex-float forms without `p` now reject explicitly; missing-mantissa forms now reject too | |
| Suffixes `f/F/l/L` | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__float_suffix_matrix` | | |
| Boundary forms (`.5`, `5.`, `5e+3`) | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__float_edge_forms` | | |
| Invalid exponent forms | `lexer` | [ ] | [x] | [x] | `in_progress` | `diag` | `02__malformed_float_reject`, `02__binary_float_forms_reject`, `02__hex_bad_exponent_tail_reject` | Current active rejection covers missing exponent digits after `e`/`p`, malformed hex-exponent tails, and binary-prefixed float-like spellings; broader malformed-float matrix is still missing | |
| Precision boundaries | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__float_literals__precision_boundary` | | |

### 1.5 Character Constants

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Standard char constants | `lexer` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `02__char_literals`, `02__empty_char_reject`, `02__unterminated_char_reject` | | |
| Escape sequences | `lexer` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `02__char_literals`, `02__invalid_escape_char`, `02__escape_question_mark`, `02__char_unicode_escapes`, `02__char_invalid_hex_escape_reject`, `02__char_invalid_u16_escape_reject`, `02__char_invalid_u32_escape_reject` | | Unknown escapes now reject; `\?` is explicitly anchored as valid |
| Octal and hex escapes | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens`, `diag` | `02__char_literals` | | |
| Multi-character constants | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__char_literals__multichar` | | Implementation-defined value |
| Wide char (`L'a'`) | `lexer` | [x] | [ ] | [ ] | `in_progress` | `tokens`, `diag` | `02__wide_utf_chars` | | |
| UTF-8/16/32 forms | `lexer` | [x] | [ ] | [ ] | `in_progress` | `tokens`, `diag` | `02__wide_utf_chars`, `02__wide_utf_strings` | UTF char coverage currently covers `u` and `U`; `u8` remains string-only | Mark unsupported if absent |
| Overflow in escape sequences | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `02__char_literals__escape_overflow` | | |

### 1.6 String Literals

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Normal strings | `lexer` | [x] | [ ] | [ ] | `in_progress` | `tokens` | `02__string_concat`, `02__wide_utf_strings` | | |
| Concatenated adjacent strings | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__string_concat` | | |
| Wide and UTF strings | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens`, `diag` | `02__wide_utf_strings` | | |
| Embedded null bytes | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens`, `diag` | `02__string_embedded_null` | | |
| Escape correctness | `lexer` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `02__string_escape_matrix`, `02__unterminated_string_reject`, `02__string_invalid_hex_escape_reject`, `02__string_invalid_u16_escape_reject`, `02__string_invalid_u32_escape_reject`, `02__string_invalid_escape_reject`, `02__escape_question_mark` | | Unknown escapes now reject; `\?` is explicitly anchored as valid |
| Extremely long strings | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__long_string` | | Stress path |

### 1.7 Operators and Punctuators

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| All unary operators | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__operator_boundary_matrix`, `02__operator_misc_matrix` | Current coverage now includes prefix `~`, `!`, `*`, and `&`; full unary surface still missing | |
| All binary operators | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__operator_boundary_matrix`, `02__operator_relational_matrix`, `02__operator_misc_matrix` | Current matrices cover arithmetic, logical, relational, shift, assignment-family, divide, and modulo boundaries; full binary surface still missing | |
| Ternary operator tokenization | `lexer` | [x] | [ ] | [ ] | `in_progress` | `tokens` | `02__operator_boundary_matrix` | | |
| Compound assignments | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__operator_boundary_matrix` | | |
| Pre/post increment | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__operator_boundary_matrix` | | |
| Arrow operator | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__operator_relational_matrix` | | |
| Digraphs | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens`, `diag` | `02__digraph_tokens`, `02__digraph_pp_tokens` | | Mark unsupported if absent |
| Trigraphs | `lexer` | [ ] | [x] | [x] | `unsupported` | `diag` | `02__trigraph_reject` | Explicit unsupported rejection path is active | Mark unsupported if absent |
| Token boundary correctness | `lexer` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `02__operator_boundary_matrix`, `02__operator_edges`, `02__operator_relational_matrix`, `02__punctuator_matrix`, `02__punctuator_oddities`, `02__percent_hashhash_digraph`, `02__invalid_dollar_reject`, `02__invalid_at_reject`, `02__invalid_backtick_reject` | | Includes adjacent punctuators |

### 1.8 Comments

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Single-line comments | `lexer` | [x] | [ ] | [ ] | `in_progress` | `tokens` | `02__comments_and_splice` | | |
| Multi-line comments | `lexer` | [x] | [x] | [ ] | `in_progress` | `tokens`, `diag` | `02__comments_and_splice`, `02__unterminated_comment_reject` | | |
| Nested-like patterns | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens`, `diag` | `02__comment_nested_like` | | |
| Comments inside macros | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens` | `02__comment_macro_context` | | Cross-phase with preprocessor |
| Backslash-newline splicing | `lexer` | [x] | [ ] | [x] | `in_progress` | `tokens`, `diag` | `02__comments_and_splice` | | |

### 1.9 Preprocessing Tokens

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Correct tokenization before macro expansion | `lexer` | [x] | [ ] | [ ] | `in_progress` | `tokens` | `02__digraph_pp_tokens` | Still only lightly covered via `%:` define flow | Distinct from final parser tokens |

## Phase 2 — Preprocessor

Primary bucket:

- `preprocessor`

Primary oracles:

- `tokens`
- `diag`
- `differential` (optional via `clang -E`)

### 2.1 Object-Like Macros

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Basic replacement | `preprocessor` | [x] | [ ] | [ ] | `in_progress` | `tokens` | `03__object_like_macro` | | |
| Recursive blocking behavior | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__recursive_block`, `03__macro_mutual_recursive_block`, `03__macro_expansion_limit_reject`, `03__macro_expansion_limit_chain_ok`, `03__macro_expansion_limit_chain6_ok`, `03__macro_expansion_limit_chain6_reject` | Self-recursive and mutual-recursive blocking anchors are active, expansion-depth-limit failures have diagnostic anchors, and explicit-limit boundary pass/fail behavior is now covered; deeper recursion stress still needs coverage | |
| Redefinition warnings or errors | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__macro_redefinition`, `03__macro_redef_identical_tokens`, `03__macro_redef_conflict_tokens`, `03__macro_redef_obj_to_function_tokens`, `03__macro_redef_function_to_object_tokens`, `03__macro_redef_function_conflict_tokens` | Behavior anchors now include identical/conflicting redefinitions and cross-kind object↔function redefinition expansion policy; dedicated diagnostic matching is still thin | Policy must be explicit |

### 2.2 Function-Like Macros

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Parameter substitution | `preprocessor` | [x] | [ ] | [ ] | `in_progress` | `tokens` | `03__function_like_macro` | | |
| Argument count mismatch | `preprocessor` | [ ] | [x] | [x] | `in_progress` | `diag` | `03__macro_arg_count_reject`, `03__macro_arg_too_many_reject`, `03__macro_arg_missing_comma_reject`, `03__variadic_missing_named_arg_reject` | Too-few, too-many, missing-comma, and variadic-missing-named-arg mismatch shapes now fail closed in preprocessing | |
| Nested macros in arguments | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens` | `03__nested_macro_expansion`, `03__macro_arg_commas` | | |
| Empty argument lists | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens`, `diag` | `03__variadic_empty`, `03__empty_fn_args` | | |
| Variadic macros | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens` | `03__variadic_macro`, `03__variadic_empty` | | |
| `__VA_ARGS__` | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__variadic_macro`, `03__variadic_empty`, `03__gnu_comma_vaargs` | Unsupported GNU `, ##__VA_ARGS__` now emits a stable fail-closed preprocessing diagnostic | |
| Token pasting (`##`) | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__stringify_and_paste`, `03__token_paste_numbers`, `03__macro_paste_expand`, `03__gnu_comma_vaargs` | Includes explicit unsupported-policy diagnostics for GNU comma-paste variadic extension | |
| Stringification (`#`) | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens` | `03__stringify_and_paste`, `03__stringize_spacing` | | |
| Expansion order correctness | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens` | `03__nested_macro_expansion`, `03__defined_macro_expansion`, `03__macro_paste_expand` | | |

### 2.3 Conditional Compilation

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `#if` constant expressions | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens` | `03__pp_if_arith`, `03__if_elif_chain`, `03__pp_elif_skips_dead_expr` | `#elif` branch-selection skipping now has a direct positive anchor | |
| `defined` operator | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__defined_expr`, `03__if_defined_paren`, `03__defined_macro_expansion`, `03__undef_defined`, `03__defined_keyword_name`, `03__defined_keyword_name_no_paren`, `03__defined_non_identifier_reject` | `defined` now accepts keyword-token macro names in both paren/no-paren forms, and non-identifier operands now fail closed | |
| Nested conditionals | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__if_elif_chain`, `03__nested_conditionals`, `03__else_without_if`, `03__endif_without_if`, `03__unterminated_if_reject`, `03__elif_after_else`, `03__pp_elif_missing_rhs_reject`, `03__pp_elif_div_zero_reject`, `03__pp_elif_missing_colon_reject`, `03__pp_elif_unmatched_paren_reject`, `03__inactive_nested_if_skips_bad_expr`, `03__nested_duplicate_else_reject`, `03__nested_elif_after_else_reject`, `03__inactive_outer_skips_malformed_elif`, `03__nested_active_bad_elif_reject`, `03__issue_endif_extra_tokens_reject`, `03__issue_ifdef_extra_tokens_reject`, `03__issue_ifndef_extra_tokens_reject`, `03__issue_else_extra_tokens_reject`, `03__issue_ifdef_missing_identifier_reject`, `03__issue_ifndef_missing_identifier_reject`, `03__nested_inner_ifdef_extra_tokens_reject`, `03__nested_inner_else_extra_tokens_reject`, `03__nested_inactive_skips_unknown_directive`, `03__nested_deep_inner_else_extra_tokens_reject`, `03__nested_deep_active_bad_elif_reject`, `03__nested_deep_inactive_skips_unknown_directive`, `03__nested_depth5_duplicate_else_reject`, `03__nested_depth5_inactive_skips_bad_if_expr`, `03__nested_depth6_duplicate_else_reject`, `03__nested_depth6_inactive_skips_bad_elif_expr`, `03__nested_depth7_active_bad_elif_reject`, `03__nested_depth7_inactive_skips_bad_if_expr`, `03__nested_depth8_active_bad_elif_reject`, `03__nested_depth8_skips_bad_elif_taken_branch` | Explicit malformed directive-stack and malformed `#elif` expression coverage now exists, including missing-identifier fail-closed checks for `#ifdef`/`#ifndef` and depth-7/depth-8 active/inactive malformed-expression anchors | |
| Short-circuit logic correctness | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__pp_if_arith`, `03__if_elif_chain`, `03__pp_if_short_circuit`, `03__pp_if_ternary_short_circuit`, `03__pp_if_missing_colon_reject`, `03__pp_if_div_zero_reject`, `03__pp_elif_skips_dead_expr` | Logical and ternary short-circuit anchors are active; malformed expression negatives now fail closed, and skipped `#elif` expressions now have a direct positive anchor | |
| Edge numeric expressions | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__pp_if_arith`, `03__line_directive_builtin`, `03__pp_if_numeric_edges`, `03__pp_if_large_values`, `03__pp_if_mixed_signedness`, `03__pp_if_unsigned_shift_boundary`, `03__pp_if_bitwise_logic_mix`, `03__pp_if_missing_rhs_reject`, `03__pp_if_unmatched_paren_reject`, `03__pp_if_div_zero_reject`, `03__pp_if_nested_ternary_invalid_reject` | Numeric boundary coverage is substantially stronger now, with malformed `#if` expression failures and a malformed nested ternary anchor covered | |
| Undefined macro in `#if` | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens`, `diag` | `03__undef_defined` | | |

### 2.4 Include Handling

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `"file.h"` vs `<file.h>` | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__include_basic`, `03__quote_include_prefers_local`, `03__angle_include_uses_include_path`, `03__include_next_basic`, `03__include_next_missing`, `03__include_next_layered`, `03__include_next_same_dir_parent`, `03__include_next_angle_layered`, `03__include_next_pragma_once_chain`, `03__include_next_order_chain`, `03__include_next_mixed_delims_order`, `03__include_collision_quote_chain`, `03__include_collision_angle_chain`, `03__include_collision_quote_chain_long`, `03__include_collision_angle_chain_long`, `03__include_collision_mixed_local_angle_chain`, `03__include_extra_tokens_reject`, `03__include_next_extra_tokens_reject`, `03__include_macro_non_header_operand_reject`, `03__include_macro_trailing_tokens_reject`, `03__include_next_macro_trailing_tokens_reject` | Explicit quote-vs-angle split is covered, and `#include_next` now has positive, missing, same-dir-parent, angle, layered, pragma-once-chain, explicit 4-level search-order, mixed-delimiter order, and short/long plus local-entry mixed include-collision anchors. Include-operand strictness (trailing-token/non-header macro-expanded forms) is now fail-closed. | |
| Include guards | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens` | `03__include_guard` | | |
| Multiple include idempotence | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens` | `03__include_guard`, `03__pragma_once`, `03__include_next_pragma_once_chain` | Dedicated non-guarded non-guard idempotence is still missing; `#pragma once` is covered both directly and in `#include_next` chains | |
| Include cycles detection | `preprocessor` | [ ] | [x] | [x] | `in_progress` | `diag` | `03__include_cycle` | | |
| Missing include diagnostics | `preprocessor` | [ ] | [x] | [x] | `in_progress` | `diag` | `03__include_missing`, `03__skip_missing_include`, `03__include_next_missing`, `03__include_next_missing_mid_chain` | Missing include failure, dead-branch skip behavior, direct missing `#include_next`, and missing-mid-chain `#include_next` now all have anchors | |

### 2.5 Edge Cases

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Macro expanding to nothing | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens` | `03__macro_to_nothing` | | |
| Macro generating partial tokens | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__token_paste_numbers`, `03__macro_paste_expand`, `03__partial_token_reject` | | |
| Whitespace sensitivity | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens` | `03__function_like_macro`, `03__macro_arg_commas`, `03__stringize_spacing` | | |
| Line continuation behavior | `preprocessor` | [x] | [ ] | [x] | `in_progress` | `tokens`, `diag` | `03__line_continuation` | | |
| Unknown/invalid directive strictness | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__issue_unknown_directive_reject`, `03__unknown_directive_inactive_branch`, `03__nested_inactive_skips_unknown_directive`, `03__issue_endif_extra_tokens_reject`, `03__issue_ifdef_extra_tokens_reject`, `03__issue_ifndef_extra_tokens_reject`, `03__issue_else_extra_tokens_reject`, `03__issue_define_missing_identifier_reject`, `03__issue_define_non_identifier_reject`, `03__issue_define_param_missing_comma_reject`, `03__issue_define_param_trailing_comma_reject`, `03__issue_define_param_unclosed_reject`, `03__issue_ifdef_non_identifier_reject`, `03__issue_ifndef_non_identifier_reject`, `03__issue_undef_extra_tokens_reject`, `03__issue_undef_missing_identifier_reject`, `03__issue_undef_non_identifier_reject`, `03__nested_inner_ifdef_extra_tokens_reject`, `03__nested_inner_else_extra_tokens_reject` | Active unknown directives plus missing-identifier, non-identifier operand, malformed parameter-list, and trailing-token directive shapes now fail closed across top-level and nested contexts; inactive-branch unknown directives are explicitly skipped | |
| `#line` directive strictness | `preprocessor` | [x] | [x] | [x] | `in_progress` | `tokens`, `diag` | `03__line_directive_builtin`, `03__line_directive_missing_number_reject`, `03__line_directive_non_number_reject`, `03__line_directive_extra_tokens_reject`, `03__line_directive_macro_trailing_reject`, `03__line_directive_zero_reject`, `03__line_directive_hex_number_reject`, `03__line_directive_suffix_number_reject`, `03__line_directive_out_of_range_reject`, `03__line_directive_macro_out_of_range_reject`, `03__line_directive_charconst_reject` | Missing-line-number, non-number first operand, direct/macro-expanded trailing-token, strict positive-decimal numeric policy, out-of-range, and char-constant cases now fail closed | |

## Phase 3 — Declarations and Types (Parsing)

Primary buckets:

- `declarations`
- `parser`

Primary oracles:

- `ast`
- `diag`

### 3.1 Storage Classes

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `static` | `declarations` | [x] | [x] | [ ] | `in_progress` | `ast`, `diag` | `04__storage__static`, `04__storage__conflict_static_auto_reject` | | Conflict checks now fail closed |
| `extern` | `declarations` | [x] | [x] | [ ] | `in_progress` | `ast`, `diag` | `04__storage__extern`, `04__storage__missing_declarator`, `04__storage__conflict_extern_register_reject`, `04__storage__typedef_extern_reject` | | Missing declarator and conflict checks now fail closed |
| `auto` | `declarations` | [x] | [x] | [ ] | `in_progress` | `ast`, `diag` | `04__storage__auto`, `04__storage__file_scope_auto_reject`, `04__storage__conflict_static_auto_reject` | | File-scope and conflict checks now fail closed |
| `register` | `declarations` | [x] | [x] | [ ] | `in_progress` | `ast`, `diag` | `04__storage__register`, `04__storage__file_scope_register_reject`, `04__storage__conflict_extern_register_reject` | | File-scope and conflict checks now fail closed |
| `_Thread_local` | `declarations` | [ ] | [x] | [ ] | `unsupported` | `diag` | `04__thread_local_unsupported` | Emits an explicit unsupported diagnostic and now fails closed before semantic/codegen | Reserved keyword now lexes correctly |

### 3.2 Type Qualifiers

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `const` | `declarations` | [x] | [x] | [ ] | `in_progress` | `ast`, `diag` | `04__qualifiers__const`, `04__qualifiers__const_pointer_assign` | | Baseline positive + const-object mutation negatives added |
| `volatile` | `declarations` | [x] | [x] | [ ] | `in_progress` | `ast`, `diag` | `04__qualifiers__volatile`, `04__qualifiers__missing_type` | | Baseline positive + missing-type parse negative added |
| `restrict` | `declarations` | [x] | [x] | [ ] | `in_progress` | `ast`, `diag` | `04__qualifiers__restrict`, `04__qualifiers__restrict_non_pointer_reject` | | Non-pointer usage now fails closed |
| `_Atomic` | `declarations` | [ ] | [x] | [ ] | `unsupported` | `diag` | `04__atomic_unsupported` | Emits an explicit unsupported diagnostic and now fails closed before semantic/codegen | Reserved keyword now lexes correctly |

### 3.3 Primitive Types

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `char` signedness behavior | `declarations` | [x] | [x] | [ ] | `in_progress` | `ast`, `runtime` | `04__primitive__char_signedness`, `04__primitive__long_char_reject` | | Runtime/differential behavior still implementation-defined and pending |
| `short`, `int`, `long`, `long long` | `declarations` | [x] | [x] | [ ] | `in_progress` | `ast` | `04__primitive__integers`, `04__primitive__signed_unsigned_conflict_reject`, `04__primitive__short_long_conflict_reject` | | Baseline valid + conflict-negative coverage added |
| `float`, `double`, `long double` | `declarations` | [x] | [x] | [ ] | `in_progress` | `ast`, `runtime` | `04__primitive__floating`, `04__primitive__unsigned_float_reject` | | Baseline valid + invalid-modifier negative coverage added |
| `_Bool` | `declarations` | [x] | [x] | [ ] | `in_progress` | `ast`, `runtime` | `04__primitive__bool`, `04__primitive__signed_bool_reject` | | Baseline valid + invalid-modifier negative coverage added |
| Complex types | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__primitive__complex` | | Mark unsupported if absent |

### 3.4 Structs

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Basic struct | `declarations` | [x] | [ ] | [ ] | `in_progress` | `ast` | `04__struct_union_enum`, `04__struct__basic` | | Baseline named struct definitions covered |
| Nested struct | `declarations` | [x] | [ ] | [x] | `in_progress` | `ast` | `04__struct__nested` | | Nested aggregate shape coverage added |
| Anonymous struct | `declarations` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag` | `04__struct__anonymous` | | Anonymous member form now covered |
| Self-referential struct | `declarations` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag` | `04__struct__self_ref` | | Pointer-to-self field form covered |
| Flexible array member | `declarations` | [x] | [x] | [x] | `in_progress` | `ast`, `diag` | `04__bitfields_flex`, `04__struct__flex_not_last_reject`, `04__union__flex_array_reject` | | Positive + structural reject coverage added |
| Bitfields | `declarations` | [x] | [x] | [x] | `in_progress` | `ast`, `diag`, `runtime` | `04__bitfields_flex`, `04__bitfield__negative_width_reject`, `04__bitfield__width_exceeds_type_reject`, `04__bitfield__non_integral_type_reject`, `04__bitfield__zero_width_unnamed`, `04__bitfield__zero_width_named_reject`, `04__bitfield__typedef_integral`, `04__bitfield__enum_type`, `04__bitfield__unnamed`, `04__struct__bitfields` | | Width/type/zero-width and typedef/enum base forms covered; ABI runtime checks still pending |
| Duplicate member names | `declarations` | [ ] | [x] | [ ] | `in_progress` | `diag` | `04__struct__duplicate_member_reject`, `04__union__duplicate_member_reject` | | Duplicate field/member names now fail closed |

### 3.5 Unions

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Basic union | `declarations` | [x] | [ ] | [ ] | `in_progress` | `ast` | `04__struct_union_enum`, `04__union__basic` | | Baseline named union definitions covered |
| Anonymous union | `declarations` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag` | `04__union__anonymous` | | Anonymous union member form now covered |
| Nested union | `declarations` | [x] | [ ] | [x] | `in_progress` | `ast` | `04__union__nested` | | Nested union shape coverage added |
| Overlapping member correctness | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `04__union__overlap` | | ABI-sensitive |

### 3.6 Enums

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Explicit enumerator values | `declarations` | [x] | [ ] | [ ] | `in_progress` | `ast` | `04__enum__explicit_values` | | Baseline explicit assignments covered |
| Implicit increment behavior | `declarations` | [x] | [x] | [x] | `in_progress` | `ast`, `runtime` | `04__enum__implicit_increment`, `04__enum__implicit_from_min`, `04__enum__implicit_overflow_reject` | | Mixed implicit + explicit stepping plus overflow-negative coverage |
| Negative values | `declarations` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `04__enum__negative_values` | | Negative enumerator constants covered |
| Large values | `declarations` | [x] | [x] | [x] | `in_progress` | `ast`, `diag` | `04__enum__large_values`, `04__enum__out_of_range_reject` | | Range-boundary positives and out-of-range negatives are both covered |
| Integer constant-expression enforcement | `declarations` | [ ] | [x] | [x] | `in_progress` | `diag` | `04__enum__non_integer_value_reject`, `04__enum__overflow_expr_literal_reject` | | Non-integer and overflow-literal expression forms now fail closed |
| Duplicate enumerators | `declarations` | [ ] | [x] | [ ] | `in_progress` | `diag` | `04__enum__duplicate` | | Negative coverage added |

### 3.7 Declarator Complexity

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Pointer to function | `declarations` | [x] | [ ] | [ ] | `in_progress` | `ast` | `04__declarator__ptr_to_fn` | | |
| Function returning pointer | `declarations` | [x] | [ ] | [ ] | `in_progress` | `ast` | `04__declarator__fn_returns_ptr` | | |
| Pointer to array | `declarations` | [x] | [ ] | [ ] | `in_progress` | `ast` | `04__declarator__ptr_to_array` | | |
| Array of pointers | `declarations` | [x] | [ ] | [ ] | `in_progress` | `ast` | `04__declarator__array_of_ptrs` | | |
| Function pointer parameters | `declarations` | [x] | [ ] | [ ] | `in_progress` | `ast` | `04__declarator__fn_ptr_param` | | |
| Abstract declarators | `declarations` | [x] | [x] | [x] | `in_progress` | `ast`, `diag` | `04__declarator__abstract_fnptr_param`, `04__declarator__abstract_ptr_to_array_param`, `04__declarator__fn_returns_fn_reject`, `04__declarator__array_of_fn_reject`, `04__declarator__fn_returns_array_reject` | | Valid abstract parameter forms and invalid declarator-shape negatives covered |
| VLAs | `declarations` | [ ] | [x] | [x] | `in_progress` | `ast`, `diag`, `runtime` | `04__declarator__vla`, `04__struct__vla_field_reject` | | Aggregate-field VLA rejection now fails closed; full VLA declarator support coverage pending |
| Multidimensional arrays | `declarations` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `04__declarator__multi_array` | | Baseline multidimensional declaration coverage added |

### 3.8 Typedef

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Typedef of complex types | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__typedef__complex_type` | | |
| Typedef shadowing | `declarations` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag` | `04__typedef__shadowing_block` | | Block-scope typedef shadowing behavior now covered |
| Redefinition errors | `declarations` | [ ] | [x] | [ ] | `in_progress` | `diag` | `04__typedef__redefinition_reject` | | Conflicting typedef redefinition now fails closed |

## Phase 4 — Expressions

Primary buckets:

- `expressions`
- `semantic`

Primary oracles:

- `ast`
- `diag`
- `runtime`
- `ir`

### 4.1 Operator Precedence

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| All precedence tiers | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `05__precedence__all_tiers` | | Runtime differential follow-up still pending |
| Nested combinations | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `05__precedence__nested` | | Runtime differential follow-up still pending |

### 4.2 Unary Operations

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Unary `+`, `-`, `!` | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `05__unary__basic` | | Runtime differential follow-up still pending |
| Bitwise `~` | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `05__unary__bitwise_not` | | Runtime differential follow-up still pending |
| Address-of | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag` | `05__unary__address_of` | | |
| Dereference | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag`, `runtime` | `05__unary__deref` | | Runtime differential follow-up still pending |
| `sizeof` type vs expression | `expressions` | [x] | [x] | [x] | `in_progress` | `ast`, `runtime`, `diag` | `05__unary__sizeof_ambiguity`, `05__diag__sizeof_void_reject` | | Type-vs-expression disambiguation and invalid-operand diagnostics are active; runtime follow-up is promoted in wave 4 |
| `_Alignof` | `expressions` | [x] | [x] | [x] | `in_progress` | `ast`, `runtime`, `diag` | `05__unary_alignof`, `05__diag__alignof_void_reject`, `05__diag__alignof_expr_reject` | | Type-name path and invalid-operand diagnostics are active |

### 4.3 Binary Operations

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Arithmetic | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `05__binary__arithmetic` | | Runtime differential follow-up still pending |
| Bitwise | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `05__binary__bitwise` | | Runtime differential follow-up still pending |
| Logical | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `ir` | `05__binary__logical` | | Runtime/IR side-effect follow-up still pending |
| Relational | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `05__binary__relational` | | Runtime differential follow-up still pending |
| Equality | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `05__binary__equality` | | Runtime differential follow-up still pending |
| Shift signed and unsigned | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `diag` | `05__binary_shift_matrix` | | UB-sensitive edges still pending |
| Invalid shift widths | `expressions` | [ ] | [x] | [x] | `in_progress` | `diag` | `05__binary__invalid_shift_width`, `12__probe_invalid_shift_width` | | Active negative anchor promoted; probe now resolves |

### 4.4 Ternary Operator

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Nested ternary | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `differential` | `05__ternary__nested`, `05__runtime__nested_ternary_runtime`, `05__runtime__nested_ternary_false_chain_runtime`, `05__runtime__nested_ternary_deep_false_chain_runtime` | | Nested false-chain runtime variants now resolve and are promoted in wave 4 |
| Mixed types | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `diag` | `05__ternary_mixed_types` | | |
| Lvalue cases | `expressions` | [ ] | [x] | [x] | `in_progress` | `ast`, `diag`, `runtime` | `05__ternary__lvalue` | | |

### 4.5 Casts

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Explicit casts | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `05__casts__explicit` | | Runtime differential follow-up still pending |
| Illegal casts | `expressions` | [ ] | [x] | [x] | `in_progress` | `diag` | `05__casts__illegal` | | |
| Cast vs parenthesis ambiguity | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag` | `05__casts__ambiguity` | | Typedef-shadow disambiguation anchor now active |

### 4.6 Compound Literals

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Struct literal | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `05__compound_literal_struct` | | |
| Array literal | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `05__compound_literal_array` | | |
| Static storage compound literal | `expressions` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `ir` | `05__compound_literal__static_storage` | | Runtime/IR follow-up still pending |

### 4.7 `_Generic`

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `_Generic` selection | `expressions` | [ ] | [x] | [ ] | `in_progress` | `ast`, `diag`, `runtime` | `05__generic_unsupported_reject` | | Explicit unsupported-policy negative test |

## Phase 5 — Statements

Primary buckets:

- `statements-controlflow`
- `parser`
- `semantic`

Primary oracles:

- `ast`
- `diag`
- `runtime`
- `ir`

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Expression statement | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `09__stmt__expr` | | |
| Compound block | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `09__stmt__compound` | | |
| `if`/`else` and dangling else | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `09__dangling_else` | | |
| `switch` fallthrough | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `ir` | `09__switch_rules` | | |
| Duplicate `case` detection | `statements-controlflow` | [ ] | [x] | [x] | `in_progress` | `diag` | `09__switch_duplicate_case` | | |
| Non-constant `case` label | `statements-controlflow` | [ ] | [x] | [x] | `in_progress` | `diag` | `09__switch_nonconst_case` | | |
| `while` | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `ir` | `09__while_loop_basic` | | |
| `do-while` | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `ir` | `09__do_while_loop_basic` | | |
| `for` loop forms | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `ir` | `09__for_forms` | | |
| Declaration in `for` initializer | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag`, `runtime` | `09__for_decl_scope` | | |
| `break` and `continue` legality | `statements-controlflow` | [ ] | [x] | [x] | `in_progress` | `diag`, `runtime` | `09__break_continue_errors` | | |
| `goto` and labels | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `09__goto_labels` | | |
| Duplicate label error | `statements-controlflow` | [ ] | [x] | [x] | `in_progress` | `diag` | `09__label_redefinition` | | |
| `goto` across scopes | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `diag`, `runtime` | `09__goto_cross_init` | | |
| `return` basic correctness | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `09__stmt__return_basic` | | |
| Missing return | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `09__stmt__missing_return` | | Policy may be warning or error |
| Wrong return type | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `09__stmt__wrong_return_type` | | |

## Phase 6 — Initializers

Primary buckets:

- `initializers-layout`
- `semantic`

Primary oracles:

- `ast`
- `diag`
- `runtime`

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Scalar initialization | `initializers-layout` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `08__scalar_initializers` | | |
| Aggregate initialization | `initializers-layout` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `08__aggregate_brace_elision`, `08__union_designated_init` | | |
| Designated initializers | `initializers-layout` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `diag` | `08__designator_reset`, `08__nested_designators` | | |
| Nested designated initializers | `initializers-layout` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `08__nested_designators` | | |
| Mixed designated and non-designated | `initializers-layout` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag`, `runtime` | `08__mixed_designated_sequence`, `08__designator_reset` | | |
| Excess elements | `initializers-layout` | [ ] | [x] | [x] | `in_progress` | `diag` | `08__array_excess_elements_reject`, `08__string_init_too_long` | | |
| Missing elements and zero-fill | `initializers-layout` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `08__zero_init_aggregate`, `08__partial_designated_zero_fill` | | |
| Static initialization constant-expression enforcement | `initializers-layout` | [ ] | [x] | [x] | `in_progress` | `diag` | `08__static_constexpr` | | |

## Phase 7 — Semantics

Primary buckets:

- `semantic`
- `scopes-linkage`
- `functions-calls`
- `lvalues-rvalues`

Primary oracles:

- `diag`
- `ast`
- `runtime`

### 7.1 Scope Rules

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Block scope | `scopes-linkage` | [x] | [x] | [x] | `in_progress` | `ast`, `diag` | `10__shadowing_rules`, `10__block_extern_reference`, `10__block_scope_conflicting_types` | | |
| File scope | `scopes-linkage` | [x] | [x] | [x] | `in_progress` | `ast`, `diag` | `10__extern_array_consistent_definition`, `10__extern_type_mismatch`, `10__var_function_name_conflict` | | Extern-array consistency anchor is active and passing |
| Shadowing | `scopes-linkage` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag` | `10__shadowing_rules`, `10__block_extern_reference` | | |
| Tentative definitions | `scopes-linkage` | [x] | [x] | [x] | `in_progress` | `ast`, `diag`, `runtime` | `10__tentative_definition`, `10__tentative_definition_multi_tu`, `10__tentative_static_conflict` | | |
| Multiple definition errors | `scopes-linkage` | [ ] | [x] | [x] | `in_progress` | `diag` | `10__multiple_static_defs`, `10__var_function_name_conflict` | | Multi-file external-definition collision still pending |

### 7.2 Type Conversions

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Integer promotions | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `07__conv__integer_promotions` | | |
| Usual arithmetic conversions | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `07__conv__usual_arith` | | |
| Signed vs unsigned comparisons | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `07__conv__signed_unsigned_compare` | | |
| Pointer conversions | `semantic` | [x] | [x] | [x] | `in_progress` | `ast`, `diag`, `runtime` | `07__void_pointer_roundtrip`, `07__void_pointer_arith_reject` | | |
| Array-to-pointer decay | `semantic` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `07__array_decay_param` | | |
| Function-to-pointer decay | `semantic` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `07__function_decay_call` | | |

### 7.3 Lvalue and Rvalue Correctness

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Assignment target rules | `lvalues-rvalues` | [x] | [x] | [x] | `in_progress` | `diag`, `ast` | `06__assignability_errors`, `06__array_assignment_reject`, `06__ternary_assignment_reject` | | |
| Const correctness enforcement | `lvalues-rvalues` | [x] | [x] | [x] | `in_progress` | `diag`, `ast` | `06__string_literal_mutability`, `06__const_pointer_write_reject` | | |
| Volatile propagation | `lvalues-rvalues` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `06__volatile_read_write` | | |

### 7.4 Function Semantics

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Prototype matching | `functions-calls` | [x] | [x] | [x] | `in_progress` | `diag`, `ast` | `11__prototype_call`, `11__argument_type_mismatch` | | |
| Argument count mismatch | `functions-calls` | [ ] | [x] | [x] | `in_progress` | `diag` | `11__prototype_too_few_args`, `11__prototype_too_many_args` | | |
| Variadic correctness | `functions-calls` | [x] | [x] | [x] | `in_progress` | `diag`, `runtime` | `11__variadic_call`, `11__variadic_requires_named_param_reject` | | |
| Return type enforcement | `functions-calls` | [x] | [x] | [x] | `in_progress` | `diag`, `runtime` | `11__return_type_mismatch`, `11__void_return_value`, `11__nonvoid_missing_return` | | |

### 7.5 Struct and Union Semantics

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Member access correctness | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `07__agg__member_access` | | |
| Invalid member access | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `07__agg__invalid_member` | | |
| Offset correctness | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `07__agg__offsets` | | ABI-sensitive |

### 7.6 Constant Expressions

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Required in array size | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `07__constexpr__array_size` | | |
| Required in case labels | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `07__constexpr__case_label` | | |
| Required in static initializer | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `07__constexpr__static_init` | | |

## Phase 8 — Code Generation and Runtime

Primary buckets:

- `codegen-ir`
- `runtime`
- `differential`

Primary oracles:

- `runtime`
- `ir`
- `differential`

### 8.1 Arithmetic Correctness

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Signed overflow behavior | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `14__arith__signed_overflow` | | UB-tag required |
| Unsigned wraparound | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__arith__unsigned_wrap`, `probes/runtime/14__probe_unsigned_wrap.c` | | Differential probe now resolves; active-suite promotion remains pending |
| Division and modulo edge cases | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `diag` | `14__runtime_div_mod_edges` | | Division by zero is UB if runtime |

### 8.2 Floating-Point Correctness

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Precision behavior | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_float_precision` | | Target-specific tolerance may be needed |
| NaN propagation | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__float__nan`, `probes/runtime/14__probe_float_nan.c` | | Differential probe now resolves; active-suite promotion remains pending |
| Infinity behavior | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_float_infinity` | | Active differential runtime anchor added |

### 8.3 Control Flow

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Nested loops | `codegen-ir` | [x] | [ ] | [x] | `in_progress` | `runtime`, `ir` | `13__ir_nested_loops` | | |
| Short-circuit logic | `codegen-ir` | [x] | [ ] | [x] | `in_progress` | `runtime`, `ir`, `differential` | `13__ir_short_circuit`, `13__ir_short_circuit_side_effect` | | |
| Switch lowering correctness | `codegen-ir` | [x] | [ ] | [x] | `in_progress` | `runtime`, `ir`, `differential` | `13__ir_switch`, `14__runtime_switch_simple`, `probes/runtime/15__probe_switch_loop_lite.c`, `probes/runtime/15__probe_switch_loop_mod5.c` | | Simple and looped switch differential probes are currently green |

### 8.4 Memory

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Stack allocation | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `ir` | `14__runtime_large_stack` | | |
| Struct layout correctness | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_struct_layout_alignment` | | ABI-sensitive tag active |
| Array indexing | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_pointer_arith`, `15__torture__large_array_stress` | | Includes larger local-array stress coverage |
| Pointer arithmetic | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_pointer_arith` | | |
| Alignment correctness | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `ir`, `differential` | `14__runtime_struct_layout_alignment` | | Offset-mod-alignment check active |

### 8.5 Function Calls

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Parameter passing | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_function_pointer`, `14__runtime_struct_return`, `15__torture__many_params` | | Added multi-parameter ABI stress anchor |
| Variadic calls | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_variadic_calls` | | Active variadic call-site anchor added |
| Recursion | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime` | `14__runtime_recursion` | | |
| Function pointers | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_function_pointer` | | |
| Struct return | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_struct_return` | | ABI-sensitive |

### 8.6 Stress Runtime

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Large stack frames | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime` | `14__runtime_large_stack` | | Stress path |
| Deep recursion | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_deep_recursion` | | Stress path |

## Phase 9 — Diagnostics

Primary buckets:

- `diagnostics-recovery`
- `semantic`
- `parser`

Primary oracle:

- `diag`

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Syntax errors | `diagnostics-recovery` | [x] | [x] | [x] | `in_progress` | `diag` | `12__missing_semicolon`, `12__bad_paren_expr`, `12__for_missing_paren` | | |
| Type mismatch | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_type_mismatch` | | Active non-scalar-to-scalar assignment mismatch anchor added |
| Undeclared identifiers | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_undeclared_identifier` | | |
| Redeclarations | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_redeclaration_conflict` | | |
| Incompatible pointer types | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_incompatible_ptr`, `probes/diagnostics/12__probe_incompatible_ptr_assign.c` | | Probe emits expected diagnostic; active-suite promotion remains pending |
| Illegal casts | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_illegal_cast`, `probes/diagnostics/12__probe_illegal_struct_to_int_cast.c` | | Probe emits expected diagnostic; active-suite promotion remains pending |
| Invalid storage class usage | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_invalid_storage_class` | | |
| Invalid initializers | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_invalid_initializer` | | |

Diagnostics acceptance rule:

- Verify category and line mapping.
- Do not require exact message string matches.

## Phase 10 — Edge and Torture

Primary buckets:

- `torture-differential`
- `runtime`
- `parser`

Primary oracles:

- `runtime`
- `diag`
- `differential`

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Extremely long expressions | `torture-differential` | [x] | [ ] | [x] | `in_progress` | `runtime`, `diag` | `15__torture__long_expr` | | Active runtime stress anchor added (100-term chain) |
| Deep nesting | `torture-differential` | [x] | [ ] | [x] | `in_progress` | `runtime`, `diag`, `differential` | `15__torture__deep_nesting`, `probes/runtime/15__probe_switch_loop_lite.c`, `probes/runtime/15__probe_switch_loop_mod5.c` | | Nested-switch differential probe set is currently green |
| Many declarations in one file | `torture-differential` | [x] | [ ] | [x] | `in_progress` | `runtime`, `diag` | `15__torture__many_decls`, `15__torture__many_globals` | | Active local+global declaration-density anchors |
| Large struct definitions | `torture-differential` | [x] | [ ] | [x] | `in_progress` | `runtime`, `diag` | `15__torture__large_struct` | | ABI-sensitive; tagged `abi_sensitive: true` |
| Pathological declarators | `torture-differential` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag`, `runtime`, `differential` | `15__torture__pathological_decl`, `15__torture__path_decl_nested`, `04__probe_deep_declarator_call_only`, `04__probe_deep_declarator_codegen_hang` | | Declarator runtime probes now resolve; keep torture expansion as follow-up |
| Fuzz-compatible harness design | `torture-differential` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `15__torture__fuzz_harness` | | Infrastructure item |
| Malformed input robustness | `torture-differential` | [ ] | [x] | [x] | `in_progress` | `diag` | `15__torture__malformed_no_crash`, `15__torture__malformed_unclosed_comment_no_crash`, `15__torture__malformed_unbalanced_block_no_crash` | | Must never crash compiler |

## Phase 11 — Implementation-Defined and UB Tracking

This phase is not a separate language feature bucket. It is a tagging layer that
must be applied across the suite.

Every applicable test should be marked when it:

- Uses undefined behavior
- Relies on implementation-defined signedness
- Depends on padding or ABI layout details
- Depends on floating formatting or platform-specific libc behavior

Tracking table:

| Tag Type | Required | Notes |
| --- | --- | --- |
| `ub: true` | Yes for undefined behavior | Exclude from strict differential runtime checks |
| `impl_defined: true` | Yes where language permits platform variance | Keep diagnostics explicit |
| `abi_sensitive: true` | Yes for layout or calling convention assumptions | Avoid brittle cross-target comparisons |
| `float_tolerance: true` | When output may need tolerant comparison | Prefer structured compare over raw strings |

## Bucket Run Log

Use this section as the current campaign tracker. Add dated entries as buckets
are worked.

Entries are historical snapshots at the time they were recorded; older entries
may describe blockers that were resolved later.

Suggested entry format:

```md
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

## Completion Rule

A phase is only complete when:

- Every row in that phase has planned tests
- Valid coverage exists
- Negative coverage exists where applicable
- Edge coverage exists
- Unsupported features are explicitly marked
- Current failures for the bucket are resolved or intentionally tracked
- The full bucket passes under the current harness target
