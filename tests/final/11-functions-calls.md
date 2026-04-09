# Functions: Calls & Prototypes

## Scope
Function types, parameter adjustments, and call checking.

## Validate
- Prototype vs non-prototype calls (if supported).
- Variadic calls and default argument promotions.
- Return type matching and array/function decay.

## Acceptance Checklist
- Function prototypes enforce parameter types.
- Array/function parameters decay correctly.
- Variadic calls accept extra arguments and promote types.
- Return types are checked for compatibility.

## Test Cases (initial list)
1) `11__prototype_call`
   - Correct and incorrect prototype usage.
2) `11__no_prototype_call`
   - Old-style declaration call lowers without ABI mismatch.
3) `11__variadic_call`
   - Variadic function with mixed argument types.
4) `11__param_decay`
   - Array and function parameter adjustment.
5) `11__return_type_mismatch`
   - Return type incompatibility diagnostic.
6) `11__void_return_value`
   - Returning a value from void function.
7) `11__function_pointer_assign`
   - Incompatible function pointer assignment diagnostic.
8) `11__prototype_too_many_args`
   - Prototyped call rejects excess arguments.
9) `11__prototype_too_few_args`
   - Prototyped call rejects missing arguments.
10) `11__argument_type_mismatch`
   - Argument type mismatch at callsite is diagnosed.
11) `11__nonvoid_missing_return`
   - Non-void function missing guaranteed return is diagnosed.
12) `11__variadic_requires_named_param_reject`
   - Variadic declaration without named parameter is rejected.

## Expected Outputs
- AST/Diagnostics goldens for call checking.
- IR golden for `11__no_prototype_call`.

## Wave 2 Additions (Parser Diagnostic Export)
13) `11__parserdiag__param_missing_comma`
   - Parser diagnostic tuples for missing comma between parameters.
14) `11__parserdiag__variadic_ellipsis_not_last`
   - Parser diagnostic tuples for invalid variadic placement in parameter list.
15) `11__parserdiag__declarator_missing_rparen`
   - Parser diagnostic tuples for unterminated function declarator parameter list.
16) `11__parserdiag__fnptr_param_missing_rparen`
   - Parser diagnostic tuples for malformed function-pointer parameter declarator.

## Wave 3 Additions (Parser Diagnostic Expansion)
17) `11__parserdiag__prototype_missing_param_comma`
   - Parser diagnostic tuples for missing comma between prototype parameters.
18) `11__parserdiag__prototype_ellipsis_then_param_reject`
   - Parser diagnostic tuples for invalid parameter after variadic ellipsis.
19) `11__parserdiag__prototype_double_comma`
   - Parser diagnostic tuples for duplicate comma token in prototype parameters.
20) `11__parserdiag__prototype_ellipsis_missing_comma`
   - Parser diagnostic tuples for malformed variadic marker missing comma delimiter.
21) `11__parserdiag__prototype_missing_rparen_then_next_decl`
   - Parser diagnostic tuples for unterminated prototype with follow-up declaration recovery.

## Wave 4 Additions (Line-Directive Diagjson Thresholds)
22) `11__line_directive_nonvoid_missing_return_diagjson_current_sparse`
   - Remapped `#line` non-void-missing-return diagjson baseline.
23) `11__line_directive_include_nonvoid_missing_return_diagjson_current_sparse`
   - Include-header + `#line` remapped non-void-missing-return diagjson baseline.
24) `11__line_directive_prototype_too_many_args_diagjson_current_sparse`
   - Remapped `#line` prototype-too-many-args diagjson baseline.
25) `11__line_directive_include_prototype_too_many_args_diagjson_current_sparse`
   - Include-header + `#line` remapped prototype-too-many-args diagjson baseline.

## Wave 5 Additions (Line-Directive Text-Diagnostic Parity)
26) `11__line_directive_nonvoid_missing_return_diag_text_strict`
   - Text diagnostic parity for remapped `#line` non-void-missing-return.
27) `11__line_directive_include_nonvoid_missing_return_diag_text_strict`
   - Text diagnostic parity for include-header + `#line` non-void-missing-return.
28) `11__line_directive_prototype_too_many_args_diag_text_strict`
   - Text diagnostic parity for remapped `#line` prototype-too-many-args.
29) `11__line_directive_include_prototype_too_many_args_diag_text_strict`
   - Text diagnostic parity for include-header + `#line` prototype-too-many-args.

## Probe Backlog
- No open probes in this bucket after line-directive diagnostic provenance fixes.
