# Runtime / Library Surface

## Status Snapshot (2026-03-28)

- Active test count: 329 (`tests/final/meta/14-runtime-surface*.json`)
- Runtime executables: 268 (`264` differential-flagged; includes policy-lane tests)
- Policy lanes: `11` UB + `11` implementation-defined
- Negative diagnostics: 56 (`28` `.diag` + `28` `.diagjson`)
- Current bucket run:
  - `PROBE_FILTER=14__probe_*` -> `resolved=234`, `blocked=0`, `skipped=0`
  - Wave42/Wave43/Wave44/Wave45/Wave46/Wave47/Wave48/Wave49/Wave50/Wave52/Wave53/Wave54/Wave55/Wave56/Wave57/Wave58/Wave59/Wave60/Wave61/Wave62/Wave63/Wave64/Wave65/Wave66/Wave67/Wave68/Wave69/Wave70/Wave71/Wave72/Wave73/Wave74/Wave75/Wave76/Wave77/Wave78/Wave79/Wave80/Wave81/Wave82/Wave83/Wave84/Wave85/Wave86/Wave87/Wave88/Wave89 exact-id checks -> all pass
  - `make final-runtime` -> `0 failing, 22 skipped`
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
- No blocked runtime probes currently.
- Runtime linking realism now has explicit multi-TU conflict lanes, and
  link-stage diagnostics-json export is now supported through a deterministic
  driver-level fallback (`code=4001`) when linker exits non-zero.
- ABI boundary coverage now includes explicit hardening lanes for reg/stack +
  `long double` + variadic mixes, but direct struct `va_arg` retrieval remains
  outside supported behavior in this toolchain lane.
- Variadic header/runtime frontier still has a tracked edge:
  `<stdarg.h>` preprocessing is unstable in this toolchain lane and direct
  struct retrieval via `va_arg(ap, struct ...)` can crash.
- Header/init frontier has a tracked edge:
  some struct-array `{0}` shorthand initializer paths are still brittle in this
  compiler lane (Wave85 stabilized this by avoiding that shorthand).
- Startup/initialization semantics now include deeper multi-TU init-order
  depth stress lanes; further expansion here is optional rather than required
  for bucket-14 stabilization.
- Deterministic mini-binary and repeatability hardening lanes are now active.

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

## Wave 29 Promotion Closure
101) `14__runtime_bitfield_unsigned_pack_roundtrip`
   - Promoted from probe lane as unsigned bitfield pack/unpack roundtrip anchor.
102) `14__runtime_bitfield_truncation_struct_flow`
   - Promoted from probe lane as bitfield truncation + by-value struct flow anchor.
103) `14__runtime_bitfield_unsigned_mask_merge_chain`
   - Promoted from probe lane as unsigned bitfield mask/merge mutation anchor.
104) `14__runtime_bitfield_by_value_roundtrip_simple`
   - Promoted from probe lane as by-value bitfield roundtrip/copy anchor.

Wave29 status:
- Current full probe-lane snapshot:
  `PROBE_FILTER=14__*` => `resolved=67`, `blocked=0`, `skipped=0`.
- Full final suite snapshot:
  `make final` => `0 failing`, `15 skipped`.

## Wave 30 Promotion Closure
105) `14__runtime_fnptr_alias_chooser_struct_chain`
   - Promoted from probe lane as alias chooser-chain through struct lanes.
106) `14__runtime_fnptr_alias_conditional_factory_simple`
   - Promoted from probe lane as alias conditional factory call-chain anchor.
107) `14__runtime_fnptr_alias_indirect_dispatch`
   - Promoted from probe lane as alias indirect dispatch/chooser anchor.
108) `14__runtime_fnptr_typedef_alias_chain_dispatch`
   - Promoted from probe lane as typedef-alias chooser chain anchor.
109) `14__runtime_fnptr_chooser_roundtrip_call`
   - Promoted from probe lane as chooser roundtrip call-surface anchor.
110) `14__runtime_fnptr_chooser_table_ternary_chain`
   - Promoted from probe lane as table + ternary function-callee chain anchor.
111) `14__runtime_fnptr_nested_return_dispatch_matrix`
   - Promoted from probe lane as nested return-dispatch matrix anchor.

Wave30 status:
- Full final suite snapshot:
  `make final` => `0 failing`, `15 skipped`.
- Targeted probe snapshot (wave30 ids):
  `resolved=7`, `blocked=0`, `skipped=0`.

## Wave 31 Promotion Closure
112) `14__runtime_variadic_promotion_edges`
   - Promoted from probe lane as variadic promotion edge anchor.
113) `14__runtime_variadic_promote_char_short_float_mix`
   - Promoted from probe lane as char/short/float promotion-mix anchor.
114) `14__runtime_variadic_signed_unsigned_char_matrix`
   - Promoted from probe lane as signed/unsigned char promotion matrix anchor.
115) `14__runtime_variadic_small_types_forward_chain`
   - Promoted from probe lane as small-types forwarding with `va_copy` anchor.

Wave31 status:
- Full final suite snapshot:
  `make final` => `0 failing`, `15 skipped`.
- Targeted probe snapshot (wave31 ids):
  `resolved=4`, `blocked=0`, `skipped=0`.

