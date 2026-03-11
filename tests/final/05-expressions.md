# Parser: Expressions

## Scope
Expression grammar, precedence, associativity, and ambiguous parses.

## Validate
- Unary/binary precedence ladder and ?:, , operators.
- Cast vs parenthesized expression ambiguity.
- Member access, calls, and subscripts chaining.

## Acceptance Checklist
- Unary/binary precedence and associativity match C99.
- Cast vs grouped expression is parsed correctly.
- Conditional operator nests correctly and binds lower than logical ops.
- Comma operator is lowest precedence.
- Member access, calls, and subscripts chain left-to-right.

## Test Cases (initial list)
1) `05__precedence_basic`
   - Mix arithmetic, shifts, comparisons, logical ops.
2) `05__ternary_assoc`
   - Nested conditional operators.
3) `05__comma_operator`
   - Comma binds lower than assignment.
4) `05__cast_vs_group`
   - `(T)x` vs `(x)` disambiguation.
5) `05__member_call_chain`
   - `a.b(c)[d].e` chaining.
6) `05__logical_precedence`
   - && binds tighter than ||.
7) `05__cast_function_style`
   - Function-style cast vs typedef name call.
8) `05__sizeof_ambiguity`
   - sizeof with/without parentheses.
9) `05__postfix_chain`
   - Postfix chaining with member and subscripts.
10) `05__unary_precedence`
   - Unary operator precedence with bitwise not.
11) `05__sizeof_typedef`
   - sizeof with typedef names.
12) `05__cast_deref_precedence`
   - Cast binds before unary deref.
13) `05__ternary_comma_true`
   - Comma expressions inside the true arm of `?:`.
14) `05__sizeof_array_type`
   - `sizeof(int[2])` and compound-literal array size.
15) `05__unary_alignof`
   - `_Alignof` on primitive types.
16) `05__compound_literal_struct`
   - Struct compound literal in block scope.
17) `05__compound_literal_array`
   - Array compound literal with pointer decay.
18) `05__ternary_mixed_types`
   - Ternary with integer and floating operands.
19) `05__binary_shift_matrix`
   - Signed right shift and unsigned left shift composition.
20) `05__generic_unsupported_reject`
   - `_Generic` currently treated as explicit unsupported policy (fail-closed).

## Expected Outputs
- AST/Diagnostics goldens for expression shape checks.

## Wave 2 Additions
21) `05__precedence__all_tiers`
   - Cross-tier precedence chain with arithmetic, shifts, comparisons, and logical ops.
22) `05__precedence__nested`
   - Nested grouped expressions across binary tiers.
23) `05__unary__basic`
   - Unary `+`, unary `-`, and logical `!` baseline.
24) `05__unary__address_of`
   - Address-of baseline with pointer initialization/use.
25) `05__unary__deref`
   - Dereference baseline with pointer read.
26) `05__casts__illegal`
   - Reject non-scalar cast (`struct` to `int`).
27) `05__compound_literal__static_storage`
   - File-scope compound literal initializer baseline.
28) `05__ternary__lvalue`
   - Reject assignment to ternary expression result.

## Probe Backlog
- None open in this bucket. `05__probe_typedef_shadow_parenthesized_expr.c` now resolves and matches clang runtime behavior.
