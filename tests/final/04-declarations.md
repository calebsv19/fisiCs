# Parser: Declarations

## Scope
Declarators, type qualifiers, aggregates, and C99 declaration features.

## Validate
- Pointers/arrays/functions with nested declarators.
- struct/union/enum definitions and forward decls.
- Designated initializers and compound literals.
- Bit-fields, flexible arrays, VLAs, inline (if supported).
- Array parameter qualifiers (`static`/`const`/`restrict`/`volatile`) and minimum-size warnings.

## Test Ideas
## Acceptance Checklist
- Nested declarators print a stable canonical type string.
- Storage class and qualifiers are preserved and ordered.
- struct/union/enum declarations handle forward refs and definitions.
- Designated initializers parse with correct target fields.
- Bit-fields and flexible arrays parse without crashing.

## Test Cases (initial list)
1) `04__nested_declarators`
   - Pointers, arrays, functions combined.
2) `04__storage_and_qualifiers`
   - static/extern/typedef/const/volatile/restrict combos.
3) `04__storage__static`
   - Dedicated `static` storage-class coverage at file and block scope.
4) `04__storage__extern`
   - Dedicated `extern` declaration + definition linkage form.
5) `04__storage__auto`
   - Dedicated block-scope `auto` declaration form.
6) `04__storage__register`
   - Dedicated block-scope `register` declaration form.
7) `04__storage__file_scope_auto_reject`
   - Reject file-scope `auto` declarations.
8) `04__storage__file_scope_register_reject`
   - Reject file-scope `register` declarations.
9) `04__storage__conflict_static_auto_reject`
   - Reject conflicting `static auto` storage specifiers.
10) `04__storage__conflict_extern_register_reject`
   - Reject conflicting `extern register` storage specifiers.
11) `04__storage__typedef_extern_reject`
   - Reject `typedef` combined with `extern`.
12) `04__storage__missing_declarator`
   - Fail-closed parse diagnostic when storage specifier has no declarator.
13) `04__qualifiers__const`
   - Dedicated `const` declarations + semantic negative on mutation.
14) `04__qualifiers__const_pointer_assign`
   - Semantic rejection for assignment to `const` pointer object.
15) `04__qualifiers__volatile`
   - Dedicated `volatile` declarations and read/write flow.
16) `04__qualifiers__missing_type`
   - Fail-closed parse diagnostic when qualifier appears without a type.
17) `04__qualifiers__restrict`
   - Dedicated `restrict` pointer qualifier declarations.
18) `04__qualifiers__restrict_non_pointer_reject`
   - Reject `restrict` on non-pointer object declarations.
19) `04__primitive__integers`
   - Baseline coverage for short/int/long/long long declaration forms.
20) `04__primitive__floating`
   - Baseline coverage for float/double/long double declaration forms.
21) `04__primitive__bool`
   - Baseline coverage for `_Bool` declarations and conversions.
22) `04__primitive__signed_unsigned_conflict_reject`
   - Reject conflicting `signed` + `unsigned` specifiers.
23) `04__primitive__short_long_conflict_reject`
   - Reject conflicting `short` + `long` specifiers.
24) `04__primitive__unsigned_float_reject`
   - Reject integer sign/width modifiers on `float`.
25) `04__primitive__signed_bool_reject`
   - Reject integer sign modifiers on `_Bool`.
26) `04__primitive__unsigned_void_reject`
   - Reject integer sign/width modifiers on `void`.
27) `04__primitive__char_signedness`
   - Baseline coverage for `char`/`signed char`/`unsigned char` forms.
28) `04__primitive__long_char_reject`
   - Reject invalid `long char` specifier combination.
29) `04__enum__explicit_values`
   - Verify enums with explicit enumerator assignments.
30) `04__enum__implicit_increment`
   - Verify implicit auto-increment around explicit enumerator anchors.
31) `04__enum__negative_values`
   - Verify negative enumerator constant expressions.
32) `04__enum__large_values`
   - Verify large integral enumerator constants parse and bind.
33) `04__enum__duplicate`
   - Reject duplicate enumerator identifiers in a single enum.
34) `04__struct__basic`
   - Baseline named struct definition with multiple field types.
35) `04__struct__nested`
   - Nested struct definition with embedded named struct member.
36) `04__struct__anonymous`
   - Anonymous struct member coverage inside an outer struct.
37) `04__struct__self_ref`
   - Self-referential struct via pointer-to-self field.
38) `04__struct__duplicate_member_reject`
   - Reject duplicate member names in a struct definition.
39) `04__union__basic`
   - Baseline named union definition with overlapping member types.