## Wave 32 Promotion Closure
116) `14__runtime_vla_three_dim_index_stride_basic`
   - Promoted from probe lane as 3D VLA indexing and slab/lane stride anchor.
117) `14__runtime_vla_three_dim_stride_reduce`
   - Promoted from probe lane as 3D VLA reduction + stride anchor.
118) `14__runtime_vla_four_dim_stride_matrix`
   - Promoted from probe lane as 4D VLA stride matrix anchor.
119) `14__runtime_vla_four_dim_slice_rebase_runtime`
   - Promoted from probe lane as 4D VLA slice-rebase pointer-delta anchor.
120) `14__runtime_vla_four_dim_param_handoff_reduce`
   - Promoted from probe lane as 4D VLA parameter handoff/reduction anchor.
121) `14__runtime_vla_four_dim_rebase_weighted_reduce`
   - Promoted from probe lane as weighted 4D VLA rebase reduction anchor.

Wave32 status:
- Full final suite snapshot:
  `make final` => `0 failing`, `15 skipped`.
- Targeted probe snapshot (wave32 ids):
  `resolved=6`, `blocked=0`, `skipped=0`.

## Wave 33 Promotion Closure
122) `14__runtime_unsigned_div_mod_extremes_matrix`
   - Promoted from probe lane as unsigned max-width divide/mod remainder anchor.
123) `14__runtime_signed_unsigned_cmp_boundary_matrix`
   - Promoted from probe lane as signed/unsigned boundary-compare anchor.
124) `14__runtime_float_signed_zero_inf_matrix`
   - Promoted from probe lane as signed-zero and infinity comparison anchor.
125) `14__runtime_cast_chain_width_sign_matrix`
   - Promoted from probe lane as signed/unsigned cast-width chain anchor.

Wave33 status:
- Targeted probe snapshot (wave33 ids):
  `resolved=4`, `blocked=0`, `skipped=0`.

## Wave 34 Promotion Closure
126) `14__runtime_header_stddef_ptrdiff_size_t_bridge`
   - Promoted from probe lane as `stddef.h` ptrdiff/size_t/offsetof runtime bridge anchor.
127) `14__runtime_header_stdint_intptr_uintptr_roundtrip`
   - Promoted from probe lane as `stdint.h` intptr/uintptr pointer roundtrip anchor.
128) `14__runtime_header_limits_llong_matrix`
   - Promoted from probe lane as `limits.h` long-long boundary macro usage anchor.
129) `14__runtime_header_stdbool_int_bridge`
   - Promoted from probe lane as `stdbool.h` bool/int conversion bridge anchor.

Wave34 status:
- One blocker was fixed during probe closure:
  `_Bool` cast lowering now uses truthiness conversion (`!= 0`) instead of integer truncation.
- Targeted probe snapshot (wave34 ids):
  `resolved=4`, `blocked=0`, `skipped=0`.

## Wave 35 Promotion Closure
130) `14__runtime_struct_bitfield_mixed_pass_return`
   - Promoted from probe lane as mixed-width bitfield by-value pass/return anchor.
131) `14__runtime_struct_double_int_padding_roundtrip`
   - Promoted from probe lane as padded struct by-value roundtrip/layout anchor.
132) `14__runtime_fnptr_variadic_dispatch_bridge`
   - Promoted from probe lane as function-pointer variadic dispatch anchor.
133) `14__runtime_many_args_struct_scalar_mix`
   - Promoted from probe lane as many-argument struct/scalar ABI mix anchor.

Wave35 status:
- Targeted probe snapshot (wave35 ids):
  `resolved=4`, `blocked=0`, `skipped=0`.

## Wave 36 Policy-Lane Expansion
134) `14__runtime_impldef_plain_int_bitfield_signedness_matrix`
   - Added as implementation-defined policy-lane coverage for plain-`int` bitfield signedness.
135) `14__runtime_impldef_signed_shift_char_matrix`
   - Added as implementation-defined policy-lane coverage for signed right-shift and plain-char signedness.
136) `14__runtime_ub_left_shift_negative_lhs_path`
   - Added as UB policy-lane coverage for left-shift of negative operands.
137) `14__runtime_ub_negate_intmin_path`
   - Added as UB policy-lane coverage for `-INT_MIN` overflow path.

Wave36 status:
- All four tests pass in targeted runs with expected policy-lane differential skips.
- Full final suite snapshot:
  `make final` => `0 failing`, `19 skipped`.

## Wave 37 Promotion Closure
138) `14__runtime_float_negzero_propagation_chain`
   - Promoted from probe lane as signed-zero propagation-chain anchor.
139) `14__runtime_ptrdiff_one_past_end_matrix`
   - Promoted from probe lane as one-past-end pointer/ptrdiff matrix anchor.
140) `14__runtime_many_args_float_int_struct_mix`
   - Promoted from probe lane as mixed struct/int/double many-arg ABI anchor.
141) `14__runtime_variadic_llong_double_bridge`
   - Promoted from probe lane as variadic long-long/double width bridge anchor.

Wave37 status:
- Targeted probe snapshot (wave37 ids):
  `resolved=4`, `blocked=0`, `skipped=0`.
