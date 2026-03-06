# Preprocessor

## Scope
Macro expansion rules, #if evaluation, and include handling.

## Validate
- Object/function-like macros, nested expansion, rescans.
- # and ## semantics.
- Variadic macros and __VA_ARGS__.
- defined() expression handling in #if.
- `#ifdef` / `#ifndef` and nested conditional stacks.
- Include resolution, `#pragma once`, and include-cycle failure paths.
- Directive-stack diagnostics (`#else` / `#endif` / unclosed conditionals).

## Acceptance Checklist
- Object-like and function-like macros expand correctly.
- Nested macros rescan and avoid re-expanding the active macro.
- Stringizing and token pasting produce valid tokens.
- Variadic macros expand with/without arguments.
- #if / #ifdef / defined() evaluate as expected.
- Include boundaries are captured and affect token stream.

## Test Cases (anchor set)
1) `03__object_like_macro`
   - Simple macro substitution in expressions.
2) `03__function_like_macro`
   - Function-like macro with arguments and spacing.
3) `03__stringify_and_paste`
   - # and ## produce expected tokens.
4) `03__variadic_macro`
   - __VA_ARGS__ with one and multiple args.
5) `03__defined_expr`
   - defined() in #if selects correct branch.
6) `03__include_basic`
   - Tokens from a local include appear in the stream.
7) `03__include_guard`
   - Multiple includes of guarded header expand once.
8) `03__pp_if_arith`
   - #if arithmetic and precedence rules.
9) `03__macro_redefinition`
   - Macro redefinition behavior (warn/override as implemented).
10) `03__nested_macro_expansion`
   - Nested macro rescans to final token.
11) `03__macro_arg_commas`
   - Commas inside macro arguments stay grouped.
12) `03__if_defined_paren`
   - defined() with/without parentheses in #if.
13) `03__stringize_spacing`
   - Stringizing keeps token text stable.
14) `03__token_paste_numbers`
   - Token pasting to form a numeric literal.
15) `03__variadic_empty`
   - Variadic macros with empty __VA_ARGS__.
16) `03__if_elif_chain`
   - #if/#elif selection.
17) `03__undef_defined`
   - #undef affects defined() result.
18) `03__macro_paste_expand`
   - Macro arguments expand before token paste.
19) `03__defined_macro_expansion`
   - defined() inside macro expansion evaluates correctly.
20) `03__line_directive_builtin`
   - #line remaps __LINE__ inside #if expressions.
21) `03__macro_to_nothing`
   - Object-like macro expands to no tokens.
22) `03__line_continuation`
   - Backslash-newline continues a macro definition.
23) `03__empty_fn_args`
   - Empty non-variadic function-like macro call.
24) `03__recursive_block`
   - Simple self-referential macro remains blocked from re-expansion.
25) `03__include_missing`
   - Missing include fails in preprocessing with a stable diagnostic.
26) `03__partial_token_reject`
   - Macro-generated partial token is rejected during preprocessing.
27) `03__ifdef_ifndef`
   - `#ifdef` / `#ifndef` choose the correct branches.
28) `03__pragma_once`
   - `#pragma once` prevents duplicate header expansion.
29) `03__skip_missing_include`
   - Inactive `#if 0` branch does not resolve a missing include.
30) `03__nested_conditionals`
   - Nested `#if` + `#ifdef` stacks choose the correct branch.
31) `03__else_without_if`
   - `#else` without a matching conditional fails closed.
32) `03__endif_without_if`
   - `#endif` without a matching conditional fails closed.
33) `03__unterminated_if_reject`
   - Unclosed conditional stack fails closed at EOF.
34) `03__elif_after_else`
   - `#elif` after `#else` fails closed.
35) `03__include_cycle`
   - Recursive includes are rejected with a preprocessing diagnostic.
36) `03__macro_arg_count_reject`
   - Too-few-argument macro calls fail closed in preprocessing.
