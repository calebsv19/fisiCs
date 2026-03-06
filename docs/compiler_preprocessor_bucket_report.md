# Preprocessor Bucket Baseline Report

Date:

- March 3, 2026
- March 5, 2026 (updated)

Scope:

- Freeze the lexer bucket as the first completed major pass
- Start the `preprocessor` bucket expansion campaign
- Record the first substantial set of active preprocessor behaviors and the
  first fix-first issue repros before deeper implementation changes

This is the active working reference for the bucket after lexer.

## Current Suite Result

The active `tests/final` preprocessor slice is currently green.

Executed using the metadata-driven suite from:

- `tests/final/meta/03-preprocessor.json`

Current active preprocessor cases:

- `03__object_like_macro`
- `03__function_like_macro`
- `03__stringify_and_paste`
- `03__variadic_macro`
- `03__defined_expr`
- `03__defined_macro_expansion`
- `03__line_directive_builtin`
- `03__include_basic`
- `03__include_guard`
- `03__pp_if_arith`
- `03__macro_redefinition`
- `03__nested_macro_expansion`
- `03__macro_arg_commas`
- `03__if_defined_paren`
- `03__stringize_spacing`
- `03__token_paste_numbers`
- `03__variadic_empty`
- `03__if_elif_chain`
- `03__undef_defined`
- `03__macro_paste_expand`
- `03__macro_to_nothing`
- `03__line_continuation`
- `03__empty_fn_args`
- `03__recursive_block`
- `03__include_missing`
- `03__partial_token_reject`
- `03__ifdef_ifndef`
- `03__pragma_once`
- `03__skip_missing_include`
- `03__nested_conditionals`
- `03__else_without_if`
- `03__endif_without_if`
- `03__unterminated_if_reject`
- `03__elif_after_else`
- `03__include_cycle`
- `03__macro_arg_count_reject`
- `03__quote_include_prefers_local`
- `03__angle_include_uses_include_path`
- `03__pp_if_short_circuit`
- `03__pp_if_numeric_edges`
- `03__pp_if_large_values`
- `03__pp_if_mixed_signedness`
- `03__pp_if_ternary_short_circuit`
- `03__pp_if_unsigned_shift_boundary`
- `03__pp_if_missing_rhs_reject`
- `03__pp_if_missing_colon_reject`
- `03__pp_if_unmatched_paren_reject`
- `03__pp_if_div_zero_reject`
- `03__pp_elif_missing_rhs_reject`
- `03__pp_elif_div_zero_reject`
- `03__include_next_basic`
- `03__pp_elif_missing_colon_reject`
- `03__pp_elif_unmatched_paren_reject`
- `03__pp_elif_skips_dead_expr`
- `03__inactive_nested_if_skips_bad_expr`
- `03__include_next_missing`
- `03__nested_duplicate_else_reject`
- `03__nested_elif_after_else_reject`
- `03__inactive_outer_skips_malformed_elif`
- `03__macro_mutual_recursive_block`
- `03__include_next_layered`
- `03__macro_arg_too_many_reject`
- `03__macro_arg_missing_comma_reject`
- `03__macro_redef_conflict_tokens`
- `03__macro_redef_identical_tokens`
- `03__include_next_same_dir_parent`
- `03__include_next_angle_layered`
- `03__nested_active_bad_elif_reject`
- `03__pp_if_bitwise_logic_mix`
- `03__pp_if_nested_ternary_invalid_reject`
- `03__variadic_missing_named_arg_reject`
- `03__macro_expansion_limit_reject`
- `03__include_next_pragma_once_chain`
- `03__vaopt_basic`
- `03__error_directive_reject`
- `03__warning_directive_diag`
- `03__warning_inactive_branch`
- `03__pp_if_shift_negative_boundary`
- `03__macro_invocation_requires_adjacent_lparen`
- `03__macro_arg_nested_paren_grouping`
- `03__issue_unknown_directive_reject`
- `03__issue_endif_extra_tokens_reject`
- `03__issue_ifdef_extra_tokens_reject`
- `03__issue_ifndef_extra_tokens_reject`
- `03__issue_else_extra_tokens_reject`
- `03__gnu_comma_vaargs`
- `03__unknown_directive_inactive_branch`
- `03__line_directive_missing_number_reject`
- `03__line_directive_non_number_reject`
- `03__line_directive_extra_tokens_reject`
- `03__nested_inner_ifdef_extra_tokens_reject`
- `03__nested_inner_else_extra_tokens_reject`
- `03__nested_inactive_skips_unknown_directive`
- `03__include_next_order_chain`
- `03__line_directive_macro_trailing_reject`
- `03__nested_deep_inner_else_extra_tokens_reject`
- `03__nested_deep_active_bad_elif_reject`
- `03__nested_deep_inactive_skips_unknown_directive`
- `03__include_next_mixed_delims_order`
- `03__line_directive_zero_reject`
- `03__line_directive_hex_number_reject`
- `03__line_directive_suffix_number_reject`
- `03__nested_depth5_duplicate_else_reject`
- `03__nested_depth5_inactive_skips_bad_if_expr`
- `03__include_collision_quote_chain`
- `03__include_collision_angle_chain`
- `03__line_directive_out_of_range_reject`
- `03__line_directive_macro_out_of_range_reject`
- `03__line_directive_charconst_reject`
- `03__nested_depth6_duplicate_else_reject`
- `03__nested_depth6_inactive_skips_bad_elif_expr`
- `03__include_collision_quote_chain_long`
- `03__include_collision_angle_chain_long`
- `03__issue_ifdef_missing_identifier_reject`
- `03__issue_ifndef_missing_identifier_reject`
- `03__issue_define_missing_identifier_reject`
- `03__issue_define_non_identifier_reject`
- `03__issue_undef_extra_tokens_reject`
- `03__issue_undef_missing_identifier_reject`
- `03__macro_expansion_limit_chain_ok`
- `03__macro_expansion_limit_chain6_ok`
- `03__macro_expansion_limit_chain6_reject`
- `03__issue_define_param_missing_comma_reject`
- `03__issue_define_param_trailing_comma_reject`
- `03__issue_define_param_unclosed_reject`
- `03__include_collision_mixed_local_angle_chain`
- `03__macro_redef_obj_to_function_tokens`
- `03__macro_redef_function_to_object_tokens`
- `03__macro_redef_function_conflict_tokens`
- `03__nested_depth7_active_bad_elif_reject`
- `03__nested_depth7_inactive_skips_bad_if_expr`
- `03__include_next_missing_mid_chain`
- `03__defined_keyword_name`
- `03__defined_keyword_name_no_paren`
- `03__defined_non_identifier_reject`
- `03__issue_ifdef_non_identifier_reject`
- `03__issue_ifndef_non_identifier_reject`
- `03__issue_undef_non_identifier_reject`
- `03__nested_depth8_active_bad_elif_reject`
- `03__nested_depth8_skips_bad_elif_taken_branch`
- `03__include_extra_tokens_reject`
- `03__include_next_extra_tokens_reject`
- `03__include_macro_non_header_operand_reject`
- `03__include_macro_trailing_tokens_reject`
- `03__include_next_macro_trailing_tokens_reject`

