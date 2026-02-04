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

## Expected Outputs
- AST/Diagnostics goldens for expression shape checks.