37) `03__quote_include_prefers_local`
   - Quoted includes prefer the including file's directory.
38) `03__angle_include_uses_include_path`
   - Angle includes resolve through the configured include path.
39) `03__pp_if_short_circuit`
   - `#if` logical `||` / `&&` short-circuiting avoids dead-side faults.
40) `03__pp_if_numeric_edges`
   - Stable numeric-edge `#if` expressions (shift + unsigned compare).
41) `03__pp_if_large_values`
   - Larger unsigned `#if` expressions evaluate correctly.
42) `03__pp_if_mixed_signedness`
   - Mixed signed/unsigned `#if` comparisons follow the evaluator's conversion rules.
43) `03__pp_if_ternary_short_circuit`
   - Ternary `#if` branches only evaluate the selected side.
44) `03__pp_if_unsigned_shift_boundary`
   - Unsigned shift boundary comparisons stay stable at the high-bit edge.
45) `03__pp_if_missing_rhs_reject`
   - Missing right-hand operands in `#if` fail closed.
46) `03__pp_if_missing_colon_reject`
   - Malformed ternary `#if` expressions fail closed.
47) `03__pp_if_unmatched_paren_reject`
   - Unbalanced parentheses in `#if` fail closed.
48) `03__pp_if_div_zero_reject`
   - Division-by-zero in `#if` expressions fails closed.
49) `03__pp_elif_missing_rhs_reject`
   - Missing right-hand operands in `#elif` fail closed.
50) `03__pp_elif_div_zero_reject`
   - Division-by-zero in `#elif` expressions fails closed.
51) `03__include_next_basic`
   - `#include_next` skips the current header origin and resolves the next match.
52) `03__pp_elif_missing_colon_reject`
   - Malformed ternary `#elif` expressions fail closed.
53) `03__pp_elif_unmatched_paren_reject`
   - Unbalanced parentheses in `#elif` fail closed.
54) `03__pp_elif_skips_dead_expr`
   - A skipped `#elif` expression is not evaluated after an earlier branch wins.
55) `03__inactive_nested_if_skips_bad_expr`
   - Malformed inner `#if` expressions are ignored inside inactive outer branches.
56) `03__include_next_missing`
   - Missing `#include_next` targets fail closed with a preprocessing diagnostic.
57) `03__nested_duplicate_else_reject`
   - Duplicate nested `#else` directives fail closed.
58) `03__nested_elif_after_else_reject`
   - Nested `#elif` after nested `#else` fails closed.
59) `03__inactive_outer_skips_malformed_elif`
   - Malformed inner `#elif` expressions are ignored inside inactive outer branches.
60) `03__macro_mutual_recursive_block`
   - Mutually recursive macros are blocked from infinite expansion and leave a stable token stream.
61) `03__include_next_layered`
   - Layered `#include_next` resolution walks multiple explicit include paths in order.
62) `03__macro_arg_too_many_reject`
   - Too-many-argument macro calls fail closed in preprocessing.
63) `03__macro_arg_missing_comma_reject`
   - Missing-comma macro invocations fail closed in preprocessing.
64) `03__macro_redef_conflict_tokens`
   - Conflicting macro redefinition currently resolves to the latest definition in token output.
65) `03__macro_redef_identical_tokens`
   - Identical macro redefinition remains stable in token output.
66) `03__include_next_same_dir_parent`
   - A same-directory parent using `#include_next` resolves through include paths from the first `-I` entry.
67) `03__include_next_angle_layered`
   - Angle-form `#include_next` in include-path headers resolves from the next include path slot.
68) `03__nested_active_bad_elif_reject`
   - Malformed nested active `#elif` expressions fail closed.
69) `03__pp_if_bitwise_logic_mix`
   - Mixed bitwise/logical/shift preprocessor expressions evaluate deterministically.
70) `03__pp_if_nested_ternary_invalid_reject`
   - Malformed nested ternary expressions in `#if` fail closed.
