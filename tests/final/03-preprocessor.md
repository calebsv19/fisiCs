# Preprocessor

## Scope
Macro expansion rules, #if evaluation, and include handling.

## Validate
- Object/function-like macros, nested expansion, rescans.
- # and ## semantics.
- Variadic macros and __VA_ARGS__.
- defined() expression handling in #if.

## Acceptance Checklist
- Object-like and function-like macros expand correctly.
- Nested macros rescan and avoid re-expanding the active macro.
- Stringizing and token pasting produce valid tokens.
- Variadic macros expand with/without arguments.
- #if / #ifdef / defined() evaluate as expected.
- Include boundaries are captured and affect token stream.

## Test Cases (initial list)
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

## Known Gaps
- Macro recursion guard currently errors out in the CLI path. Track in `FAILING.md`
  and re-enable a `03__macro_recursion_guard` test once preprocessing can fail
  gracefully without aborting the entire compile.

## Expected Outputs
- Token stream expectations (`.tokens`) for all cases.
