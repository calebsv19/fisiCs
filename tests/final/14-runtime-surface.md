# Runtime / Library Surface

## Status Snapshot (2026-03-21)

- Active test count: 85 (`tests/final/meta/14-runtime-surface*.json`)
- Runtime+differential tests: 80 (`65` strict differential + `8` UB-policy + `7` implementation-defined-policy)
- Current bucket run: `PROBE_FILTER=14__*` probe lane is in expansion (`48 resolved, 7 blocked`)
- Next expansion plan: `docs/plans/runtime_bucket_14_execution_plan.md`

## Scope
Minimal headers and builtin surface to compile real programs.

## Validate
- Provided headers: stddef.h, stdint.h, stdbool.h, limits.h (as supported).
- Builtins/intrinsics you choose to implement.
- offsetof macro expansion via system headers.

## Acceptance Checklist
- System-like headers compile and expose expected typedefs/macros.
- stdbool.h defines bool/true/false appropriately.
- stdint.h defines fixed-width types.
- stddef.h exposes size_t and NULL.
- offsetof computes field offsets in headers/macros.
- limits.h defines key integer limits.

## Test Cases (initial list)
1) `14__header_stdbool`
   - bool/true/false compile.
2) `14__header_stdint`
   - int8_t/uint32_t usage.
3) `14__header_stddef`
   - size_t and NULL usage.
4) `14__header_limits`
   - integer limits macros usable.
5) `14__offsetof`
   - offsetof macro expands and folds to a constant.
6) `14__runtime_div_mod_edges`
   - Runtime division/modulo edge behavior (`7/3`, `7%3`, `-7/3`).
7) `14__runtime_pointer_arith`
   - Runtime pointer-difference and indexed load behavior.
8) `14__runtime_recursion`
   - Recursive call lowering and runtime stack behavior.
9) `14__runtime_struct_return`
   - Struct return ABI/runtime behavior.
10) `14__runtime_function_pointer`
   - Function pointer call runtime behavior.
11) `14__runtime_float_precision`
   - Floating precision surface behavior (`0.1 + 0.2` formatting).
12) `14__runtime_float_infinity`
   - Runtime infinity comparison behavior (`1.0/0.0 > 1e300`).
13) `14__runtime_large_stack`
   - Larger stack frame handling.
14) `14__runtime_switch_simple`
   - Runtime switch dispatch correctness for direct case matches.
15) `14__runtime_args_env`
   - Harness runtime `run_args` and `run_env` behavior.
16) `14__runtime_stdin`
   - Harness runtime `run_stdin` behavior.
17) `14__runtime_variadic_calls`
   - Variadic call-site behavior through libc `printf`.
18) `14__runtime_deep_recursion`
   - Deep recursive call-path stress (`tri(300)`).
19) `14__runtime_struct_layout_alignment`
   - Runtime struct layout and alignment-offset checks.

## Open Gaps (Tracked)
- No active blockers in the current `14__probe_` lane.

## Wave 2 Additions (Codegen Probe Promotion)
20) `14__runtime_loop_continue_break_phi`
   - Loop-carried accumulator with `continue`/`break` edges.
21) `14__runtime_short_circuit_chain_effects`
   - Nested short-circuit chain side effects and evaluation ordering.
22) `14__runtime_struct_copy_update`
   - Struct return + by-value copy/update runtime behavior.
23) `14__runtime_pointer_stride_long_long`
   - Pointer-difference scaling and pointer-index stride on `long long*`.
24) `14__runtime_global_partial_init_zerofill`
   - Global partial initializer zero-fill semantics.

## Wave 3 Additions (ABI + Control Flow)
25) `14__runtime_struct_mixed_width_pass_return`
   - Mixed-width struct parameter/return ABI path (`int`, `long long`, `double`).
26) `14__runtime_struct_nested_copy_chain`
   - Nested aggregate copy/update chain semantics.
27) `14__runtime_global_designated_sparse`
   - Sparse designated global initializer + zero-fill verification.

## Wave 4 Additions (Control-Flow Promotion)
28) `14__runtime_switch_loop_control_mix`
   - Promoted from probe lane after `continue` target resolution fix in
     `switch` inside loop lowering.

