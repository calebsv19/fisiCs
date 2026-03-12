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

## Wave 3 Additions
29) `05__unary__bitwise_not`
   - Bitwise unary `~` baseline on unsigned operands.
30) `05__unary__sizeof_ambiguity`
   - Distinguish `sizeof` expression-vs-type with typedef shadowing.
31) `05__binary__arithmetic`
   - Baseline arithmetic binary operators (`+`, `-`, `*`, `/`, `%`).
32) `05__binary__bitwise`
   - Baseline bitwise binary operators (`&`, `|`, `^`).
33) `05__binary__logical`
   - Baseline logical binary operators (`&&`, `||`) with unary `!`.
34) `05__binary__relational`
   - Baseline relational binary operators (`<`, `<=`, `>`, `>=`).
35) `05__binary__equality`
   - Baseline equality binary operators (`==`, `!=`).
36) `05__binary__invalid_shift_width`
   - Reject negative shift width (`x << -1`) with semantic diagnostic.
37) `05__ternary__nested`
   - Nested ternary parse and binding baseline.
38) `05__casts__explicit`
   - Valid scalar explicit cast matrix (`double->int->unsigned->long`).
39) `05__casts__ambiguity`
   - Cast-vs-parenthesized expression disambiguation with typedef shadowing.

## Wave 4 Additions (Probe Promotion)
40) `05__runtime__*` (13 tests)
   - Promoted runtime probe coverage into active suite with clang differential checks.
   - Includes nested ternary false-chain variants and VLA `sizeof` side-effect runtime path.
41) `05__diag__*` (24 tests)
   - Promoted diagnostic probe coverage into active suite with `.diag` expectations.
   - Includes strict `_Alignof(void)`, `_Alignof(expr)`, and `sizeof(void)` rejection checks.

## Wave 5 Additions (Parser Diagnostic Export)
42) `05__parserdiag__ternary_missing_colon`
   - Parser diagnostic tuples for malformed conditional operator (`?:`) syntax.
43) `05__parserdiag__call_missing_rparen`
   - Parser diagnostic tuples for unterminated call-expression argument list.
44) `05__parserdiag__subscript_missing_rbracket`
   - Parser diagnostic tuples for unterminated subscript expression.
45) `05__parserdiag__cast_missing_rparen`
   - Parser diagnostic tuples for malformed cast-expression parenthesis closure.

## Wave 6 Additions (Parser Diagnostic Expansion)
46) `05__parserdiag__member_missing_identifier`
   - Parser diagnostic tuples for member access missing field identifier.
47) `05__parserdiag__arrow_missing_identifier`
   - Parser diagnostic tuples for arrow member access missing field identifier.
48) `05__parserdiag__call_missing_first_arg`
   - Parser diagnostic tuples for call-expression missing first argument token.
49) `05__parserdiag__subscript_missing_index`
   - Parser diagnostic tuples for subscript expression missing index expression.
50) `05__parserdiag__ternary_missing_false_expr`
   - Parser diagnostic tuples for conditional operator missing false arm.
51) `05__parserdiag__ternary_gnu_omitted_middle_reject`
   - Parser diagnostic tuples for unsupported GNU omitted-middle ternary form.
52) `05__parserdiag__nested_group_unclosed`
   - Parser diagnostic tuples for nested grouping with unclosed parenthesis.

## Probe Status
- `tests/final/probes/run_probes.py` summary now reports:
  - blocked: `0`
  - resolved: `77`
  - skipped: `0`
- Previously blocked expression probes are now resolved and represented in active suite via `05-expressions-wave4.json`.

## Bucket Status
- `05` is marked stable for current parser/expression scope and is now part of the active final suite baseline.

## Post-Stable Hardening Checklist (Future 05 Sweep)
- Conditional operator type merge edges:
  - pointer-vs-null literal arms (`p ? ptr : 0` / `p ? 0 : ptr`)
  - qualified pointer merge (`const`/`volatile`) behavior
  - `void *` compatibility cases
- Cast diagnostics depth:
  - function-pointer casts and incompatible-call paths
  - qualifier-dropping conversions (e.g. `const T*` to `T*`)
- `sizeof`/`_Alignof` depth:
  - nested VLA scopes and typedef’d VLA operands
  - incomplete-type edge variations and typedef indirection
- Sequencing and side-effect stress:
  - deeper comma/ternary nesting with side effects
  - volatile reads/writes in short-circuit paths
- Stress/recovery:
  - long expression chains near parser recursion limits
  - malformed expression recovery variants under bucket 12 diagnostics checks
- Differential hygiene:
  - explicitly tag UB/implementation-defined expression cases in runtime tests to avoid noisy differential assertions.