- Full probe-lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=87`, `blocked=0`, `skipped=0`.
- Exact-id runtime checks pass for all 4 promoted ids.

## Wave 38 Promotion Closure
142) `14__runtime_control_do_while_switch_phi_chain`
   - Promoted from probe lane as do-while + switch + continue/goto phi anchor.
143) `14__runtime_struct_array_double_by_value_roundtrip`
   - Promoted from probe lane as struct(array+double) by-value roundtrip anchor.
144) `14__runtime_vla_dim_recompute_stride_matrix`
   - Promoted from probe lane as VLA dimension-recompute stride anchor.
145) `14__runtime_pointer_byte_rebase_roundtrip_matrix`
   - Promoted from probe lane as byte-rebase pointer roundtrip/ptrdiff anchor.

Wave38 status:
- Targeted probe snapshot (wave38 ids):
  `resolved=4`, `blocked=0`, `skipped=0`.
- Full probe-lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=91`, `blocked=0`, `skipped=0`.
- Exact-id runtime checks pass for all 4 promoted ids.

## Wave 39 Promotion Closure
146) `14__runtime_float_nan_inf_order_chain`
   - Promoted from probe lane as NaN/Infinity ordering chain anchor.
147) `14__runtime_header_null_ptrdiff_bridge`
   - Promoted from probe lane as NULL + ptrdiff header bridge anchor.
148) `14__runtime_struct_union_array_overlay_roundtrip`
   - Promoted from probe lane as struct/union/array overlay by-value roundtrip anchor.
149) `14__runtime_variadic_long_width_crosscheck`
   - Promoted from probe lane as variadic long-width crosscheck anchor.

Wave39 status:
- Targeted probe snapshot (wave39 ids):
  `resolved=4`, `blocked=0`, `skipped=0`.
- Full probe-lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=91`, `blocked=0`, `skipped=0`.
- Full final suite snapshot:
  `make final` => `0 failing`, `19 skipped`.

## Wave 40 Promotion Closure
150) `14__runtime_struct_ternary_by_value_select_chain`
   - Promoted from probe lane as struct ternary by-value selection anchor.
151) `14__runtime_switch_sparse_signed_case_ladder`
   - Promoted from probe lane as sparse signed switch dispatch anchor.
152) `14__runtime_bitwise_compound_recurrence_matrix`
   - Promoted from probe lane as bitwise compound recurrence anchor.
153) `14__runtime_header_offsetof_nested_matrix`
   - Promoted from probe lane as nested `offsetof` member-designator anchor.

Wave40 status:
- Targeted probe snapshot (wave40 ids):
  `resolved=4`, `blocked=0`, `skipped=0`.
- Full probe-lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=95`, `blocked=0`, `skipped=0`.
- Full final suite snapshot:
  `make final` => `0 failing`, `19 skipped`.

## Wave 41 Promotion Closure
154) `14__runtime_harness_exit_stderr_nonzero_arg`
   - Added as harness contract anchor for non-zero runtime exit + non-empty runtime stderr.
155) `14__runtime_harness_env_stderr_nonzero`
   - Added as harness contract anchor for stdin-driven non-zero runtime exit + non-empty runtime stderr.
156) `14__runtime_multitu_struct_return_bridge`
   - Added as true multi-TU struct-return ABI bridge anchor.
157) `14__runtime_multitu_variadic_bridge`
   - Added as true multi-TU variadic ABI bridge anchor.

Wave41 status:
- Targeted exact-id checks:
  all 4 Wave41 tests pass.