## Wave 5 Additions (Arithmetic + Floating Start)
29) `14__runtime_unsigned_wraparound`
   - Promoted from `14__probe_unsigned_wrap` into active differential runtime.
30) `14__runtime_nan_propagation`
   - Promoted from `14__probe_float_nan` into active differential runtime.
31) `14__runtime_signed_div_mod_sign_matrix`
   - Added signed-operand division/modulo sign matrix and promoted into active differential runtime.
32) `14__runtime_nan_comparisons`
   - Added NaN comparison-matrix anchor and promoted into active differential runtime.

## Wave 5 Promotion
- `14__runtime_float_cast_roundtrip`
  - Promoted after codegen cast/unary-float fixes; now matches clang.

## Wave 6 Additions (ABI / Call Surface)
33) `14__runtime_many_args_mixed_width`
   - Promoted from `14__probe_many_args_mixed_width` into active differential runtime.
34) `14__runtime_variadic_promotions_matrix`
   - Promoted after signedness-correct vararg integer promotion fixes.
35) `14__runtime_struct_with_array_pass_return`
   - Promoted after array-element pointer/load lowering fixes.
36) `14__runtime_union_payload_roundtrip`
   - Promoted after semantic long-width and union layout fixes.
37) `14__runtime_fnptr_dispatch_table_mixed`
   - Promoted after function-pointer array element-type GEP normalization fix.
38) `14__runtime_float_cast_roundtrip`
   - Added to active runtime list as wave5 closure item.

## Wave 7 (Memory / Layout Depth)
39) `14__runtime_pointer_negative_offset`
   - Promoted to active runtime suite and validated.

## Wave 7 Probe Closures
- `14__probe_vla_stride_indexing`, `14__probe_alignment_long_double_struct`,
  `14__probe_struct_array_byte_stride`, `14__probe_union_embedded_alignment`,
  and `14__probe_vla_row_pointer_decay` are all resolved and green in
  `PROBE_FILTER=14__probe_` runs.

## Wave 8 Additions (Control + VLA Pointer Difference)
40) `14__runtime_nested_switch_fallthrough_loop`
   - Promoted after probe validation of nested `switch` fallthrough/loop control edges.
41) `14__runtime_short_circuit_side_effect_counter`
   - Promoted as explicit short-circuit side-effect counter anchor.
42) `14__runtime_vla_ptrdiff_row_size_dynamic`
   - Promoted as dynamic VLA row-pointer subtraction/stride scaling anchor.

## Wave 9 Additions (Probe Promotion: VLA + Layout)
43) `14__runtime_vla_stride_indexing`
   - Promoted from probe lane and validated in active differential runtime.
44) `14__runtime_alignment_long_double_struct`
   - Promoted from probe lane as long-double alignment/offset anchor.
45) `14__runtime_struct_array_byte_stride`
   - Promoted from probe lane as struct-array stride/layout anchor.
46) `14__runtime_union_embedded_alignment`
   - Promoted from probe lane as embedded-union alignment/offset anchor.
47) `14__runtime_vla_row_pointer_decay`
   - Promoted from probe lane as VLA row-pointer decay/ptrdiff anchor.

## Wave 10 Additions (UB Policy Lane)
48) `14__runtime_signed_overflow_ub_path`
   - Added as explicit UB-policy runtime lane (`ub: true`).
   - Differential is intentionally skipped by harness policy; compile+runtime
     path remains required and tracked.
49) `14__runtime_signed_mul_overflow_ub_path`
   - Added as UB-policy marker for signed multiply overflow paths.
50) `14__runtime_signed_sub_overflow_ub_path`
   - Added as UB-policy marker for signed subtract overflow paths.
51) `14__runtime_shift_count_ub_path`
   - Added as UB-policy marker for invalid shift-count paths.
52) `14__runtime_preinc_intmax_ub_path`
   - Added as UB-policy marker for pre-increment overflow paths.

## Wave 11 Additions (Implementation-Defined Policy Lane)
53) `14__runtime_impldef_signed_right_shift`
   - Added as implementation-defined signed right-shift policy lane (`impl_defined: true`).
54) `14__runtime_impldef_char_signedness`
   - Added as implementation-defined `char` signedness policy lane (`impl_defined: true`).