71) `03__variadic_missing_named_arg_reject`
   - Variadic macros with required named parameters reject empty invocation (`V()`) in preprocessing.
72) `03__macro_expansion_limit_reject`
   - Macro expansion-depth limit failures are surfaced as preprocessing diagnostics.
73) `03__include_next_pragma_once_chain`
   - `#include_next` chains with `#pragma once` stay deterministic across repeated includes.
74) `03__vaopt_basic`
   - `__VA_OPT__` includes optional tokens only when variadic arguments are present.
75) `03__error_directive_reject`
   - `#error` directives fail closed in preprocessing with stable diagnostics.
76) `03__warning_directive_diag`
   - `#warning` directives emit diagnostics without forcing preprocess failure.
77) `03__warning_inactive_branch`
   - `#warning` directives in inactive branches are skipped.
78) `03__pp_if_shift_negative_boundary`
   - Negative shift-width `#if` expressions stay deterministic under current evaluator policy.
79) `03__macro_invocation_requires_adjacent_lparen`
   - Function-like macros require an adjacent `(` and do not expand across whitespace.
80) `03__macro_arg_nested_paren_grouping`
   - Nested parentheses and commas inside macro arguments stay grouped correctly.
81) `03__issue_unknown_directive_reject`
   - Unknown active preprocessor directives now fail closed in preprocessing.
82) `03__issue_endif_extra_tokens_reject`
   - `#endif` with trailing tokens fails closed.
83) `03__issue_ifdef_extra_tokens_reject`
   - `#ifdef` with trailing tokens fails closed.
84) `03__issue_ifndef_extra_tokens_reject`
   - `#ifndef` with trailing tokens fails closed.
85) `03__issue_else_extra_tokens_reject`
   - `#else` with trailing tokens fails closed.
86) `03__gnu_comma_vaargs`
   - GNU `, ##__VA_ARGS__` extension is explicitly unsupported with stable diagnostics.
87) `03__unknown_directive_inactive_branch`
   - Unknown directives in inactive branches are skipped and do not fail active flow.
88) `03__line_directive_missing_number_reject`
   - `#line` without a line-number operand fails closed.
89) `03__line_directive_non_number_reject`
   - `#line` with a non-numeric first operand fails closed.
90) `03__line_directive_extra_tokens_reject`
   - `#line` with trailing tokens after optional filename fails closed.
91) `03__nested_inner_ifdef_extra_tokens_reject`
   - Nested `#ifdef` directives with trailing tokens fail closed.
92) `03__nested_inner_else_extra_tokens_reject`
   - Nested `#else` directives with trailing tokens fail closed.
93) `03__nested_inactive_skips_unknown_directive`
   - Unknown directives in nested inactive branches are skipped.
94) `03__include_next_order_chain`
   - Multi-level `#include_next` search-order chaining across four include paths is stable.
95) `03__line_directive_macro_trailing_reject`
   - Macro-expanded trailing tokens in `#line` fail closed.
96) `03__nested_deep_inner_else_extra_tokens_reject`
   - Deeply nested `#else` with trailing tokens fails closed.
97) `03__nested_deep_active_bad_elif_reject`
   - Deep active nested malformed `#elif` expressions fail closed.
98) `03__nested_deep_inactive_skips_unknown_directive`
   - Deep inactive nested unknown directives are skipped.
99) `03__include_next_mixed_delims_order`
   - Mixed quote/angle `#include_next` chaining across four include paths is stable.
100) `03__line_directive_zero_reject`
   - `#line 0` now fails closed under strict positive-decimal policy.
101) `03__line_directive_hex_number_reject`
   - Non-decimal `#line` values (hex forms) fail closed.
102) `03__line_directive_suffix_number_reject`
   - Suffix-bearing `#line` numeric tokens fail closed.
103) `03__nested_depth5_duplicate_else_reject`
   - Duplicate `#else` in a deeper nested stack fails closed.