40) `04__union__nested`
   - Nested union definition with embedded named union member.
41) `04__union__anonymous`
   - Anonymous union member coverage inside an outer union.
42) `04__union__duplicate_member_reject`
   - Reject duplicate member names in a union definition.
43) `04__struct__flex_not_last_reject`
   - Reject struct flexible array members that are not the final field.
44) `04__union__flex_array_reject`
   - Reject flexible array members declared in unions.
45) `04__struct__vla_field_reject`
   - Reject variable-length array fields inside struct/union declarations.
46) `04__struct_union_enum`
   - forward declaration + definition + enum values.
47) `04__designated_initializers`
   - .field and [index] designators.
48) `04__bitfields_flex`
   - bit-fields and flexible array member.
49) `04__array_param_qualifiers`
   - array parameter qualifiers and `static` size warnings.
50) `04__static_assert_pass`
   - Valid `_Static_assert` declaration form.
51) `04__static_assert_fail`
   - Failing constant-expression `_Static_assert`.
52) `04__static_assert_nonconst`
   - Non-constant `_Static_assert` rejection.
53) `04__thread_local_unsupported`
   - Explicit unsupported diagnostic for `_Thread_local`.
54) `04__atomic_unsupported`
   - Explicit unsupported diagnostic for `_Atomic`.
55) `04__typedef__redefinition_reject`
   - Reject conflicting typedef redefinitions in the same scope.
56) `04__typedef__shadowing_block`
   - Allow typedef shadowing in nested block scope.
57) `04__enum__non_integer_value_reject`
   - Reject non-integer enumerator values as invalid constant expressions.
58) `04__bitfield__negative_width_reject`
   - Reject negative bitfield widths.
59) `04__bitfield__width_exceeds_type_reject`
   - Reject bitfield widths that exceed the declared base type width.
60) `04__bitfield__non_integral_type_reject`
   - Reject bitfields declared with non-integral base types.
61) `04__enum__out_of_range_reject`
   - Reject enum values that use overflowed literals or exceed `int` range.
62) `04__declarator__ptr_to_fn`
   - Pointer-to-function declarator baseline.
63) `04__declarator__fn_returns_ptr`
   - Function-returning-pointer declarator baseline.
64) `04__declarator__ptr_to_array`
   - Pointer-to-array declarator baseline.
65) `04__declarator__array_of_ptrs`
   - Array-of-pointers declarator baseline.
66) `04__declarator__fn_ptr_param`
   - Function pointer parameter declarator baseline.
67) `04__declarator__abstract_fnptr_param`
   - Abstract function-pointer parameter declarator form.
68) `04__declarator__abstract_ptr_to_array_param`
   - Abstract pointer-to-array parameter declarator form.
69) `04__declarator__multi_array`
   - Multidimensional array declarator baseline.
70) `04__declarator__fn_returns_fn_reject`
   - Reject invalid declarator where function returns function.
71) `04__declarator__array_of_fn_reject`
   - Reject invalid declarator where array element type is function.
72) `04__declarator__fn_returns_array_reject`
   - Reject invalid declarator where function returns array.
73) `04__enum__implicit_from_min`
   - Implicit increment baseline from minimum in-range enumerator value.
74) `04__enum__implicit_overflow_reject`
   - Reject implicit enumerator increment that overflows `int` range.
75) `04__enum__overflow_expr_literal_reject`
   - Reject enum expressions containing overflowed integer literals.
76) `04__bitfield__zero_width_unnamed`
   - Zero-width unnamed bitfield baseline.
77) `04__bitfield__zero_width_named_reject`
   - Reject named zero-width bitfields.
78) `04__bitfield__typedef_integral`
   - Bitfield baseline with typedef-resolved integral base type.
79) `04__bitfield__enum_type`
   - Bitfield baseline with enum base type.
80) `04__bitfield__unnamed`
   - Unnamed non-zero-width bitfield baseline.

## Expected Outputs
- AST/Diagnostics goldens for parsing and declaration structure.

## Wave 2 Additions
81) `04__primitive__complex`
    - `_Complex` primitive declaration baseline.
82) `04__typedef__complex_type`
    - Typedef of complex type used in function return/local declarations.
83) `04__alignas_basic`
    - `_Alignas` declaration baseline.
84) `04__alignas_invalid_reject`
    - Reject malformed `_Alignas` argument syntax.
85) `04__vla_block_scope`
    - Block-scope VLA declaration baseline.
86) `04__vla_file_scope_reject`
    - Reject file-scope VLA object declarations.
87) `04__incomplete_type_object_reject`
    - Reject object declaration with incomplete struct type.