55) `14__runtime_impldef_enum_neg_cast`
   - Added as implementation-defined enum cast/sign policy lane (`impl_defined: true`).

## Wave 12 Additions (Strict Differential Expansion)
56) `14__runtime_pointer_diff_compare_matrix`
   - Added as strict differential pointer-difference/order matrix anchor.
57) `14__runtime_bitwise_shift_mask_matrix`
   - Added as strict differential bitwise/shift mask matrix anchor.

## Wave 13 Additions (Strict Differential Expansion)
58) `14__runtime_ternary_side_effect_phi`
   - Added as strict differential ternary branch/phi and side-effect ordering anchor.
59) `14__runtime_comma_sequence_matrix`
   - Added as strict differential comma-expression sequencing anchor.

## Wave 14 Additions (Implementation-Defined Policy Expansion)
60) `14__runtime_impldef_plain_int_bitfield_sign`
   - Added as implementation-defined plain-`int` bitfield signedness policy lane (`impl_defined: true`).
61) `14__runtime_impldef_enum_size_matrix`
   - Added as implementation-defined enum underlying-size policy lane (`impl_defined: true`).

## Wave 15 Additions (UB Policy Expansion)
62) `14__runtime_predec_intmin_ub_path`
   - Added as UB-policy marker for pre-decrement signed overflow paths (`ub: true`).
63) `14__runtime_add_assign_intmax_ub_path`
   - Added as UB-policy marker for `+=` signed overflow paths (`ub: true`).
64) `14__runtime_sub_assign_intmin_ub_path`
   - Added as UB-policy marker for `-=` signed overflow paths (`ub: true`).

## Wave 16 Additions (Strict Differential Expansion)
65) `14__runtime_call_sequence_conditional_mix`
   - Added as strict differential mixed call/control sequencing anchor.

## Wave 17 Additions (Strict Differential Expansion)
66) `14__runtime_fnptr_ternary_dispatch_accumulate`
   - Added as strict differential function-pointer ternary dispatch anchor.
67) `14__runtime_struct_ptrdiff_update_matrix`
   - Added as strict differential struct/ptrdiff update matrix anchor.

## Wave 18 Additions (Implementation-Defined Policy Expansion)
68) `14__runtime_impldef_signed_char_narrowing`
   - Added as implementation-defined signed-char narrowing policy lane (`impl_defined: true`).

## Wave 19 Additions (Strict Differential Expansion)
69) `14__runtime_switch_fnptr_dispatch_chain`
   - Added as strict differential switch/function-pointer dispatch chain anchor.
70) `14__runtime_fnptr_struct_pointer_pipeline`
   - Added as strict differential struct-pointer function pipeline anchor.

## Wave 20 Additions (Implementation-Defined Policy Expansion)
71) `14__runtime_impldef_long_double_size_align`
   - Added as implementation-defined `long double` size/alignment policy lane (`impl_defined: true`).

## Wave 21 Additions (Probe Closures Promoted)
72) `14__runtime_vla_param_matrix_reduce`
   - Promoted after VLA parameter pointer-index cast fix; now matches clang.
73) `14__runtime_fnptr_struct_by_value_dispatch`
   - Promoted after typedef/function-pointer return refinement and struct-by-value dispatch fixes.

## Wave 22 Additions (Probe Promotions)
74) `14__runtime_fnptr_typedef_return_direct`
   - Promoted from probe lane after typedef function-pointer direct-return call path validation.
75) `14__runtime_fnptr_typedef_return_ternary_callee`
   - Promoted after non-identifier callee signature recovery fix in codegen call lowering.
76) `14__runtime_fnptr_expression_callee_chain`
   - Promoted from probe lane for expression-callee function-pointer chain behavior.
77) `14__runtime_pointer_index_width_signedness`
   - Promoted for signed/unsigned index width behavior over pointer arithmetic paths.
78) `14__runtime_vla_param_mixed_signed_unsigned_indices`
   - Promoted after VLA mixed index cast-width fix; now matches clang runtime behavior.

## Wave 23 Probe Expansion (Closed)
1) `14__probe_bitfield_unsigned_pack_roundtrip`
   - Resolved vs clang (`5 17 41 2 6275`).