104) `03__nested_depth5_inactive_skips_bad_if_expr`
   - Malformed deep nested `#if` expressions in inactive branches are skipped.
105) `03__include_collision_quote_chain`
   - Quote-entry include collision chain resolves local → include-next path order deterministically.
106) `03__include_collision_angle_chain`
   - Angle-entry include collision chain resolves include-path order deterministically.
107) `03__line_directive_out_of_range_reject`
   - Out-of-range decimal `#line` values fail closed.
108) `03__line_directive_macro_out_of_range_reject`
   - Macro-expanded out-of-range `#line` values fail closed.
109) `03__line_directive_charconst_reject`
   - Character-constant `#line` operands fail closed.
110) `03__nested_depth6_duplicate_else_reject`
   - Duplicate `#else` in a depth-6 conditional stack fails closed.
111) `03__nested_depth6_inactive_skips_bad_elif_expr`
   - Malformed deep nested `#elif` expressions in inactive branches are skipped.
112) `03__include_collision_quote_chain_long`
   - Quote-entry include collision chain across local + 3 include-path hops is deterministic.
113) `03__include_collision_angle_chain_long`
   - Angle-entry include collision chain across 3 include-path hops is deterministic.
114) `03__issue_ifdef_missing_identifier_reject`
   - `#ifdef` without a same-line identifier now fails closed.
115) `03__issue_ifndef_missing_identifier_reject`
   - `#ifndef` without a same-line identifier now fails closed.
116) `03__issue_define_missing_identifier_reject`
   - `#define` without a same-line identifier now fails closed.
117) `03__issue_define_non_identifier_reject`
   - `#define` with non-identifier macro names now fails closed.
118) `03__issue_undef_extra_tokens_reject`
   - `#undef` with trailing tokens now fails closed.
119) `03__issue_undef_missing_identifier_reject`
   - `#undef` without a same-line identifier now fails closed.
120) `03__macro_expansion_limit_chain_ok`
   - A bounded macro expansion chain passes when the configured expansion limit is sufficient.
121) `03__macro_expansion_limit_chain6_ok`
   - A deeper bounded macro expansion chain passes at the explicit boundary limit.
122) `03__macro_expansion_limit_chain6_reject`
   - The same deeper chain fails closed when the expansion limit is one step too small.
123) `03__issue_define_param_missing_comma_reject`
   - Function-like `#define` with missing parameter comma fails closed.
124) `03__issue_define_param_trailing_comma_reject`
   - Function-like `#define` with trailing comma in parameter list fails closed.
125) `03__issue_define_param_unclosed_reject`
   - Function-like `#define` with unclosed parameter list fails closed.
126) `03__include_collision_mixed_local_angle_chain`
   - Local-entry include collision chain with alternating quote/angle `#include_next` hops is deterministic.
127) `03__macro_redef_obj_to_function_tokens`
   - Object-like macro redefinition to function-like form follows latest-definition expansion behavior.
128) `03__macro_redef_function_to_object_tokens`
   - Function-like macro redefinition to object-like form follows latest-definition expansion behavior.
129) `03__macro_redef_function_conflict_tokens`
   - Conflicting function-like macro redefinition follows latest-definition expansion behavior.
130) `03__nested_depth7_active_bad_elif_reject`
   - Malformed active `#elif` expression at deeper nesting fails closed.
131) `03__nested_depth7_inactive_skips_bad_if_expr`
   - Malformed nested `#if` expression in an inactive depth-7 branch is skipped.
132) `03__include_next_missing_mid_chain`
   - Missing `#include_next` target from a mid-chain include path fails closed.
133) `03__defined_keyword_name`
   - `defined(<keyword-token macro name>)` now works when that macro is defined.
134) `03__defined_keyword_name_no_paren`
   - `defined <keyword-token macro name>` now works in the no-parentheses form.
135) `03__defined_non_identifier_reject`
   - `defined(...)` with a non-identifier operand fails closed.
