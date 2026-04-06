# Type System & Conversions

## Status Snapshot (2026-04-05)

- Active tests: `102`
  (`tests/final/meta/07-types-conversions.json` +
  `tests/final/meta/07-types-conversions-wave2-semantic.json` +
  `tests/final/meta/07-types-conversions-wave3-conv-diagjson.json` +
  `tests/final/meta/07-types-conversions-wave4-conv-matrix.json` +
  `tests/final/meta/07-types-conversions-wave5-blocker-fixes.json` +
  `tests/final/meta/07-types-conversions-wave6-constexpr-positive.json` +
  `tests/final/meta/07-types-conversions-wave7-aggregate-union-depth.json` +
  `tests/final/meta/07-types-conversions-wave8-cast-constexpr-edge.json` +
  `tests/final/meta/07-types-conversions-wave9-policy-aggregate-edge.json` +
  `tests/final/meta/07-types-conversions-wave10-policy-designator-constexpr-depth.json` +
  `tests/final/meta/07-types-conversions-wave11-constexpr-diagjson-depth.json` +
  `tests/final/meta/07-types-conversions-wave12-diff-diagjson-constexpr-frontier.json` +
  `tests/final/meta/07-types-conversions-wave13-strict-memberdiag-constexpr-depth.json` +
  `tests/final/meta/07-types-conversions-wave14-strict-conv-diagjson-constexpr-frontier.json` +
  `tests/final/meta/07-types-conversions-wave15-strict-pointer-diff-depth.json` +
  `tests/final/meta/07-types-conversions-wave16-strict-diagjson-cast-aggregate-depth.json` +
  `tests/final/meta/07-types-conversions-wave17-constexpr-coupled-ice-depth.json` +
  `tests/final/meta/07-types-conversions-wave18-aggregate-runtime-promotions.json`)
- Current bucket run:
  - `make final-prefix PREFIX=07__` -> all pass
  - `PROBE_FILTER=07__probe_*` -> `resolved=91`, `blocked=0`, `skipped=0`

## Scope
Integer promotions, usual arithmetic conversions, and pointer rules.

## Validate
- Promotions for integer types and _Bool.
- Usual arithmetic conversions for mixed signedness.
- Pointer arithmetic scaling and comparisons.

## Acceptance Checklist
- Integer promotions and usual arithmetic conversions are applied.
- Mixed signed/unsigned comparisons follow C99 rules.
- Pointer arithmetic scales by element size.
- Pointer comparisons and null checks are accepted with correct types.

## Test Cases (initial list)
1) `07__integer_promotions`
   - _Bool/char/short promoted in expressions.
2) `07__usual_arith_conversions`
   - signed/unsigned mix with long/ulong.
3) `07__pointer_arithmetic`
   - ptr +/- int and ptr - ptr.
4) `07__pointer_comparisons`
   - null pointer checks and relational comparisons.
5) `07__mixed_signed_unsigned_compare`
   - signed/unsigned comparison rules.
6) `07__array_decay_param`
   - Array parameter usage validates array-to-pointer decay.
7) `07__function_decay_call`
   - Function designator used via function pointer parameter.
8) `07__void_pointer_roundtrip`
   - Object pointer to `void*` and back.
9) `07__void_pointer_arith_reject`
   - `void*` arithmetic rejected as invalid.

## Expected Outputs
- AST/Diagnostics goldens for type conversion behavior.

## Probe Backlog
- No open probes in this bucket at the current baseline.

## Wave 2 Additions (Semantic Expansion)
1) `07__assign_struct_to_int_reject`
   - Reject assigning struct value to scalar int object.
2) `07__agg__member_access`
   - Nested member access using both `.` and `->`.
3) `07__agg__offsets`
   - Aggregate member-offset ordering via `offsetof`.
4) `07__agg__invalid_member_reject`
   - Reject unknown struct member access.
5) `07__constexpr__array_size_reject`
   - Reject non-constant file-scope array size.