88) `04__incomplete_type_pointer_ok`
    - Allow pointer declaration to incomplete struct type.
89) `04__declarator__ptr_to_fn_returning_ptr_to_array`
    - Deep declarator shape: pointer to function returning pointer to array.
    - Runtime/codegen follow-up promoted and covered by
      `15__torture__path_decl_nested` runtime differential test.
90) `04__declarator__array_param_vla`
    - Function parameter VLA declarator baseline.

## Wave 3 Additions
91) `04__enum__trailing_comma`
    - Enum declaration accepts trailing comma in enumerator list.

## Wave 4 Additions
92) `04__enum__empty_reject`
    - Reject empty enum bodies with no enumerators.

## Wave 5 Additions
93) `04__struct__tag_redefinition_reject`
    - Reject redefinition of a struct tag in the same scope.

## Wave 6 Additions (Parser Diagnostic Export)
94) `04__parserdiag__storage_missing_declarator`
    - Parser diagnostic tuples for missing declarator after storage-class specifier.
95) `04__parserdiag__qualifier_missing_type`
    - Parser diagnostic tuples for qualifier-without-type declaration form.
96) `04__parserdiag__declarator_fn_returns_fn_reject`
    - Parser diagnostic tuples for invalid function-returning-function declarator.
97) `04__parserdiag__declarator_array_of_fn_reject`
    - Parser diagnostic tuples for invalid array-of-function declarator.
98) `04__parserdiag__declarator_fn_returns_array_reject`
    - Parser diagnostic tuples for invalid function-returning-array declarator.
99) `04__parserdiag__alignas_invalid_reject`
    - Parser diagnostic tuples for malformed `_Alignas` declaration syntax.
100) `04__parserdiag__enum_empty_reject`
    - Parser diagnostic tuples for empty enum-body reject path.

## Wave 7 Additions (Parser Diagnostic Expansion)
101) `04__parserdiag__struct_member_missing_semicolon`
    - Parser diagnostic tuples for missing semicolon between struct members.
102) `04__parserdiag__bitfield_missing_width_expr`
    - Parser diagnostic tuples for bitfield width expression omission.
103) `04__parserdiag__enum_missing_comma_between_enumerators`
    - Parser diagnostic tuples for missing comma between enum constants.
104) `04__parserdiag__declarator_nested_fnptr_missing_rparen`
    - Parser diagnostic tuples for malformed nested function-pointer declarator.
105) `04__parserdiag__array_param_static_missing_size`
    - Parser diagnostic tuples for malformed array-parameter `static` form.

## Parserdiag Lane Additions (Current 04 Pass)
106) `04__parserdiag__struct_member_missing_type_reject`
    - Parser diagnostic tuples for struct member declaration missing a type specifier.
107) `04__parserdiag__bitfield_missing_colon_reject`
    - Parser diagnostic tuples for bitfield declaration missing `:` before width.
108) `04__parserdiag__enum_missing_rbrace_reject`
    - Parser diagnostic tuples for enum body missing closing `}`.
109) `04__parserdiag__typedef_missing_identifier_reject`
    - Parser diagnostic tuples for typedef declaration missing declarator identifier.
110) `04__parserdiag__declarator_unbalanced_parens_reject`
    - Parser diagnostic tuples for unbalanced declarator parentheses.

## Runtime + Complex Legality Additions
111) `04__union__overlap`
    - Runtime/differential anchor that verifies union member base-address overlap and union size floor checks.
112) `04__primitive__complex_unsigned_reject`
    - Reject `unsigned _Complex double` declaration form.
113) `04__primitive__complex_missing_base_reject`
    - Reject `_Complex` declaration without a valid floating base type.
114) `04__primitive__complex_int_reject`
    - Reject `_Complex int` because complex types require a floating base.

## Probe Backlog
- Current 04 probe sweep (`PROBE_FILTER=04__probe_`) result:
  - blocked: `0`
  - resolved: `20`
  - skipped: `0`
- Newly added and resolved 04 parser-shape probes:
  - `04__probe_struct_member_missing_type_reject`
  - `04__probe_bitfield_missing_colon_reject`
  - `04__probe_enum_missing_rbrace_reject`
  - `04__probe_typedef_missing_identifier_reject`
  - `04__probe_declarator_unbalanced_parens_reject`
- Newly added and resolved 04 runtime/complex probes:
  - `04__probe_union_overlap_runtime`
  - `04__probe_complex_int_reject`
  - `04__probe_complex_unsigned_reject`
  - `04__probe_complex_missing_base_reject`