Current result:

- 145 / 145 passing
- 0 known active expectation mismatches inside the current preprocessor slice

## What The Current Preprocessor Suite Covers

Current validated areas:

- Basic object-like macro replacement
- Function-like macro parameter substitution
- Nested macro rescans
- Stringification and token pasting
- Variadic macros, including empty `__VA_ARGS__`
- `defined` handling in `#if` (including keyword-token macro names and
  no-parentheses form)
- `defined` operand-shape strictness (`defined(...)` rejects non-identifiers)
- `#if`, `#ifdef`, `#ifndef`, nested stacks, and `#elif` selection
- Local include expansion
- Include guards
- `#pragma once`
- Include-cycle detection
- Missing include diagnostics
- Dead-branch skipping of missing includes
- Macro argument-count mismatch diagnostics
- Quote-vs-angle include resolution split
- Macro redefinition policy as currently implemented
- Macro argument grouping around commas
- Macro argument expansion before token paste
- Macros expanding to nothing
- Line continuation inside directives
- Directive-stack diagnostics for malformed `#else`, `#endif`, and unclosed
  conditionals
- Partial-token generation rejection during preprocessing
- `#line` interaction with built-in line evaluation inside preprocessor flow
- Dedicated `#if` short-circuit logic anchors
- Stable numeric-edge `#if` anchors
- Large-value unsigned `#if` anchors
- Mixed-signedness and ternary `#if` anchors
- Unsigned high-bit shift boundary `#if` anchors
- Fail-closed malformed `#if` expression diagnostics
- Fail-closed malformed `#elif` expression diagnostics
- `#elif` dead-branch skipping when an earlier branch is already selected
- Nested inactive-branch skipping of malformed inner `#if` expressions
- Nested duplicate-`#else` and nested `#elif`-after-`#else` diagnostics
- Inactive-outer-branch skipping for malformed nested `#elif` expressions
- Deeper nested malformed active/inactive conditional-expression behavior
  (depth-7 and depth-8 anchors)
- Macro recursion blocking anchors for self-recursive and mutual-recursive expansions
- More explicit macro-arity mismatch negatives (too many args, missing comma)
- Variadic missing-named-argument calls now fail closed in preprocessing
- Macro redefinition behavior anchors for identical vs conflicting redefines
- Macro redefinition cross-kind behavior anchors (object↔function) with
  latest-definition expansion policy