- Full probe-lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=95`, `blocked=0`, `skipped=0`.
- Full final suite snapshot:
  `make final` => `0 failing`, `19 skipped`.

## Wave 42 Promotion Closure
158) `14__runtime_harness_args_stderr_exit13`
   - Added as harness contract anchor for argv-driven non-zero runtime exit + non-empty runtime stderr.
159) `14__runtime_harness_stdin_stderr_exit17`
   - Added as harness contract anchor for stdin-driven non-zero runtime exit + non-empty runtime stderr.
160) `14__runtime_multitu_fnptr_callback_accumulate`
   - Added as true multi-TU function-pointer callback/accumulate ABI anchor.
161) `14__runtime_multitu_union_struct_dispatch`
   - Added as true multi-TU union+struct dispatch/return ABI anchor.
162) `14__runtime_multitu_vla_param_bridge`
   - Added as true multi-TU VLA parameter bridge ABI anchor.

Wave42 status:
- Targeted exact-id checks:
  all 5 Wave42 tests pass.

## Wave 43 Promotion Closure
163) `14__runtime_multitu_struct_array_return_chain`
   - Added as true multi-TU struct-with-array by-value return/call chain ABI anchor.
164) `14__runtime_multitu_mixed_abi_call_chain`
   - Added as true multi-TU mixed scalar/floating/aggregate call ABI anchor.
165) `14__runtime_multitu_fnptr_struct_fold_pipeline`
   - Added as true multi-TU function-pointer + struct-config fold pipeline ABI anchor.
166) `14__runtime_multitu_nested_aggregate_roundtrip`
   - Added as true multi-TU nested aggregate by-value roundtrip ABI anchor.

Wave43 status:
- Targeted exact-id checks:
  all 4 Wave43 tests pass.

## Wave 44 Promotion Closure
167) `14__runtime_many_args_reg_stack_split_matrix`
   - Added as many-argument mixed scalar/aggregate register+stack split ABI anchor.
168) `14__runtime_variadic_kind_fold_matrix`
   - Added as variadic kind-tag folding matrix anchor across int/unsigned/double/long long lanes.
169) `14__runtime_large_struct_return_select_chain`
   - Added as large-struct return and ternary selection chain ABI anchor.
170) `14__runtime_bitfield_pack_rewrite_matrix`
   - Added as unsigned bitfield pack/rewrite mutation-chain anchor.

Wave44 status:
- `make final-wave WAVE=44`:
  all 4 Wave44 tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `19 skipped`.

## Wave 45 Promotion Closure
171) `14__runtime_multitu_large_struct_return_select`
   - Added as deep multi-TU large-struct return/select ABI anchor.
172) `14__runtime_multitu_variadic_route_table`
   - Added as deep multi-TU variadic route-table + `va_copy` ABI anchor.
173) `14__runtime_multitu_fnptr_return_ladder`
   - Added as deep multi-TU function-pointer return ladder ABI anchor.
174) `14__runtime_multitu_vla_stride_bridge`
   - Added as deep multi-TU VLA stride bridge ABI anchor.

Wave45 status:
- `make final-wave WAVE=45`:
  all 4 Wave45 tests pass.

## Wave 46 Promotion Closure
175) `14__runtime_layout_stride_invariants_matrix`
   - Added as runtime layout offset/stride invariant matrix anchor.
176) `14__runtime_layout_nested_offset_consistency`
   - Added as runtime nested-aggregate offset-consistency anchor.
177) `14__runtime_layout_union_byte_rewrite_invariants`
   - Added as runtime object-representation byte rewrite invariant anchor.
178) `14__runtime_layout_vla_subslice_stride_matrix`
   - Added as runtime VLA subslice stride/layout matrix anchor.

Wave46 status:
- `make final-wave WAVE=46`:
  all 4 Wave46 tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `19 skipped`.

## Wave 47 Promotion Closure
179) `14__runtime_multitu_tentative_def_zero_init_bridge`
   - Added as multi-TU tentative-definition zero-init linkage anchor.
180) `14__runtime_multitu_extern_array_size_consistency`
   - Added as multi-TU extern array size/shape consistency anchor.
181) `14__runtime_multitu_static_extern_partition`
   - Added as multi-TU static vs extern partition behavior anchor.
182) `14__runtime_multitu_global_struct_init_bridge`
   - Added as multi-TU global struct initialization/update bridge anchor.

Wave47 status:
- `make final-wave WAVE=47`:
  all 4 Wave47 tests pass.

## Wave 48 Promotion Closure
183) `14__runtime_multitu_fnptr_qualifier_decay_bridge`
   - Added as multi-TU function-pointer qualifier/decay bridge anchor.
184) `14__runtime_multitu_array_param_decay_stride`
   - Added as multi-TU array-parameter decay + row-stride bridge anchor.
185) `14__runtime_multitu_const_volatile_ptr_pipeline`
   - Added as multi-TU const-pointer pipeline anchor (qualifier-safe call path).
186) `14__runtime_multitu_struct_with_fnptr_return_chain`
   - Added as multi-TU struct-with-function-pointer return chain anchor.

Wave48 status:
- `make final-wave WAVE=48`:
  all 4 Wave48 tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `19 skipped`.

## Wave 49 Promotion Closure
187) `14__runtime_layout_bitfield_container_boundary_matrix`
   - Added as implementation-defined bitfield-container boundary policy lane (`impl_defined: true`).
188) `14__runtime_layout_enum_underlying_width_bridge`
   - Added as implementation-defined enum underlying-width policy lane (`impl_defined: true`).
189) `14__runtime_layout_ptrdiff_large_span_matrix`
   - Added as strict differential large-span ptrdiff/layout anchor.
190) `14__runtime_layout_nested_vla_rowcol_stride_bridge`
   - Added as strict differential nested VLA row/column stride bridge anchor.

Wave49 status:
- `make final-wave WAVE=49`:
  all 4 Wave49 tests pass (`2` expected policy skips).
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `21 skipped`.
- Full final suite:
  `make final` => `0 failing`, `21 skipped`.

## Wave 50 Promotion Closure
191) `14__runtime_bool_scalar_conversion_matrix`
   - Added as strict differential scalar-to-bool conversion matrix anchor across int/float/pointer lanes.
192) `14__runtime_bitfield_signed_promotion_matrix`
   - Added as strict differential signed-bitfield promotion/update matrix anchor.
193) `14__runtime_bitfield_compound_assign_bridge`
   - Added as strict differential unsigned-bitfield compound-update bridge anchor.
194) `14__runtime_bitfield_bool_bridge_matrix`
   - Added as strict differential bitfield+bool bridge/control anchor.

Wave50 status:
- Probe lane:
  `PROBE_FILTER=14__probe_*` => `resolved=99`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=50`:
  all 4 Wave50 tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `21 skipped`.
- Full final suite:
  `make final` => `0 failing`, `21 skipped`.

## Wave 52 Promotion Closure
195) `14__runtime_multitu_fnptr_table_state_bridge`
   - Added as strict differential multi-TU function-pointer table/state bridge anchor.
196) `14__runtime_multitu_variadic_fold_bridge`
   - Added as strict differential multi-TU mixed-width fold bridge anchor.