136) `03__issue_ifdef_non_identifier_reject`
   - `#ifdef` with non-identifier operand fails closed.
137) `03__issue_ifndef_non_identifier_reject`
   - `#ifndef` with non-identifier operand fails closed.
138) `03__issue_undef_non_identifier_reject`
   - `#undef` with non-identifier operand fails closed.
139) `03__nested_depth8_active_bad_elif_reject`
   - Malformed active `#elif` expression at depth-8 fails closed.
140) `03__nested_depth8_skips_bad_elif_taken_branch`
   - Malformed `#elif` is skipped at depth-8 when an earlier branch is already taken.
141) `03__include_extra_tokens_reject`
   - `#include` with trailing tokens after the header operand fails closed.
142) `03__include_next_extra_tokens_reject`
   - `#include_next` with trailing tokens after the header operand fails closed.
143) `03__include_macro_non_header_operand_reject`
   - Macro-expanded non-header `#include` operands fail closed.
144) `03__include_macro_trailing_tokens_reject`
   - Macro-expanded `#include` operands with trailing tokens fail closed.
145) `03__include_next_macro_trailing_tokens_reject`
   - Macro-expanded `#include_next` operands with trailing tokens fail closed.

## Known Gaps
- Recursive blocking is covered by `03__recursive_block` (self-recursive) and
  `03__macro_mutual_recursive_block` (mutual recursion anchor).
  Expansion-depth-limit diagnostics now have anchors via
  `03__macro_expansion_limit_reject` and
  `03__macro_expansion_limit_chain6_reject`, and explicit limit-boundary
  pass behavior is covered via `03__macro_expansion_limit_chain_ok` and
  `03__macro_expansion_limit_chain6_ok`; richer recursion diagnostics still
  need dedicated coverage.
- Include path coverage is improved, but broader path-resolution cases still
  need dedicated tests (`#include_next`, more search-path layering).
  Positive, missing-target, and explicit layered `#include_next` anchors now
  exist, and a missing-mid-chain anchor now exists, but more path-order
  variants still need work.
- Preprocessor expression coverage is much stronger now, including malformed
  `#if` / `#elif` rejection and dead-branch skipping, but it is still thinner
  than the lexer baseline.
- Directive syntax hardening for unknown directives, missing identifiers,
  malformed function-like parameter lists, and trailing tokens on `#define` /
  `#undef` / `#else` / `#endif` / `#ifdef` / `#ifndef` is now covered by
  explicit fail-closed tests.
- `#line` directive hardening now covers missing number, non-number first
  operand, direct trailing-token rejection, and macro-expanded trailing-token
  rejection, plus strict positive-decimal policy (`0`, hex, suffix, char-constant,
  and out-of-range forms rejected).
- GNU `, ##__VA_ARGS__` is now an explicit unsupported-policy path with a
  stable preprocessing diagnostic.
## Active Manifest
- Current preprocessor metadata lives in `tests/final/meta/03-preprocessor.json`.
- `tests/final/meta/index.json` is now only the manifest registry; it no longer
  stores the full test list inline.

## Current Baseline
- Current active preprocessor bucket cases: 145
- Current active preprocessor bucket result: passing
- This is now a meaningful first expansion pass, not just the initial anchor set.

## Next Pass Focus
- Add more include path and include-next layering coverage beyond the current
  explicit multi-`-I`, same-dir/angle, pragma-once-chain, 4-level order-chain,
  mixed-delimiter order-chain anchors, and include-collision quote/angle anchors
  (short and long chains).
- Add deeper directive syntax hardening around remaining uncommon malformed
  forms beyond current unknown/trailing and `#line` negatives.
- Add more malformed nested-conditional error-path cases in deeper stacks now
  that core malformed `#if` and `#elif` rejection is active.
- Continue collecting failures by behavior cluster, then fix the cluster and
  re-run the full preprocessor bucket.

## Expected Outputs
- Token stream expectations (`.tokens`) for all cases.