6) `07__constexpr__case_label_reject`
   - Reject non-constant `case` label expression.
7) `07__constexpr__static_init_reject`
   - Reject non-constant static-storage initializer.

## Wave 3 Additions (Conversion + DiagJSON)
1) `07__conv__pointer_assign_float_reject`
   - Reject pointer assignment from floating expression.
2) `07__conv__pointer_init_struct_reject`
   - Reject pointer initializer from struct expression.
3) `07__signed_unsigned_compare_boundary_runtime`
   - Runtime matrix for int/unsigned boundary behavior.
4) `07__agg_nested_offsets_runtime`
   - Runtime nested aggregate offset/stride checks.
5) `07__diagjson__assign_struct_to_int_reject`
6) `07__diagjson__constexpr_array_size_reject`
7) `07__diagjson__constexpr_case_label_reject`
8) `07__diagjson__constexpr_static_init_reject`
9) `07__diagjson__conv_pointer_assign_float_reject`
10) `07__diagjson__conv_pointer_init_struct_reject`

## Wave 4 Additions (Conversion Matrix Expansion)
1) `07__signed_unsigned_compare_long_boundary_runtime`
   - Runtime matrix for long/unsigned long and long long/unsigned long long.
2) `07__conv__pointer_init_nonzero_int_reject`
   - Reject pointer initialization from nonzero integer constant.
3) `07__conv__assign_pointer_to_int_reject`
   - Reject pointer-to-integer assignment without explicit cast.
4) `07__diagjson__conv_pointer_init_nonzero_int_reject`
5) `07__diagjson__conv_assign_pointer_to_int_reject`

## Wave 5 Additions (Blocker Resolution Promotion)
1) `07__null_pointer_constant_runtime`
   - Validate ternary null-pointer constant compatibility at runtime.
2) `07__diagjson__agg_invalid_member_reject`
   - Confirm diagnostics JSON parity for invalid aggregate member access.

## Wave 6 Additions (Constexpr Positive Matrix)
1) `07__constexpr_array_size_positive_runtime`
   - Validate integer constant expressions in file-scope array bounds.
2) `07__constexpr_case_label_positive_runtime`
   - Validate integer constant expressions in `switch` case labels.
3) `07__constexpr_static_init_positive_runtime`
   - Validate integer constant expressions in static-storage initializers.

## Wave 7 Additions (Aggregate/Union Depth)
1) `07__agg_union_nested_member_runtime`
   - Validate deeper nested member-path evaluation through struct-in-union shapes.
2) `07__agg_union_copy_update_runtime`
   - Validate by-value aggregate copy/update through union member paths.
3) `07__agg_union_missing_nested_field_reject`
   - Reject unknown nested field access through union variant.
4) `07__agg_union_arrow_nonptr_reject`
   - Reject `->` on non-pointer aggregate/union value.
5) `07__diagjson__agg_union_missing_nested_field_reject`
   - Confirm diagnostics JSON parity for nested aggregate-member rejection.

## Wave 8 Additions (Cast Policy + Constexpr Edge)
1) `07__cast_ptr_uintptr_roundtrip_runtime`
   - Validate explicit pointer<->`uintptr_t` cast roundtrip behavior.
2) `07__constexpr_enum_cast_sizeof_runtime`
   - Validate nested enum/cast/`sizeof` constant-expression forms.
3) `07__constexpr_large_bound_runtime`
   - Validate large stress-sized constant-expression array bounds.
4) `07__cast_struct_to_int_reject`
   - Reject explicit struct-to-integer cast.
5) `07__cast_int_to_struct_reject`
   - Reject explicit integer-to-struct cast.
6) `07__diagjson__cast_struct_to_int_reject`
7) `07__diagjson__cast_int_to_struct_reject`

## Wave 9 Additions (Policy + Aggregate Edge Negatives)
1) `07__policy_impldef_ptr_ulong_roundtrip_runtime`
   - Implementation-defined pointer<->`unsigned long` cast roundtrip lane.