197) `14__runtime_multitu_vla_window_reduce_bridge`
   - Added as strict differential multi-TU VLA window-reduction bridge anchor.
198) `14__runtime_multitu_struct_union_route_bridge`
   - Added as strict differential multi-TU struct/union route bridge anchor.

Wave52 status:
- Targeted wave probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=52`:
  all 4 Wave52 tests pass.

## Wave 53 Promotion Closure
199) `14__runtime_control_switch_do_for_state_machine`
   - Added as strict differential control-flow state-machine anchor across `for`/`do`/`switch`.
200) `14__runtime_layout_offset_stride_bridge`
   - Added as strict differential `offsetof`/stride bridge anchor.
201) `14__runtime_pointer_stride_rebase_control_mix`
   - Added as strict differential pointer-stride rebasing + mixed-control anchor.
202) `14__runtime_bool_bitmask_control_chain`
   - Added as strict differential bool-bitmask control chain anchor.

Wave53 status:
- Targeted wave probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- Full probe lane:
  `PROBE_FILTER=14__probe_*` => `resolved=107`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=53`:
  all 4 Wave53 tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `21 skipped`.
- Full final suite:
  `make final` => `0 failing`, `21 skipped`.

## Wave 54 Promotion Closure
203) `14__runtime_control_goto_cleanup_counter_matrix`
   - Added as strict differential nested-loop `goto` cleanup control-flow anchor.
204) `14__runtime_vla_sizeof_rebind_stride_matrix`
   - Added as strict differential VLA rebind + `sizeof(row)` stride anchor.
205) `14__runtime_fnptr_array_ternary_reseed_matrix`
   - Added as strict differential function-pointer array + ternary reseed anchor.
206) `14__runtime_ptrdiff_signed_index_rebase_matrix`
   - Added as strict differential signed/unsigned index rebasing + ptrdiff anchor.

Wave54 status:
- Targeted wave probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=54`:
  all 4 Wave54 tests pass.

## Wave 55 Promotion Closure
207) `14__diag__continue_outside_loop_reject`
   - Added as control diagnostics hardening anchor for `continue` outside iterative statements.
208) `14__diag__break_outside_loop_reject`
   - Added as control diagnostics hardening anchor for `break` outside loop/switch statements.
209) `14__diag__case_outside_switch_reject`
   - Added as control diagnostics hardening anchor for `case` label outside switch statements.
210) `14__diag__default_outside_switch_reject`
   - Added as control diagnostics hardening anchor for `default` label outside switch statements.

Wave55 status:
- Targeted diagnostics probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- Full probe lane:
  `PROBE_FILTER=14__probe_*` => `resolved=119`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=55`:
  all 4 Wave55 tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `21 skipped`.

## Wave 56 Promotion Closure
211) `14__runtime_multitu_static_local_counter_bridge`
   - Added as strict differential multi-TU static-local function-state persistence anchor.
212) `14__runtime_multitu_static_storage_slice_bridge`
   - Added as strict differential multi-TU static storage slice/update bridge anchor.
213) `14__runtime_vla_row_pointer_handoff_rebase_matrix`
   - Added as strict differential VLA row-pointer handoff/rebase stride anchor.
214) `14__runtime_fnptr_struct_state_reseed_loop_matrix`
   - Added as strict differential function-pointer struct-state reseed loop anchor.

Wave56 status:
- Targeted wave probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- Full probe lane:
  `PROBE_FILTER=14__probe_*` => `resolved=123`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=56`:
  all 4 Wave56 tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `21 skipped`.

## Wave 57 Promotion Closure
215) `14__diag__switch_duplicate_default_reject`
   - Added as diagnostics hardening anchor for duplicate `default` labels in one switch.
216) `14__diag__switch_duplicate_case_reject`
   - Added as diagnostics hardening anchor for duplicate `case` labels in one switch.
217) `14__diag__switch_nonconst_case_reject`
   - Added as diagnostics hardening anchor for non-constant `case` labels.
218) `14__diag__continue_in_switch_reject`
   - Added as diagnostics hardening anchor for `continue` inside switch without an enclosing loop.

Wave57 status:
- Targeted diagnostics probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- Full probe lane:
  `PROBE_FILTER=14__probe_*` => `resolved=127`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=57`:
  all 4 Wave57 tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `21 skipped`.

## Wave 58 Promotion Closure
219) `14__runtime_multitu_fnptr_rotation_bridge`
   - Added as strict differential multi-TU function-pointer rotation/dispatch ABI anchor.
220) `14__runtime_multitu_pair_fold_bridge`
   - Added as strict differential multi-TU struct pair fold/update bridge anchor.
221) `14__runtime_vla_column_pointer_stride_mix`
   - Added as strict differential VLA column-pointer stride arithmetic anchor.
222) `14__runtime_switch_goto_reentry_matrix`
   - Added as strict differential switch/goto reentry control-flow matrix anchor.