2) `14__probe_variadic_promotion_edges`
   - Resolved vs clang (`-3 250 1.50 -2.25`).
3) `14__probe_fnptr_nested_return_dispatch_matrix`
   - Resolved vs clang (`6 4 -8`).
4) `14__probe_fnptr_chooser_roundtrip_call`
   - Resolved vs clang (`6 8`).
5) `14__probe_vla_three_dim_stride_reduce`
   - Resolved vs clang (`2814 12 123`).
6) `14__probe_vla_three_dim_index_stride_basic`
   - Resolved vs clang (`123 12 8`).

Wave23 status:
- Closed after grouped codegen fixes for nested function-pointer return-call typing
  and 3D VLA indexing/stride lowering.

## Wave 24 Probe Expansion (In Progress)
1) `14__probe_fnptr_typedef_alias_chain_dispatch`
   - Resolved vs clang (`11 14 16`).
2) `14__probe_fnptr_chooser_table_ternary_chain`
   - Resolved vs clang (`8 3 3`).
3) `14__probe_vla_four_dim_stride_matrix`
   - Resolved vs clang (`1213 24 16 2223`).

Wave24 status:
- Probe lane additions are green and currently staged for promotion review.

## Wave 25 Probe Expansion (Closed)
1) `14__probe_fnptr_struct_temporary_chain`
   - Resolved vs clang (`10 12 7`) after semantic callable-target recovery fix for typedef/function-pointer struct-return call chains.
2) `14__probe_vla_param_slice_stride_rebase`
   - Resolved vs clang (`331 12 323`) after pointer-difference lowering moved to `LLVMBuildPtrDiff2` and VLA index cast-width normalization.
3) `14__probe_volatile_short_circuit_sequence`
   - Resolved vs clang (`7 975 1 1 24425`).

Wave25 status:
- Current full probe-lane snapshot:
  `PROBE_FILTER=14__*` => `resolved=36`, `blocked=0`, `skipped=0`.

## Wave 25 Promotion Closure
79) `14__runtime_fnptr_struct_temporary_chain`
   - Promoted from probe lane after grouped semantic/codegen fixes.
80) `14__runtime_vla_param_slice_stride_rebase`
   - Promoted from probe lane after VLA ptrdiff lowering fixes.
81) `14__runtime_volatile_short_circuit_sequence`
   - Promoted from probe lane as volatile sequencing anchor.

## Wave 26 Probe Expansion (Closed)
1) `14__probe_vla_ptrdiff_subslice_rebase_chain`
   - Resolved vs clang (`352 21 172`).
2) `14__probe_fnptr_struct_array_dispatch_pipeline`
   - Resolved vs clang (`55 18`).
3) `14__probe_ptrdiff_char_bridge_roundtrip`
   - Resolved vs clang (`14 14 5 73`).
4) `14__probe_volatile_comma_ternary_control_chain`
   - Resolved vs clang (`9 466 22884 1 160198`).
5) `14__probe_variadic_width_stress_matrix`
   - Resolved vs clang (`1238567890392`) after mapping `__builtin_va_list` to
     pointer-form lowering in codegen type resolution.

Wave26 status:
- Current full probe-lane snapshot:
  `PROBE_FILTER=14__*` => `resolved=41`, `blocked=0`, `skipped=0`.

## Wave 26 Promotion Closure
82) `14__runtime_vla_ptrdiff_subslice_rebase_chain`
   - Promoted from probe lane as VLA subslice-rebase ptrdiff anchor.
83) `14__runtime_fnptr_struct_array_dispatch_pipeline`
   - Promoted from probe lane as struct-wrapped function-pointer dispatch pipeline anchor.
84) `14__runtime_ptrdiff_char_bridge_roundtrip`
   - Promoted from probe lane as typed-pointer/byte-pointer delta roundtrip anchor.
85) `14__runtime_volatile_comma_ternary_control_chain`
   - Promoted from probe lane as volatile comma/ternary sequencing anchor.

## Wave 27 Probe Expansion (Closed)
1) `14__probe_variadic_vacopy_forwarder_matrix`
   - Resolved vs clang (`36666669468`) after builtin `va_copy` lowering fix.