2) `07__policy_impldef_int_to_ptr_nonzero_runtime`
   - Implementation-defined nonzero integer-to-pointer cast lane (no dereference).
3) `07__constexpr_fold_ternary_bitwise_runtime`
   - Compound ternary/bitwise constant-folding runtime lane.
4) `07__agg_dot_scalar_base_reject`
   - Reject member access via `.` on scalar base expression.
5) `07__agg_arrow_ptr_to_scalar_reject`
   - Reject member access via `->` on pointer-to-scalar expression.
6) `07__agg_dot_array_base_reject`
   - Reject member access via `.` on array expression.
7) `07__diagjson__agg_dot_scalar_base_reject`
8) `07__diagjson__agg_arrow_ptr_to_scalar_reject`

## Wave 10 Additions (Policy Depth + Designator Edge + Constexpr Stress)
1) `07__policy_impldef_ptr_slong_roundtrip_runtime`
   - Implementation-defined pointer<->`signed long` cast roundtrip lane.
2) `07__policy_explicit_nonzero_int_cast_ptr_runtime`
   - Explicit nonzero integer-to-pointer cast contrast lane (no dereference).
3) `07__constexpr_multidim_fold_runtime`
   - Multi-dimensional constant-expression and designated-array fold runtime lane.
4) `07__designator_array_index_nonconst_reject`
   - Reject non-constant array designator index.
5) `07__designator_array_index_negative_reject`
   - Reject negative array designator index.
6) `07__designator_array_index_oob_reject`
   - Reject out-of-bounds array designator index.
7) `07__designator_nested_unknown_field_reject`
   - Reject unknown nested field in designated aggregate initializer.
8) `07__diagjson__designator_array_index_nonconst_reject`
9) `07__diagjson__designator_array_index_oob_reject`

## Wave 11 Additions (Constexpr Frontier + Designator DiagJSON Parity)
1) `07__constexpr_array_chain_depth_runtime`
   - Deeper chained integer-constant-expression array-bound runtime lane.
2) `07__constexpr_case_chain_depth_runtime`
   - Deeper chained integer-constant-expression case-label runtime lane.
3) `07__diagjson__designator_nested_unknown_field_reject`
   - Diagnostics JSON parity for nested unknown-field designated initializer rejection.

## Wave 12 Additions (Tri-Reference Strict + DiagJSON Strict + Constexpr Frontier II)
1) `07__policy_strict_zero_cast_matrix_runtime`
   - Strict policy-safe null-pointer and `void*` roundtrip conversion matrix lane.
2) `07__constexpr_static_chain_depth_runtime`
   - Deeper chained integer-constant-expression static-initializer runtime lane.
3) `07__constexpr_switch_cast_sizeof_depth_runtime`
   - Mixed cast/`sizeof` switch-selector constant-expression runtime lane.
4) `07__diagjson__designator_array_index_nonconst_reject_strict`
   - Strict code+line `.diagjson` parity for non-constant designator index rejection.
5) `07__diagjson__designator_array_index_oob_reject_strict`
   - Strict code+line `.diagjson` parity for out-of-bounds designator index rejection.
6) `07__diagjson__designator_nested_unknown_field_reject_strict`
   - Strict code+line `.diagjson` parity for nested unknown-field designator rejection.

## Wave 13 Additions (Tri-Reference Pointer Boundary + Member-Operator DiagJSON Strict + Constexpr Frontier III)
1) `07__policy_strict_fnptr_decay_null_runtime`
   - Strict function-pointer decay/null-check/call conversion matrix lane.
2) `07__policy_strict_array_decay_ptrdiff_runtime`
   - Strict array-decay + pointer-difference + `void*` roundtrip conversion matrix lane.
3) `07__constexpr_static_ternary_bitcast_depth_runtime`
   - Deeper static-initializer ICE lane combining ternary/bitwise/casts.
4) `07__diagjson__agg_dot_scalar_base_reject_strict`
   - Strict code+line `.diagjson` parity for `.` on scalar-base member-access rejection.