Wave58 status:
- Targeted wave probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- Full probe lane:
  `PROBE_FILTER=14__probe_*` => `resolved=131`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=58`:
  all 4 Wave58 tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `21 skipped`.
- Full final suite:
  `make final` => `0 failing`, `21 skipped`.

## Wave 59 Promotion Closure
223) `14__runtime_restrict_nonoverlap_accumulate_matrix`
   - Added as strict differential restrict-qualified non-overlap accumulation matrix anchor.
224) `14__runtime_restrict_vla_window_stride_matrix`
   - Added as strict differential restrict-qualified VLA window/stride anchor.
225) `14__runtime_multitu_restrict_bridge`
   - Added as strict differential multi-TU restrict bridge pipeline anchor.
226) `14__runtime_restrict_alias_overlap_ub_path`
   - Added as UB-policy restrict-overlap lane (`ub: true`, differential intentionally skipped).

Wave59 status:
- Targeted wave probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=59`:
  `3` strict differential passes + `1` expected UB-policy skip.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 60 Promotion Closure
227) `14__runtime_long_double_call_chain_matrix`
   - Added as strict differential long-double representation/hash call-chain anchor.
228) `14__runtime_long_double_variadic_bridge`
   - Added as strict differential long-double variadic ABI bridge anchor.
229) `14__runtime_multitu_long_double_bridge`
   - Added as strict differential multi-TU long-double ABI transfer anchor.
230) `14__runtime_long_double_struct_pass_return`
   - Added as strict differential long-double struct pass/return ABI anchor.

Wave60 status:
- Targeted wave probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=60`:
  all 4 Wave60 tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.
- Full probe lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=139`, `blocked=0`, `skipped=0`.

## Wave 61 Promotion Closure
231) `14__runtime_complex_addsub_eq_matrix`
   - Added as strict differential complex add/sub + equality matrix anchor.
232) `14__runtime_complex_unary_scalar_promotion_chain`
   - Added as strict differential complex unary + scalar-promotion anchor.
233) `14__runtime_complex_param_return_bridge`
   - Added as strict differential complex parameter/return bridge anchor.
234) `14__runtime_multitu_complex_bridge`
   - Added as strict differential multi-TU complex bridge anchor.

Wave61 status:
- Targeted wave probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=61`:
  all 4 Wave61 tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 62 Promotion Closure
235) `14__diag__fnptr_table_too_many_args_reject`
   - Added as fnptr diagnostics hardening anchor for too-many-arguments calls.
236) `14__diag__fnptr_struct_arg_incompatible_reject`
   - Added as fnptr diagnostics hardening anchor for non-callable struct argument.
237) `14__diag__fnptr_param_noncallable_reject`
   - Added as fnptr diagnostics hardening anchor for non-callable integer argument.

Wave62 status:
- Targeted diagnostics probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=62`:
  all 3 Wave62 diagnostics tests pass.
- Full probe lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=146`, `blocked=0`, `skipped=0`.

## Wave 63 Promotion Closure
238) `14__runtime_complex_struct_pass_return_bridge`
   - Added as strict differential by-value struct pass/return lane with `_Complex` fields.
239) `14__runtime_complex_struct_array_pass_return_chain`
   - Added as strict differential struct-with-complex-array pass/return chain lane.
240) `14__runtime_complex_nested_struct_field_bridge`
   - Added as strict differential nested-struct field bridge lane with `_Complex` payloads.
241) `14__runtime_complex_struct_ternary_select_bridge`
   - Added as strict differential ternary selection lane over structs carrying `_Complex` fields.

Wave63 status:
- Targeted wave probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=63`:
  all 4 Wave63 tests pass.

## Wave 64 Promotion Closure
242) `14__runtime_multitu_complex_struct_pass_return`
   - Added as strict differential multi-TU by-value struct pass/return lane with `_Complex` fields.
243) `14__runtime_multitu_complex_nested_route`
   - Added as strict differential multi-TU nested route lane with `_Complex` payloads.
244) `14__runtime_complex_struct_pointer_roundtrip_bridge`
   - Added as strict differential pointer roundtrip read lane over structs containing `_Complex` fields.
245) `14__runtime_complex_struct_copy_assign_chain`
   - Added as strict differential copy/assign chain lane over structs containing `_Complex` fields.

Wave64 status:
- Targeted wave probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=64`:
  all 4 Wave64 tests pass.

## Wave 65 Promotion Closure
246) `14__runtime_complex_layout_size_align_matrix`
   - Added as strict differential `_Complex` size/alignment and field-offset matrix lane.
247) `14__runtime_complex_layout_nested_offset_matrix`
   - Added as strict differential nested offset lane around `_Complex` fields.
248) `14__runtime_complex_layout_array_stride_matrix`
   - Added as strict differential array-stride lane for structs containing `_Complex` fields.
249) `14__runtime_complex_layout_union_overlay_roundtrip`
   - Added as strict differential union overlay roundtrip lane through struct/flat complex views.

Wave65 status:
- Targeted wave probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=65`:
  all 4 Wave65 tests pass.
- Full probe lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=158`, `blocked=0`, `skipped=0`.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 70 Promotion Closure
250) `14__diagjson__continue_outside_loop_reject`
   - Added as diagnostics-JSON assertion lane for `continue` outside loop rejection.
251) `14__diagjson__break_outside_loop_reject`
   - Added as diagnostics-JSON assertion lane for `break` outside loop/switch rejection.
252) `14__diagjson__case_outside_switch_reject`
   - Added as diagnostics-JSON assertion lane for `case` outside switch rejection.
253) `14__diagjson__default_outside_switch_reject`
   - Added as diagnostics-JSON assertion lane for `default` outside switch rejection.

Wave70 status:
- Targeted diagnostic-json probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=70`:
  all 4 Wave70 diagnostics-json tests pass.