- Active `#include_next` positive, missing-include, same-dir-parent, and layered multi-path anchors
- Mid-chain missing `#include_next` resolution failure anchors
- Strict include-operand shape policy:
  trailing tokens after `#include` / `#include_next` operands now fail closed
  (including macro-expanded operand forms)
- Additional numeric-expression stress via mixed bitwise/logical `#if` anchors
- Malformed nested ternary `#if` rejection anchor
- Macro expansion-depth-limit diagnostics via per-test environment overrides
- `#include_next` + `#pragma once` repeated-include chain determinism
- `__VA_OPT__` optional-token behavior anchors
- `#error` / `#warning` directive behavior anchors (active + inactive branches)
- Shift-width policy anchor for negative-shift `#if` expressions
- Macro invocation adjacency and nested-parenthesis argument grouping anchors
- Unknown-directive fail-closed policy in active branches
- Trailing-token fail-closed policy for `#else`, `#endif`, `#ifdef`, `#ifndef`
- Explicit unsupported-policy diagnostics for GNU `, ##__VA_ARGS__`
- Unknown directives in inactive branches are explicitly skipped
- `#line` fail-closed policy for missing number, non-number first operand, and
  trailing tokens
- `#line` fail-closed policy for macro-expanded trailing-token forms
- Strict positive-decimal-only `#line` line-number policy
- `#line` out-of-range and non-numeric token-shape rejection
- Nested directive strictness hardening in deeper stack positions
- Missing/same-line identifier and trailing-token strictness for
  `#define`, `#undef`, `#ifdef`, and `#ifndef`
- Non-identifier operand strictness for `defined(...)`, `#ifdef`, `#ifndef`,
  and `#undef`
- Function-like `#define` parameter-list strictness for malformed forms
  (missing comma, trailing comma, unclosed list)
- Four-level explicit `#include_next` search-order chain behavior
- Four-level mixed quote/angle `#include_next` search-order chain behavior
- Include-collision chain behavior for quote vs angle entrypoints
- Include-collision chain behavior across longer include-path hop counts
- Include-collision chain behavior for local-entry + alternating quote/angle
  `#include_next` hops across four include-path slots
- Positive expansion-limit-chain behavior under explicit
  `FISICS_MACRO_EXP_LIMIT` configuration
- Deeper expansion-limit boundary behavior (pass at N, fail at N-1)

This is now a solid first-wave preprocessor slice, but it is not yet a
complete preprocessor audit.

## Known Gaps Before The Main Failure Campaign

Important preprocessor gaps still not covered by active tests:

- More malformed nested-conditional expression diagnostics in deeper stacks
- Deeper macro recursion stress beyond the current self + mutual recursion
  anchors and expansion-limit boundary anchors
- More explicit macro redefinition diagnostic policy matching
- Additional include search-path layering and more complex `#include_next` variants
- More malformed directive-form coverage beyond current hardening anchors
  (for example remaining uncommon malformed `#line` variants and other less common invalid forms)
- Decide whether GNU `, ##__VA_ARGS__` should stay unsupported long-term or be
  promoted to explicit extension support in a future compatibility pass

Known implementation note:

- Recursive blocking is now covered by both `03__recursive_block` (self) and
  `03__macro_mutual_recursive_block` (mutual).
- Expansion-depth-limit behavior is now covered with both shallow and deeper
  boundary pass/fail anchors (`03__macro_expansion_limit_chain_ok`,
  `03__macro_expansion_limit_chain6_ok`,
  `03__macro_expansion_limit_reject`,
  `03__macro_expansion_limit_chain6_reject`).
- The next recursion pass should target deeper expansion chains and explicit
  recursion diagnostics, not the already-covered base cases.

## Working References For The Preprocessor Bucket

Use these files as the main preprocessor working set:

- `tests/final/03-preprocessor.md`
- `tests/final/meta/03-preprocessor.json`
- `docs/compiler_behavior_coverage_checklist.md` (Phase 2)
- `docs/compiler_test_workflow_guide.md`

## Next Bucket Process

The next preprocessor passes should follow the same pattern that closed the
lexer major pass:

1. Keep expanding the active preprocessor test set across the remaining Phase 2 rows.
2. Preserve fix-first issue repros outside the active manifest until the broken
   behavior is corrected.
3. Group failures by behavior class (macro expansion, conditionals, includes,
   diagnostics).
4. Fix by behavior cluster, not by one-off spot patching.
5. Re-run the full preprocessor bucket until the bucket baseline is stable.

## Current Bucket Status

Current bucket status:

- `in_progress`

Reason:

- The current preprocessor slice is stable and materially broader than the
  initial anchor set
- The bucket still has open Phase 2 rows even though `pp_expr` numeric,
  malformed-expression, and include-path coverage are substantially stronger
  than the earlier baseline
- More negative and stress coverage is still needed before the bucket can be
  considered complete

The bucket is now in the same discovery-and-expansion cycle that closed lexer,
with the first substantial active coverage wave in place.