2) `14__probe_variadic_nested_forwarder_table`
   - Resolved vs clang (`870`) after builtin `va_copy` lowering fix.
3) `14__probe_struct_large_return_pipeline`
   - Resolved vs clang (`-259 -483`) after aggregate assignment typing fixes.
4) `14__probe_struct_large_return_fnptr_pipeline`
   - Resolved vs clang (`1262 278`) after aggregate assignment typing fixes.
5) `14__probe_ptrdiff_struct_char_bridge_matrix`
   - Resolved vs clang (`6 6 72 74 22 17`) after typedef-surface pointer type preservation.
6) `14__probe_struct_union_by_value_roundtrip_chain`
   - Resolved vs clang (`616424 618408`) after union LLVM storage/body fix.
7) `14__probe_vla_param_cross_function_pipeline`
   - Resolved vs clang (`2449 23`) after pointer-qualification compatibility normalization.
8) `14__probe_variadic_fnptr_dispatch_chain`
   - Resolved vs clang (`443`).
9) `14__probe_vla_param_negative_ptrdiff_matrix`
   - Resolved vs clang (`381 -18 27`).
10) `14__probe_vla_rebased_slice_signed_unsigned_mix`
    - Resolved vs clang (`1879 40`).
11) `14__probe_fnptr_stateful_table_loop_matrix`
    - Resolved vs clang (`298 18`).
12) `14__probe_fnptr_return_struct_pipeline`
    - Resolved vs clang (`164 20`).
13) `14__probe_ptrdiff_reinterpret_longlong_bridge`
    - Resolved vs clang (`8 8 64 16 5007`).
14) `14__probe_recursive_fnptr_mix_runtime`
    - Resolved vs clang (`191 61`).

Wave27 status:
- Current full probe-lane snapshot:
  `PROBE_FILTER=14__*` => `resolved=55`, `blocked=0`, `skipped=0`.

## Wave 27 Promotion Closure
86) `14__runtime_variadic_vacopy_forwarder_matrix`
   - Promoted from probe lane after builtin `va_copy` lowering fix.
87) `14__runtime_variadic_nested_forwarder_table`
   - Promoted from probe lane as nested variadic forwarder anchor.
88) `14__runtime_ptrdiff_struct_char_bridge_matrix`
   - Promoted from probe lane as typed-struct/char-bridge pointer-difference anchor.
89) `14__runtime_struct_union_by_value_roundtrip_chain`
   - Promoted from probe lane as struct+union by-value roundtrip anchor.
90) `14__runtime_vla_param_cross_function_pipeline`
   - Promoted from probe lane as cross-function VLA parameter pipeline anchor.

## Wave 28 Promotion Closure
91) `14__runtime_variadic_width_stress_matrix`
   - Promoted from probe lane as width-mix variadic promotion anchor.
92) `14__runtime_variadic_fnptr_dispatch_chain`
   - Promoted from probe lane as variadic function-pointer dispatch anchor.
93) `14__runtime_vla_param_negative_ptrdiff_matrix`
   - Promoted from probe lane as negative/positive VLA ptrdiff anchor.
94) `14__runtime_vla_rebased_slice_signed_unsigned_mix`
   - Promoted from probe lane as mixed signed/unsigned VLA slice anchor.
95) `14__runtime_fnptr_stateful_table_loop_matrix`
   - Promoted from probe lane as stateful table-dispatch anchor.
96) `14__runtime_fnptr_return_struct_pipeline`
   - Promoted from probe lane as function-pointer selected struct-return anchor.
97) `14__runtime_ptrdiff_reinterpret_longlong_bridge`
   - Promoted from probe lane as long-long reinterpret delta bridge anchor.
98) `14__runtime_recursive_fnptr_mix_runtime`
   - Promoted from probe lane as recursive function-pointer path anchor.
99) `14__runtime_struct_large_return_pipeline`
   - Promoted from probe lane as large-struct by-value return anchor.
100) `14__runtime_struct_large_return_fnptr_pipeline`
   - Promoted from probe lane as function-pointer large-struct return anchor.

## Expected Outputs
- AST/Diagnostics/IR goldens for compile-surface checks.
- Runtime stdout/stderr/exit expectations for executable checks.