- Full probe lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=176`, `blocked=0`, `skipped=0`.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 71 Promotion Closure
254) `14__diagjson__switch_duplicate_default_reject`
   - Added as diagnostics-JSON assertion lane for duplicate `default` labels in switch.
255) `14__diagjson__switch_duplicate_case_reject`
   - Added as diagnostics-JSON assertion lane for duplicate `case` labels in switch.
256) `14__diagjson__switch_nonconst_case_reject`
   - Added as diagnostics-JSON assertion lane for non-constant `case` label rejection.
257) `14__diagjson__continue_in_switch_reject`
   - Added as diagnostics-JSON assertion lane for `continue` in switch without loop.

Wave71 status:
- Targeted diagnostic-json probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=71`:
  all 4 Wave71 diagnostics-json tests pass.

## Wave 72 Promotion Closure
258) `14__diagjson__fnptr_table_too_many_args_reject`
   - Added as diagnostics-JSON assertion lane for function-pointer too-many-arguments rejection.
259) `14__diagjson__fnptr_struct_arg_incompatible_reject`
   - Added as diagnostics-JSON assertion lane for function-pointer incompatible struct-argument rejection.
260) `14__diagjson__fnptr_param_noncallable_reject`
   - Added as diagnostics-JSON assertion lane for function-pointer non-callable-argument rejection.

Wave72 status:
- Targeted diagnostic-json probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=72`:
  all 3 Wave72 diagnostics-json tests pass.
- Full probe lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=183`, `blocked=0`, `skipped=0`.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 73 Promotion Closure
261) `14__diagjson__file_scope_vla_reject`
   - Added as diagnostics-JSON assertion lane for file-scope VLA static-duration rejection.
262) `14__diagjson__offsetof_bitfield_reject`
   - Added as diagnostics-JSON assertion lane for `offsetof` on bitfield rejection.
263) `14__diagjson__ternary_struct_condition_reject`
   - Added as diagnostics-JSON assertion lane for ternary non-scalar condition rejection.
264) `14__diagjson__fnptr_table_too_few_args_reject`
   - Added as diagnostics-JSON assertion lane for function-pointer too-few-args rejection.

Wave73 status:
- Targeted diagnostic-json probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=73`:
  all 4 Wave73 diagnostics-json tests pass.
- Full probe lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=187`, `blocked=0`, `skipped=0`.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 74 Promotion Closure
265) `14__diag__switch_struct_condition_reject`
   - Added as diagnostics lane for switch with non-integer struct controlling expression.
266) `14__diag__if_struct_condition_reject`
   - Added as diagnostics lane for if with non-scalar struct controlling expression.
267) `14__diag__while_struct_condition_reject`
   - Added as diagnostics lane for while with non-scalar struct controlling expression.
268) `14__diag__return_struct_to_int_reject`
   - Added as diagnostics lane for incompatible return type (struct returned from int function).

Wave74 status:
- Targeted diagnostics probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- Targeted diagnostic-json probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=74`:
  all 4 Wave74 diagnostic tests pass (`.diag` + `.diagjson`).
- Full probe lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=195`, `blocked=0`, `skipped=0`.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 75 Promotion Closure
269) `14__diag__fnptr_table_incompatible_signature_reject`
   - Added as diagnostics lane for incompatible function-pointer initializer
     signature in array table setup.

Wave75 status:
- Targeted diagnostics probe run:
  `resolved=1`, `blocked=0`, `skipped=0`.
- Targeted diagnostic-json probe run:
  `resolved=1`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=75`:
  Wave75 diagnostic test passes (`.diag` + `.diagjson`).
- Full probe lane snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=197`, `blocked=0`, `skipped=0`.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 76 Promotion Closure
270) `14__diagjson__fnptr_table_incompatible_signature_reject`
271) `14__diagjson__switch_struct_condition_reject`
272) `14__diagjson__if_struct_condition_reject`
273) `14__diagjson__while_struct_condition_reject`
274) `14__diagjson__return_struct_to_int_reject`

Wave76 status:
- Targeted diagnostic-json probe run:
  `resolved=5`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=76`:
  all 5 Wave76 diagnostics-json tests pass.

## Wave 77 Promotion Closure
275) `14__diagjson__complex_lt_reject`
276) `14__diagjson__complex_le_reject`
277) `14__diagjson__complex_gt_reject`
278) `14__diagjson__complex_ge_reject`

Wave77 status:
- Targeted diagnostic-json probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=77`:
  all 4 Wave77 diagnostics-json tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 78 Promotion Closure
279) `14__runtime_abi_reg_stack_frontier_matrix`
280) `14__runtime_abi_mixed_struct_float_boundary`
281) `14__runtime_variadic_abi_reg_stack_frontier`

Wave78 status:
- Targeted runtime probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=78`:
  all 3 Wave78 ABI frontier tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 79 Promotion Closure
282) `14__diag__multitu_duplicate_external_definition_reject`
283) `14__diag__multitu_extern_type_mismatch_reject`
284) `14__diag__multitu_duplicate_tentative_type_conflict_reject`

Wave79 status:
- Targeted diagnostics probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=79`:
  all 3 Wave79 link-conflict diagnostics tests pass.