5) `07__diagjson__agg_arrow_ptr_to_scalar_reject_strict`
   - Strict code+line `.diagjson` parity for `->` on pointer-to-scalar rejection.
6) `07__diagjson__agg_dot_array_base_reject_strict`
   - Strict code+line `.diagjson` parity for `.` on array-base rejection.

## Wave 14 Additions (Strict Pointer Matrix + Strict Conversion DiagJSON + Constexpr Frontier IV)
1) `07__policy_strict_const_ptr_roundtrip_runtime`
   - Strict const-qualified pointer-to-void roundtrip conversion lane.
2) `07__policy_strict_null_conditional_matrix_runtime`
   - Strict null-pointer conditional conversion matrix lane.
3) `07__policy_strict_fnptr_table_null_dispatch_runtime`
   - Strict function-pointer table null-dispatch conversion lane.
4) `07__constexpr_array_static_shared_chain_depth_runtime`
   - Deeper ICEs shared across array-bound and static-initializer surfaces.
5) `07__constexpr_switch_nested_ternary_bitwise_depth_runtime`
   - Deeper switch-label ICE lane with nested ternary+bitwise forms.
6) `07__constexpr_static_cast_sizeof_ternary_depth_runtime`
   - Deeper static-initializer ICE lane with cast+`sizeof`+ternary forms.
7) `07__diagjson__conv_pointer_assign_float_reject_strict`
   - Strict code+line `.diagjson` parity for pointer-assignment-from-float rejection.
8) `07__diagjson__conv_pointer_init_struct_reject_strict`
   - Strict code+line `.diagjson` parity for pointer-initializer-from-struct rejection.
9) `07__diagjson__conv_pointer_init_nonzero_int_reject_strict`
   - Strict code+line `.diagjson` parity for pointer-initializer-from-nonzero-int rejection.
10) `07__diagjson__conv_assign_pointer_to_int_reject_strict`
    - Strict code+line `.diagjson` parity for pointer-to-int assignment rejection.

## Wave 15 Additions (Strict Pointer Differential Depth)
1) `07__policy_strict_const_compare_matrix_runtime`
   - Strict const-qualified pointer comparison matrix over a shared array domain.
2) `07__policy_strict_const_chain_assignment_runtime`
   - Strict const-qualified pointer assignment and `void*` roundtrip chain.
3) `07__policy_strict_ptr_compare_one_past_runtime`
   - Strict one-past pointer comparison and pointer-difference matrix lane.

## Wave 16 Additions (Strict DiagJSON Cast + Aggregate Depth)
1) `07__diagjson__cast_struct_to_int_reject_strict`
   - Strict code+line `.diagjson` parity for explicit struct-to-int cast rejection.
2) `07__diagjson__cast_int_to_struct_reject_strict`
   - Strict code+line `.diagjson` parity for explicit int-to-struct cast rejection.
3) `07__diagjson__agg_union_missing_nested_field_reject_strict`
   - Strict code+line `.diagjson` parity for nested aggregate-member rejection.
4) `07__diagjson__agg_invalid_member_reject_strict`
   - Strict code+line `.diagjson` parity for unknown aggregate-member rejection.

## Wave 17 Additions (Constexpr Coupled ICE Frontier V)
1) `07__constexpr_shared_ice_matrix_runtime`
   - Coupled ICE matrix across array size, switch case labels, and static initializers.
2) `07__constexpr_shared_ice_matrix_nested_runtime`
   - Nested coupled ICE matrix across the same three constant-expression surfaces.
3) `07__constexpr_shared_ice_cross_surface_depth_runtime`
   - Cross-surface coupled ICE depth lane with multi-array/static/switch linkage.

## Wave 18 Additions (Aggregate Runtime Promotions)
1) `07__agg_member_access_runtime`
   - Runtime promotion for nested aggregate member-access shape coverage (`.` + `->` pathing).
2) `07__agg_offsets_runtime`
   - Runtime promotion for `offsetof` ordering and aggregate layout stride invariants.
