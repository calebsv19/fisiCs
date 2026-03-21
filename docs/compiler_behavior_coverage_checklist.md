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
| Complex types | `declarations` | [x] | [x] | [x] | `in_progress` | `ast`, `diag`, `runtime` | `04__primitive__complex`, `04__primitive__complex_int_reject`, `04__primitive__complex_unsigned_reject`, `04__primitive__complex_missing_base_reject` | | Baseline and legality negative matrix are active |

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
| Overlapping member correctness | `declarations` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `04__union__overlap`, `probes/runtime/04__probe_union_overlap_runtime.c` | | Runtime+differential overlap anchor is active (ABI-sensitive) |

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
| Typedef of complex types | `declarations` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `04__typedef__complex_type` | | Baseline complex typedef usage is active; conflict negatives still pending |
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
| Expression statement | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast` | `09__stmt__expr` | | Baseline expression/empty-statement path is active |
| Compound block | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast` | `09__stmt__compound` | | Nested block statement baseline is active |
| `if`/`else` and dangling else | `statements-controlflow` | [x] | [x] | [x] | `in_progress` | `ast`, `runtime`, `diag` | `09__dangling_else`, `09__parserdiag__if_missing_then_stmt_reject`, `09__parserdiag__else_missing_stmt_reject` | | Added malformed `if`/`else` statement parser-diag anchors |
| `switch` fallthrough | `statements-controlflow` | [x] | [x] | [x] | `in_progress` | `ast`, `runtime`, `ir`, `diag` | `09__switch_rules`, `09__parserdiag__switch_missing_rparen_reject` | | Added malformed `switch` header parser-diag anchor |
| Duplicate `case` detection | `statements-controlflow` | [ ] | [x] | [x] | `in_progress` | `diag` | `09__switch_duplicate_case` | | |
| Non-constant `case` label | `statements-controlflow` | [ ] | [x] | [x] | `in_progress` | `diag` | `09__switch_nonconst_case` | | |
| `while` | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `ir` | `09__while_loop_basic` | | |
| `do-while` | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime`, `ir` | `09__do_while_loop_basic` | | |
| `for` loop forms | `statements-controlflow` | [x] | [x] | [x] | `in_progress` | `ast`, `runtime`, `ir`, `diag` | `09__for_forms`, `09__parserdiag__for_missing_first_semicolon_reject` | | Added malformed `for` header separator parser-diag anchor |
| Declaration in `for` initializer | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast`, `diag`, `runtime` | `09__for_decl_scope` | | |
| `break` and `continue` legality | `statements-controlflow` | [ ] | [x] | [x] | `in_progress` | `diag`, `runtime` | `09__break_continue_errors` | | |
| `goto` and labels | `statements-controlflow` | [x] | [x] | [x] | `in_progress` | `ast`, `runtime`, `diag` | `09__goto_labels`, `09__goto_undefined_label_reject`, `09__goto_undefined_label_nested_reject` | | Undefined-label negatives now include nested-scope shape |
| Duplicate label error | `statements-controlflow` | [ ] | [x] | [x] | `in_progress` | `diag` | `09__label_redefinition` | | |
| `goto` across scopes | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `diag`, `runtime` | `09__goto_cross_init` | | |
| `return` basic correctness | `statements-controlflow` | [x] | [ ] | [x] | `in_progress` | `ast`, `runtime` | `09__stmt__return_basic` | | Baseline `return` statement path is active |
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

Active expansion plan:

- `docs/plans/runtime_bucket_14_execution_plan.md`

### 8.1 Arithmetic Correctness

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Signed overflow behavior | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `14__runtime_signed_overflow_ub` | | UB-tag required |
| Unsigned wraparound | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_unsigned_wraparound` | | Promoted from `14__probe_unsigned_wrap` and now active in runtime suite |
| Division and modulo edge cases | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_div_mod_edges`, `14__runtime_signed_div_mod_sign_matrix` | | Division by zero is UB if runtime |