- Note:
  Wave79 established text `.diag` link-conflict anchors; Wave89 later adds
  explicit `.diagjson` parity for the same link-failure classes.

## Wave 80 Promotion Closure
285) `14__runtime_static_local_init_once_chain`
286) `14__runtime_static_local_init_recursion_gate`
287) `14__runtime_multitu_static_init_visibility_bridge`

Wave80 status:
- Targeted runtime probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=80`:
  all 3 Wave80 static-local/re-entrancy tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 81 Promotion Closure
288) `14__runtime_vla_param_nested_handoff_matrix`
289) `14__runtime_vla_stride_rebind_cross_fn_chain`
290) `14__runtime_vla_ptrdiff_cross_scope_matrix`

Wave81 status:
- Targeted runtime probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=81`:
  all 3 Wave81 VLA depth/stress tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 82 Promotion Closure
291) `14__runtime_intptr_roundtrip_offset_matrix`
292) `14__runtime_uintptr_mask_rebase_matrix`
293) `14__runtime_ptrdiff_boundary_crosscheck_matrix`

Wave82 status:
- Targeted runtime probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=82`:
  all 3 Wave82 pointer/int bridge tests pass.
- Blocker memory (fixed before promotion):
  initial `uintptr` probe compared raw pointer low bits and mismatched clang due
  address-layout variance; fixed by reconstructing pointers from
  base-relative masked deltas (`delta = p - base`) instead of raw addresses.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 83 Promotion Closure
294) `14__runtime_long_double_variadic_struct_bridge`
295) `14__runtime_complex_long_double_bridge_matrix`
296) `14__runtime_multitu_long_double_variadic_bridge`

Wave83 status:
- Targeted runtime probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=83`:
  all 3 Wave83 long-double/complex ABI tests pass.
- Blocker memory (fixed before promotion):
  initial probe lane hit two issues:
  1) `<stdarg.h>` preprocessing failed (`invalid #if expression`) in this toolchain path.
  2) direct `va_arg(ap, struct ...)` lane crashed compiler (`exit -11`).
  Wave83 promotion was stabilized by switching to builtin vararg macros
  (`__builtin_va_list` family) and restructuring the first lane to combine
  long-double variadics with struct return bridging (without struct `va_arg`).
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 84 Promotion Closure
297) `14__runtime_volatile_restrict_store_order_matrix`
298) `14__runtime_restrict_nonoverlap_rebind_chain`
299) `14__runtime_volatile_fnptr_control_chain`

Wave84 status:
- Targeted runtime probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=84`:
  all 3 Wave84 volatile/restrict sequencing tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 85 Promotion Closure
300) `14__runtime_header_stdalign_bridge`
301) `14__runtime_header_stdint_limits_crosscheck`
302) `14__runtime_header_null_sizeof_ptrdiff_bridge`

Wave85 status:
- Targeted runtime probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=85`:
  all 3 Wave85 header/builtin expansion tests pass.
- Blocker memory (fixed before promotion):
  initial `NULL/sizeof/ptrdiff` lane used `struct-array = {0}` shorthand which
  this compiler path currently rejects in this context; lane was stabilized with
  equivalent non-initialized storage since values are unused.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 86 Promotion Closure
303) `14__runtime_smoke_expr_eval_driver`
304) `14__runtime_smoke_dispatch_table_driver`
305) `14__runtime_smoke_multitu_config_driver`

Wave86 status:
- Targeted runtime probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=86`:
  all 3 Wave86 deterministic smoke-driver tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 87 Promotion Closure
306) `14__runtime_repeatable_output_hash_matrix`
307) `14__runtime_repeatable_diagjson_hash_matrix`
308) `14__runtime_repeatable_multitu_link_hash_matrix`

Wave87 status:
- Targeted runtime probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=87`:
  all 3 Wave87 repeatability/oracle-hardening tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 88 Promotion Closure
309) `14__runtime_abi_long_double_variadic_regstack_hardening`
310) `14__runtime_multitu_static_init_order_depth_bridge`
311) `14__diag__multitu_duplicate_function_definition_reject`

Wave88 status:
- Targeted probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=88`:
  all 3 Wave88 ABI/init-order/link-diagnostic hardening tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Wave 89 Promotion Closure
312) `14__diagjson__multitu_duplicate_external_definition_reject`
313) `14__diagjson__multitu_extern_type_mismatch_reject`
314) `14__diagjson__multitu_duplicate_tentative_type_conflict_reject`
315) `14__diagjson__multitu_duplicate_function_definition_reject`

Wave89 status:
- Targeted diagnostics-json probe run:
  `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=89`:
  all 4 Wave89 link-stage diagnostics-json parity tests pass.
- Full bucket-14 slice:
  `make final-runtime` => `0 failing`, `22 skipped`.

## Planned Expansion Roadmap (Post-Wave89)

Wave89 closure complete.
Next runtime-surface wave targets are currently unassigned/TBD.

Wave execution rule:
for each wave, land probes first, gather all blocked results, fix in grouped
batch, then promote only after exact-id + `make final-wave` + `make final-runtime`
are green.

## Expected Outputs
- AST/Diagnostics/IR goldens for compile-surface checks.
- Runtime stdout/stderr/exit expectations for executable checks.