### 8.2 Floating-Point Correctness

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Precision behavior | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_float_precision`, `14__probe_float_cast_roundtrip` | `14__probe_float_cast_roundtrip` mismatch (`fisics=1 0 1 0`, `clang=1 1 1 1`) | Target-specific tolerance may be needed; cast-roundtrip lane is currently blocked |
| NaN propagation | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_nan_propagation`, `14__runtime_nan_comparisons` | | Propagation and comparison anchors are now both active in runtime suite |
| Infinity behavior | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_float_infinity` | | Active differential runtime anchor added |

### 8.3 Control Flow

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Nested loops | `codegen-ir` | [x] | [ ] | [x] | `in_progress` | `runtime`, `ir` | `13__ir_nested_loops`, `14__runtime_loop_continue_break_phi` | | Runtime differential anchor promoted from probe set |
| Short-circuit logic | `codegen-ir` | [x] | [ ] | [x] | `in_progress` | `runtime`, `ir`, `differential` | `13__ir_short_circuit`, `13__ir_short_circuit_side_effect`, `14__runtime_short_circuit_chain_effects` | | Runtime differential anchor promoted from probe set |
| Switch lowering correctness | `codegen-ir` | [x] | [ ] | [x] | `in_progress` | `runtime`, `ir`, `differential` | `13__ir_switch`, `14__runtime_switch_simple`, `14__runtime_switch_loop_control_mix` | | Mixed control-edge runtime case was promoted from probe and is stable |

### 8.4 Memory

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Stack allocation | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `ir` | `14__runtime_large_stack` | | |
| Struct layout correctness | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_struct_layout_alignment` | | ABI-sensitive tag active |
| Array indexing | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_pointer_arith`, `15__torture__large_array_stress` | | Includes larger local-array stress coverage |
| Pointer arithmetic | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_pointer_arith`, `14__runtime_pointer_stride_long_long` | | Promoted long-long stride/difference differential anchor; element-width scaling bug fixed |
| Alignment correctness | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `ir`, `differential` | `14__runtime_struct_layout_alignment` | | Offset-mod-alignment check active |

### 8.5 Function Calls

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Parameter passing | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_function_pointer`, `14__runtime_struct_return`, `14__runtime_struct_mixed_width_pass_return`, `15__torture__many_params` | | Added mixed-width aggregate ABI runtime anchor |
| Variadic calls | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_variadic_calls` | | Active variadic call-site anchor added |
| Recursion | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime` | `14__runtime_recursion` | | |
| Function pointers | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_function_pointer` | | |
| Struct return | `runtime` | [x] | [ ] | [x] | `in_progress` | `runtime`, `differential` | `14__runtime_struct_return`, `14__runtime_struct_copy_update`, `14__runtime_struct_mixed_width_pass_return` | | Struct return/copy runtime differential anchors promoted |

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
| Undeclared identifiers | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_undeclared_identifier`, `12__diag_line_anchor__undeclared_calls`, `12__diag_line_anchor__macro_two_callsites` | | Includes direct and macro-expanded callsite line-anchor checks |
| Redeclarations | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_redeclaration_conflict` | | |
| Incompatible pointer types | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_incompatible_ptr`, `12__diag_line_anchor__ptr_then_cast`, `probes/diagnostics/12__probe_incompatible_ptr_assign.c` | | Active-suite promotion completed in diagnostics-recovery wave 2; wave 3 adds ordering/line-anchor chaining |
| Illegal casts | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_illegal_cast`, `12__diag_line_anchor__ptr_then_cast`, `probes/diagnostics/12__probe_illegal_struct_to_int_cast.c` | | Active-suite promotion completed in diagnostics-recovery wave 2; wave 3 adds ordering/line-anchor chaining |
| Invalid storage class usage | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_invalid_storage_class` | | |
| Invalid initializers | `diagnostics-recovery` | [ ] | [x] | [x] | `in_progress` | `diag` | `12__diag_invalid_initializer` | | |
| Primary vs follow-on diagnostic ordering | `diagnostics-recovery` | [x] | [x] | [x] | `in_progress` | `diag` | `12__diag_order__nonvoid_return_then_unreachable` | | Wave 3 ordering anchor active |
| Cascade suppression bounds | `diagnostics-recovery` | [x] | [ ] | [x] | `in_progress` | `diag` | `12__diag_cascade_bound__single_unreachable_region` | | Wave 3 unreachable-region bounding anchor active |
| Parser diagnostic category/line export assertions | `diagnostics-recovery` | [x] | [x] | [x] | `in_progress` | `diag` | `12__parserdiag__missing_semicolon`, `12__parserdiag__for_missing_paren`, `12__parserdiag__bad_paren_expr`, `12__parserdiag__stray_else_recovery`, `12__probe_diagjson_while_missing_lparen`, `12__probe_diagjson_do_while_missing_semicolon` | | Harness validates parser diags via exported diagnostics JSON (`.pdiag`) where available; selected recovery tests now also force frontend diagnostic capture via `capture_frontend_diag: true` to avoid silent parser-error drops in `.diag` expectations |

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

The rolling execution log now lives in:

- `docs/compiler_behavior_coverage_run_log.md`

Keep this checklist focused on coverage requirements and status.

## Completion Rule

A phase is only complete when:

- Every row in that phase has planned tests
- Valid coverage exists
- Negative coverage exists where applicable
- Edge coverage exists
- Unsupported features are explicitly marked
- Current failures for the bucket are resolved or intentionally tracked
- The full bucket passes under the current harness target
