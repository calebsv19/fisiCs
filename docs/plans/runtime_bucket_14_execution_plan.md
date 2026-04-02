# Runtime Bucket 14 Next-Step Plan

This is the active execution plan for `tests/final` bucket `14` (runtime
surface) after the current baseline stabilization.

Date: 2026-04-01

## Current Baseline

- Active manifests: `tests/final/meta/14-runtime-surface*.json` (87 shards)
- Active tests: 329
- Runtime executables: 268 (`264` differential-flagged, including policy-lane tests)
- Policy lanes: `11` UB + `11` implementation-defined
- Negative diagnostics: 56 (`28` `.diag` + `28` `.diagjson`)
- Current targeted run status:
  - `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py` -> `resolved=234`, `blocked=0`, `skipped=0`
  - Wave42/Wave43/Wave44/Wave45/Wave46/Wave47/Wave48/Wave49/Wave50/Wave52/Wave53/Wave54/Wave55/Wave56/Wave57/Wave58/Wave59/Wave60/Wave61/Wave62/Wave63/Wave64/Wave65/Wave66/Wave67/Wave68/Wave69/Wave70/Wave71/Wave72/Wave73/Wave74/Wave75/Wave76/Wave77/Wave78/Wave79/Wave80/Wave81/Wave82/Wave83/Wave84/Wave85/Wave86/Wave87/Wave88/Wave89 exact-id checks -> all pass
  - `make final-runtime` -> `0 failing, 22 skipped`

## Objective

Expand runtime-behavior confidence from "core paths are stable" to "phase-complete
runtime surface coverage" across arithmetic, floating point, ABI boundaries,
control flow, and memory layout.

## Latest Delta (Wave 76-89 Closure Set)

- Wave66 promoted runtime tests:
  `14__runtime_complex_ptr_field_writeback_direct`,
  `14__runtime_complex_ptr_field_compound_writeback`.
- New Wave66 shard:
  `tests/final/meta/14-runtime-surface-wave66-complex-pointer-writeback-promotions.json`.
- Wave67 promoted runtime tests:
  `14__runtime_complex_ptr_field_mul_writeback`,
  `14__runtime_complex_ptr_field_div_writeback`.
- New Wave67 shard:
  `tests/final/meta/14-runtime-surface-wave67-complex-muldiv-writeback-promotions.json`.
- Wave68 promoted runtime tests:
  `14__runtime_complex_mul_div_matrix`,
  `14__runtime_complex_scalar_mul_div_chain`.
- New Wave68 shard:
  `tests/final/meta/14-runtime-surface-wave68-complex-muldiv-expression-promotions.json`.
- Wave69 promoted diagnostics:
  `14__diag__complex_lt_reject`,
  `14__diag__complex_le_reject`,
  `14__diag__complex_gt_reject`,
  `14__diag__complex_ge_reject`.
- New Wave69 shard:
  `tests/final/meta/14-runtime-surface-wave69-complex-relational-diagnostics.json`.
- Wave70 promoted diagnostics-json assertions:
  `14__diagjson__continue_outside_loop_reject`,
  `14__diagjson__break_outside_loop_reject`,
  `14__diagjson__case_outside_switch_reject`,
  `14__diagjson__default_outside_switch_reject`.
- New Wave70 shard:
  `tests/final/meta/14-runtime-surface-wave70-control-diagnostics-json.json`.
- Wave71 promoted diagnostics-json assertions:
  `14__diagjson__switch_duplicate_default_reject`,
  `14__diagjson__switch_duplicate_case_reject`,
  `14__diagjson__switch_nonconst_case_reject`,
  `14__diagjson__continue_in_switch_reject`.
- New Wave71 shard:
  `tests/final/meta/14-runtime-surface-wave71-switch-diagnostics-json.json`.
- Wave72 promoted diagnostics-json assertions:
  `14__diagjson__fnptr_table_too_many_args_reject`,
  `14__diagjson__fnptr_struct_arg_incompatible_reject`,
  `14__diagjson__fnptr_param_noncallable_reject`.
- New Wave72 shard:
  `tests/final/meta/14-runtime-surface-wave72-fnptr-diagnostics-json.json`.
- Wave73 promoted diagnostics-json assertions:
  `14__diagjson__file_scope_vla_reject`,
  `14__diagjson__offsetof_bitfield_reject`,
  `14__diagjson__ternary_struct_condition_reject`,
  `14__diagjson__fnptr_table_too_few_args_reject`.
- New Wave73 shard:
  `tests/final/meta/14-runtime-surface-wave73-diagnostics-json-expansion.json`.
- Wave74 promoted diagnostics:
  `14__diag__switch_struct_condition_reject`,
  `14__diag__if_struct_condition_reject`,
  `14__diag__while_struct_condition_reject`,
  `14__diag__return_struct_to_int_reject`.
- New Wave74 shard:
  `tests/final/meta/14-runtime-surface-wave74-struct-condition-return-diagnostics.json`.
- Wave75 promoted diagnostics:
  `14__diag__fnptr_table_incompatible_signature_reject`.
- New Wave75 shard:
  `tests/final/meta/14-runtime-surface-wave75-fnptr-incompatible-signature-diagnostics.json`.
- Wave76 promoted diagnostics-json assertions:
  `14__diagjson__fnptr_table_incompatible_signature_reject`,
  `14__diagjson__switch_struct_condition_reject`,
  `14__diagjson__if_struct_condition_reject`,
  `14__diagjson__while_struct_condition_reject`,
  `14__diagjson__return_struct_to_int_reject`.
- New Wave76 shard:
  `tests/final/meta/14-runtime-surface-wave76-diagjson-parity-struct-fnptr.json`.
- Wave77 promoted diagnostics-json assertions:
  `14__diagjson__complex_lt_reject`,
  `14__diagjson__complex_le_reject`,
  `14__diagjson__complex_gt_reject`,
  `14__diagjson__complex_ge_reject`.
- New Wave77 shard:
  `tests/final/meta/14-runtime-surface-wave77-diagjson-parity-complex-relational.json`.
- Wave78 promoted runtime ABI-frontier tests:
  `14__runtime_abi_reg_stack_frontier_matrix`,
  `14__runtime_abi_mixed_struct_float_boundary`,
  `14__runtime_variadic_abi_reg_stack_frontier`.
- New Wave78 shard:
  `tests/final/meta/14-runtime-surface-wave78-abi-reg-stack-frontier.json`.
- Wave79 promoted multi-TU diagnostics:
  `14__diag__multitu_duplicate_external_definition_reject`,
  `14__diag__multitu_extern_type_mismatch_reject`,
  `14__diag__multitu_duplicate_tentative_type_conflict_reject`.
- New Wave79 shard:
  `tests/final/meta/14-runtime-surface-wave79-multitu-linkage-conflict-diagnostics.json`.
- Wave80 promoted static-local/re-entrancy runtime tests:
  `14__runtime_static_local_init_once_chain`,
  `14__runtime_static_local_init_recursion_gate`,
  `14__runtime_multitu_static_init_visibility_bridge`.
- New Wave80 shard:
  `tests/final/meta/14-runtime-surface-wave80-static-local-init-reentrancy.json`.
- Wave81 promoted VLA parameter/depth stress tests:
  `14__runtime_vla_param_nested_handoff_matrix`,
  `14__runtime_vla_stride_rebind_cross_fn_chain`,
  `14__runtime_vla_ptrdiff_cross_scope_matrix`.
- New Wave81 shard:
  `tests/final/meta/14-runtime-surface-wave81-vla-parameter-depth-stress-ii.json`.
- Wave82 promoted pointer/int bridge bound tests:
  `14__runtime_intptr_roundtrip_offset_matrix`,
  `14__runtime_uintptr_mask_rebase_matrix`,
  `14__runtime_ptrdiff_boundary_crosscheck_matrix`.
- New Wave82 shard:
  `tests/final/meta/14-runtime-surface-wave82-pointer-int-bridge-bounds.json`.
- Wave83 promoted long-double/complex ABI tests:
  `14__runtime_long_double_variadic_struct_bridge`,
  `14__runtime_complex_long_double_bridge_matrix`,
  `14__runtime_multitu_long_double_variadic_bridge`.
- New Wave83 shard:
  `tests/final/meta/14-runtime-surface-wave83-long-double-complex-abi-cross-product.json`.
- Wave84 promoted volatile/restrict sequencing tests:
  `14__runtime_volatile_restrict_store_order_matrix`,
  `14__runtime_restrict_nonoverlap_rebind_chain`,
  `14__runtime_volatile_fnptr_control_chain`.
- New Wave84 shard:
  `tests/final/meta/14-runtime-surface-wave84-volatile-restrict-sequencing-depth.json`.
- Wave85 promoted header/builtin expansion tests:
  `14__runtime_header_stdalign_bridge`,
  `14__runtime_header_stdint_limits_crosscheck`,
  `14__runtime_header_null_sizeof_ptrdiff_bridge`.
- New Wave85 shard:
  `tests/final/meta/14-runtime-surface-wave85-header-builtin-surface-expansion.json`.
- Closure status:
  targeted probe checks pass for writeback, mul/div expression, complex-relational diagnostics, control/fnptr/layout diagnostics-json assertion lanes, and new struct-condition/return diagnostics lanes;
  `make final-wave WAVE=66`, `make final-wave WAVE=67`, `make final-wave WAVE=68`, `make final-wave WAVE=69`, `make final-wave WAVE=70`, `make final-wave WAVE=71`, `make final-wave WAVE=72`, `make final-wave WAVE=73`, `make final-wave WAVE=74`, `make final-wave WAVE=75`, `make final-wave WAVE=76`, `make final-wave WAVE=77`, `make final-wave WAVE=78`, `make final-wave WAVE=79`, `make final-wave WAVE=80`, `make final-wave WAVE=81`, `make final-wave WAVE=82`, `make final-wave WAVE=83`, `make final-wave WAVE=84`, and `make final-wave WAVE=85` pass;
  bucket slice remains green (`make final-runtime` => `0 failing, 22 skipped`).

## Hard Rules

- Keep bucket work isolated to `14` while probing/expanding.
- Add tests first, collect failures, then fix in grouped batches.
- Keep oracles deterministic (`runtime` + `differential` where safe).
- For pointer/integer bridge tests, never oracle raw pointer bit patterns;
  compare base-relative deltas/reconstructions only.
- Tag ABI-sensitive or implementation-defined tests.
- Tag UB tests and exclude them from strict differential assertions.
- No automatic expected-output rewrite outside explicit update flow.

## Behavior Groups and Test Subsets

| Group | Active Coverage (now) | Planned Subset Additions | Status |
| --- | --- | --- | --- |
| Arithmetic semantics | `14__runtime_div_mod_edges`, `14__runtime_unsigned_wraparound`, `14__runtime_signed_div_mod_sign_matrix`, `14__runtime_signed_overflow_ub_path`, `14__runtime_signed_mul_overflow_ub_path`, `14__runtime_signed_sub_overflow_ub_path`, `14__runtime_shift_count_ub_path`, `14__runtime_preinc_intmax_ub_path`, `14__runtime_predec_intmin_ub_path`, `14__runtime_add_assign_intmax_ub_path`, `14__runtime_sub_assign_intmin_ub_path` | continue UB-policy surface expansion as needed | `wave15 UB policy lane expanded` |
| Floating semantics | `14__runtime_float_precision`, `14__runtime_float_infinity`, `14__runtime_nan_propagation`, `14__runtime_nan_comparisons`, `14__runtime_float_cast_roundtrip` | `14__runtime_signed_overflow_ub` (UB-tagged policy lane) | `wave5 stable` |
| Calls / ABI | `14__runtime_function_pointer`, `14__runtime_variadic_calls`, `14__runtime_struct_return`, `14__runtime_struct_mixed_width_pass_return`, `14__runtime_struct_nested_copy_chain`, `14__runtime_many_args_mixed_width`, `14__runtime_struct_with_array_pass_return`, `14__runtime_union_payload_roundtrip`, `14__runtime_fnptr_dispatch_table_mixed`, `14__runtime_variadic_promotions_matrix`, `14__runtime_fnptr_ternary_dispatch_accumulate`, `14__runtime_switch_fnptr_dispatch_chain`, `14__runtime_fnptr_struct_pointer_pipeline`, `14__runtime_fnptr_struct_by_value_dispatch`, `14__runtime_fnptr_typedef_return_direct`, `14__runtime_fnptr_typedef_return_ternary_callee`, `14__runtime_fnptr_expression_callee_chain`, `14__runtime_multitu_struct_return_bridge`, `14__runtime_multitu_variadic_bridge`, `14__runtime_multitu_fnptr_callback_accumulate`, `14__runtime_multitu_union_struct_dispatch`, `14__runtime_multitu_vla_param_bridge`, `14__runtime_multitu_struct_array_return_chain`, `14__runtime_multitu_mixed_abi_call_chain`, `14__runtime_multitu_fnptr_struct_fold_pipeline`, `14__runtime_multitu_nested_aggregate_roundtrip`, `14__runtime_many_args_reg_stack_split_matrix`, `14__runtime_variadic_kind_fold_matrix`, `14__runtime_large_struct_return_select_chain`, `14__runtime_bitfield_pack_rewrite_matrix`, `14__runtime_multitu_large_struct_return_select`, `14__runtime_multitu_variadic_route_table`, `14__runtime_multitu_fnptr_return_ladder`, `14__runtime_multitu_vla_stride_bridge`, `14__runtime_multitu_tentative_def_zero_init_bridge`, `14__runtime_multitu_extern_array_size_consistency`, `14__runtime_multitu_static_extern_partition`, `14__runtime_multitu_global_struct_init_bridge`, `14__runtime_multitu_fnptr_qualifier_decay_bridge`, `14__runtime_multitu_array_param_decay_stride`, `14__runtime_multitu_const_volatile_ptr_pipeline`, `14__runtime_multitu_struct_with_fnptr_return_chain` | continue broadening multi-TU ABI stress with register/stack split signatures | `wave48 expanded` |
| Memory / layout | `14__runtime_pointer_arith`, `14__runtime_pointer_stride_long_long`, `14__runtime_struct_layout_alignment`, `14__runtime_global_partial_init_zerofill`, `14__runtime_global_designated_sparse`, `14__runtime_pointer_negative_offset`, `14__runtime_vla_ptrdiff_row_size_dynamic`, `14__runtime_vla_stride_indexing`, `14__runtime_alignment_long_double_struct`, `14__runtime_struct_array_byte_stride`, `14__runtime_union_embedded_alignment`, `14__runtime_vla_row_pointer_decay`, `14__runtime_struct_ptrdiff_update_matrix`, `14__runtime_vla_param_matrix_reduce`, `14__runtime_pointer_index_width_signedness`, `14__runtime_vla_param_mixed_signed_unsigned_indices`, `14__runtime_layout_stride_invariants_matrix`, `14__runtime_layout_nested_offset_consistency`, `14__runtime_layout_union_byte_rewrite_invariants`, `14__runtime_layout_vla_subslice_stride_matrix` | continue signed/unsigned index-cast edge expansions as needed | `wave46 expanded` |
| Control flow / execution | `14__runtime_switch_simple`, `14__runtime_switch_loop_control_mix`, `14__runtime_loop_continue_break_phi`, `14__runtime_short_circuit_chain_effects`, `14__runtime_nested_switch_fallthrough_loop`, `14__runtime_short_circuit_side_effect_counter`, `14__runtime_recursion`, `14__runtime_deep_recursion`, `14__runtime_large_stack`, `14__runtime_call_sequence_conditional_mix` | none | `wave16 strict lane expanded` |
| Policy lanes | `14__runtime_signed_overflow_ub_path`, `14__runtime_signed_mul_overflow_ub_path`, `14__runtime_signed_sub_overflow_ub_path`, `14__runtime_shift_count_ub_path`, `14__runtime_preinc_intmax_ub_path`, `14__runtime_predec_intmin_ub_path`, `14__runtime_add_assign_intmax_ub_path`, `14__runtime_sub_assign_intmin_ub_path`, `14__runtime_impldef_signed_right_shift`, `14__runtime_impldef_char_signedness`, `14__runtime_impldef_enum_neg_cast`, `14__runtime_impldef_plain_int_bitfield_sign`, `14__runtime_impldef_enum_size_matrix`, `14__runtime_impldef_signed_char_narrowing`, `14__runtime_impldef_long_double_size_align` | keep expanding implementation-defined and UB policy lanes | `wave20 expanded` |
| Harness surface | `14__runtime_args_env`, `14__runtime_stdin`, `14__runtime_harness_exit_stderr_nonzero_arg`, `14__runtime_harness_env_stderr_nonzero`, `14__runtime_harness_args_stderr_exit13`, `14__runtime_harness_stdin_stderr_exit17` | none | `wave42 expanded` |

Current blocked probes:

- None.
- Current probe-lane snapshot: `PROBE_FILTER=14__probe_*` => `resolved=234`, `blocked=0`, `skipped=0`.

## Wave Plan

### Wave 5: Arithmetic + Floating Runtime Gaps

Add:

- `14__runtime_unsigned_wraparound`
- `14__runtime_signed_div_mod_sign_matrix`
- `14__runtime_nan_propagation`
- `14__runtime_nan_comparisons`
- `14__runtime_float_cast_roundtrip`

Progress:

- Promoted and passing:
  `14__runtime_unsigned_wraparound`,
  `14__runtime_nan_propagation`,
  `14__runtime_signed_div_mod_sign_matrix`,
  `14__runtime_nan_comparisons`,
  `14__runtime_float_cast_roundtrip`

Notes:

- `signed overflow` stays out of strict differential correctness (UB).
- Prefer explicit deterministic checks for unsigned wrap and non-UB float paths.

### Wave 6: ABI and Call Surface Stress

Add:

- `14__runtime_many_args_mixed_width`
- `14__runtime_struct_with_array_pass_return`
- `14__runtime_union_payload_roundtrip`
- `14__runtime_fnptr_dispatch_table_mixed`
- `14__runtime_variadic_promotions_matrix`

Progress:

- Promoted and passing:
  `14__runtime_many_args_mixed_width`,
  `14__runtime_struct_with_array_pass_return`,
  `14__runtime_union_payload_roundtrip`,
  `14__runtime_fnptr_dispatch_table_mixed`,
  `14__runtime_variadic_promotions_matrix`
  (`tests/final/meta/14-runtime-surface-wave6-abi-calls.json`)

Notes:

- Mark layout/calling-convention sensitive tests with `abi_sensitive: true`.
- Keep stdout expectations compact and exact.

### Wave 7: Memory/Layout + Control-Flow Stress

Add:

- `14__runtime_pointer_negative_offset`
- `14__runtime_vla_stride_indexing`
- `14__runtime_alignment_long_double_struct`
- `14__runtime_nested_switch_fallthrough_loop`
- `14__runtime_short_circuit_side_effect_counter`

Progress:

- Promoted and passing:
  `14__runtime_pointer_negative_offset`
- Probe lane closed and resolved:
  `14__probe_vla_stride_indexing`,
  `14__probe_alignment_long_double_struct`,
  `14__probe_struct_array_byte_stride`,
  `14__probe_union_embedded_alignment`,
  `14__probe_vla_row_pointer_decay`

Notes:

- Ensure control-flow tests verify side effects, not just final scalar output.
- Prefer integer-only assertions where possible for deterministic diff behavior.

### Wave 8: Control / Pointer-Difference Promotions

Add:

- `14__runtime_nested_switch_fallthrough_loop`
- `14__runtime_short_circuit_side_effect_counter`
- `14__runtime_vla_ptrdiff_row_size_dynamic`

Progress:

- Promoted and passing:
  `14__runtime_nested_switch_fallthrough_loop`,
  `14__runtime_short_circuit_side_effect_counter`,
  `14__runtime_vla_ptrdiff_row_size_dynamic`
  (`tests/final/meta/14-runtime-surface-wave8-control-memory.json`)

### Wave 9: Probe Lane Promotions (VLA + Layout)

Add:

- `14__runtime_vla_stride_indexing`
- `14__runtime_alignment_long_double_struct`
- `14__runtime_struct_array_byte_stride`
- `14__runtime_union_embedded_alignment`
- `14__runtime_vla_row_pointer_decay`

Progress:

- Promoted and passing:
  `14__runtime_vla_stride_indexing`,
  `14__runtime_alignment_long_double_struct`,
  `14__runtime_struct_array_byte_stride`,
  `14__runtime_union_embedded_alignment`,
  `14__runtime_vla_row_pointer_decay`
  (`tests/final/meta/14-runtime-surface-wave9-vla-layout-promotions.json`)

### Wave 10: UB Policy Lane

Add:

- `14__runtime_signed_overflow_ub_path`
- `14__runtime_signed_mul_overflow_ub_path`
- `14__runtime_signed_sub_overflow_ub_path`
- `14__runtime_shift_count_ub_path`
- `14__runtime_preinc_intmax_ub_path`

Progress:

- Promoted and passing:
  `14__runtime_signed_overflow_ub_path`,
  `14__runtime_signed_mul_overflow_ub_path`,
  `14__runtime_signed_sub_overflow_ub_path`,
  `14__runtime_shift_count_ub_path`,
  `14__runtime_preinc_intmax_ub_path`
  (`tests/final/meta/14-runtime-surface-wave10-ub-policy.json`)

Notes:

- These tests are tagged with `ub: true`; runtime correctness is enforced while
  strict differential comparison is intentionally skipped by harness policy.

### Wave 11: Implementation-Defined Policy Lane

Add:

- `14__runtime_impldef_signed_right_shift`
- `14__runtime_impldef_char_signedness`
- `14__runtime_impldef_enum_neg_cast`

Progress:

- Promoted and passing:
  `14__runtime_impldef_signed_right_shift`,
  `14__runtime_impldef_char_signedness`,
  `14__runtime_impldef_enum_neg_cast`
  (`tests/final/meta/14-runtime-surface-wave11-impl-defined-policy.json`)

Notes:

- These tests are tagged with `impl_defined: true`; runtime behavior is checked
  against local expectations while strict differential comparison is
  intentionally skipped by harness policy.

### Wave 12: Strict Differential Expansion

Add:

- `14__runtime_pointer_diff_compare_matrix`
- `14__runtime_bitwise_shift_mask_matrix`

Progress:

- Promoted and passing:
  `14__runtime_pointer_diff_compare_matrix`,
  `14__runtime_bitwise_shift_mask_matrix`
  (`tests/final/meta/14-runtime-surface-wave12-strict-differential.json`)
- Added blocked probes for next fix cycle (not promoted):
  `14__probe_vla_param_matrix_reduce`,
  `14__probe_fnptr_struct_by_value_dispatch`.

### Wave 13: Strict Differential Expansion

Add:

- `14__runtime_ternary_side_effect_phi`
- `14__runtime_comma_sequence_matrix`

Progress:

- Promoted and passing:
  `14__runtime_ternary_side_effect_phi`,
  `14__runtime_comma_sequence_matrix`
  (`tests/final/meta/14-runtime-surface-wave13-strict-differential.json`)

### Wave 14: Implementation-Defined Policy Expansion

Add:

- `14__runtime_impldef_plain_int_bitfield_sign`
- `14__runtime_impldef_enum_size_matrix`

Progress:

- Promoted and passing:
  `14__runtime_impldef_plain_int_bitfield_sign`,
  `14__runtime_impldef_enum_size_matrix`
  (`tests/final/meta/14-runtime-surface-wave14-impl-defined-policy.json`)

Notes:

- These tests are tagged with `impl_defined: true`; compile/runtime behavior is
  enforced, strict differential is intentionally skipped by harness policy.

### Wave 15: UB Policy Expansion

Add:

- `14__runtime_predec_intmin_ub_path`
- `14__runtime_add_assign_intmax_ub_path`
- `14__runtime_sub_assign_intmin_ub_path`

Progress:

- Promoted and passing:
  `14__runtime_predec_intmin_ub_path`,
  `14__runtime_add_assign_intmax_ub_path`,
  `14__runtime_sub_assign_intmin_ub_path`
  (`tests/final/meta/14-runtime-surface-wave15-ub-policy.json`)

### Wave 16: Strict Differential Expansion

Add:

- `14__runtime_call_sequence_conditional_mix`

Progress:

- Promoted and passing:
  `14__runtime_call_sequence_conditional_mix`
  (`tests/final/meta/14-runtime-surface-wave16-strict-differential.json`)

### Wave 17: Strict Differential Expansion

Add:

- `14__runtime_fnptr_ternary_dispatch_accumulate`
- `14__runtime_struct_ptrdiff_update_matrix`

Progress:

- Promoted and passing:
  `14__runtime_fnptr_ternary_dispatch_accumulate`,
  `14__runtime_struct_ptrdiff_update_matrix`
  (`tests/final/meta/14-runtime-surface-wave17-strict-differential.json`)

### Wave 18: Implementation-Defined Policy Expansion

Add:

- `14__runtime_impldef_signed_char_narrowing`

Progress:

- Promoted and passing:
  `14__runtime_impldef_signed_char_narrowing`
  (`tests/final/meta/14-runtime-surface-wave18-impl-defined-policy.json`)

### Wave 19: Strict Differential Expansion

Add:

- `14__runtime_switch_fnptr_dispatch_chain`
- `14__runtime_fnptr_struct_pointer_pipeline`

Progress:

- Promoted and passing:
  `14__runtime_switch_fnptr_dispatch_chain`,
  `14__runtime_fnptr_struct_pointer_pipeline`
  (`tests/final/meta/14-runtime-surface-wave19-strict-differential.json`)

### Wave 20: Implementation-Defined Policy Expansion

Add:

- `14__runtime_impldef_long_double_size_align`

Progress:

- Promoted and passing:
  `14__runtime_impldef_long_double_size_align`
  (`tests/final/meta/14-runtime-surface-wave20-impl-defined-policy.json`)

### Wave 21: Probe Closure Promotions

Add:

- `14__runtime_vla_param_matrix_reduce`
- `14__runtime_fnptr_struct_by_value_dispatch`

Progress:

- Promoted and passing:
  `14__runtime_vla_param_matrix_reduce`,
  `14__runtime_fnptr_struct_by_value_dispatch`
  (`tests/final/meta/14-runtime-surface-wave21-vla-fnptr-promotions.json`)

### Wave 22: Probe Expansion (Promoted)

Promoted from probe lane:

- `14__runtime_fnptr_typedef_return_direct`
- `14__runtime_fnptr_typedef_return_ternary_callee`
- `14__runtime_fnptr_expression_callee_chain`
- `14__runtime_pointer_index_width_signedness`
- `14__runtime_vla_param_mixed_signed_unsigned_indices`

Outcome:

- All promoted wave22 runtime tests pass in final lane with differential checks.
- Promotion shard added:
  `tests/final/meta/14-runtime-surface-wave22-fnptr-vla-index-promotions.json`.
- Full final sweep remains green after integration (`0 failing, 15 skipped`).

### Wave 23: Probe Expansion (Closed)

Added probes:

- `14__probe_bitfield_unsigned_pack_roundtrip` (resolved)
- `14__probe_variadic_promotion_edges` (resolved)
- `14__probe_fnptr_nested_return_dispatch_matrix` (resolved)
- `14__probe_fnptr_chooser_roundtrip_call` (resolved)
- `14__probe_vla_three_dim_stride_reduce` (resolved)
- `14__probe_vla_three_dim_index_stride_basic` (resolved)

Outcome:

- Both blocker families were fixed in codegen:
  - nested chooser/function-pointer return-call signature resolution
  - 3D VLA indexing/stride lvalue lowering
- Wave23 probe lane is fully green.

### Wave 24: Probe Expansion (In Progress)

Added probes:

- `14__probe_fnptr_typedef_alias_chain_dispatch` (resolved)
- `14__probe_fnptr_chooser_table_ternary_chain` (resolved)
- `14__probe_vla_four_dim_stride_matrix` (resolved)

Outcome:

- New alias-heavy nested function-pointer call-chain probes validate against clang.
- 4D VLA stride/index probe validates against clang.
- Current wave24 additions remain in probe lane pending promotion decision.

### Wave 25: Probe Expansion (Closed)

Added probes:

- `14__probe_fnptr_struct_temporary_chain` (resolved)
- `14__probe_vla_param_slice_stride_rebase` (resolved)
- `14__probe_volatile_short_circuit_sequence` (resolved)

Outcome:

- Wave25 blocker pair was resolved in grouped fix mode:
  - semantic call analysis now recovers callable targets through typedef-named
    function-pointer chains for temporary struct-return call expressions.
  - pointer difference lowering now uses `LLVMBuildPtrDiff2`, and VLA index casts
    avoid same-width no-op cast emission.
- Full wave25 probe set is now green and ready for promotion review.

### Wave 25 Promotion (Closed)

Promoted tests:

- `14__runtime_fnptr_struct_temporary_chain`
- `14__runtime_vla_param_slice_stride_rebase`
- `14__runtime_volatile_short_circuit_sequence`

Outcome:

- Added promotion shard `14-runtime-surface-wave25-probe-promotions.json` and
  registered it in `tests/final/meta/index.json`.
- Generated expected runtime outputs via targeted `UPDATE_FINAL=1` runs.
- Verified each promoted test with targeted `FINAL_FILTER=...` runs and a full
  `python3 tests/final/run_final.py` sweep (`0 failing, 15 skipped`).

### Wave 26: Probe Expansion (Closed)

Added probes:

- `14__probe_vla_ptrdiff_subslice_rebase_chain` (resolved)
- `14__probe_fnptr_struct_array_dispatch_pipeline` (resolved)
- `14__probe_ptrdiff_char_bridge_roundtrip` (resolved)
- `14__probe_volatile_comma_ternary_control_chain` (resolved)
- `14__probe_variadic_width_stress_matrix` (resolved)

Outcome:

- New VLA subslice-rebase, function-pointer struct dispatch pipeline,
  pointer-diff byte bridge, volatile control-chain, and variadic width matrix
  probes all match clang.
- `14__probe_variadic_width_stress_matrix` was fixed by updating codegen
  named-alias lowering so `__builtin_va_list` maps to pointer-form type
  lowering, eliminating backend crash in `-o` compile flow.

### Wave 26 Promotion (Closed)

Promoted tests:

- `14__runtime_vla_ptrdiff_subslice_rebase_chain`
- `14__runtime_fnptr_struct_array_dispatch_pipeline`
- `14__runtime_ptrdiff_char_bridge_roundtrip`
- `14__runtime_volatile_comma_ternary_control_chain`

Outcome:

- Added promotion shard `14-runtime-surface-wave26-probe-promotions.json` and
  registered it in `tests/final/meta/index.json`.
- Generated expected runtime outputs via targeted `UPDATE_FINAL=1` runs.
- Validated each promoted id via `FINAL_FILTER=...` and full `make final`
  (`0 failing, 15 skipped`).

### Wave 27: Broad Probe Expansion (Closed + Promoted)

Added probes:

- `14__probe_variadic_vacopy_forwarder_matrix` (resolved)
- `14__probe_variadic_fnptr_dispatch_chain` (resolved)
- `14__probe_variadic_nested_forwarder_table` (resolved)
- `14__probe_struct_large_return_pipeline` (resolved)
- `14__probe_struct_large_return_fnptr_pipeline` (resolved)
- `14__probe_vla_param_negative_ptrdiff_matrix` (resolved)
- `14__probe_vla_rebased_slice_signed_unsigned_mix` (resolved)
- `14__probe_ptrdiff_struct_char_bridge_matrix` (resolved)
- `14__probe_fnptr_stateful_table_loop_matrix` (resolved)
- `14__probe_struct_union_by_value_roundtrip_chain` (resolved)
- `14__probe_fnptr_return_struct_pipeline` (resolved)
- `14__probe_vla_param_cross_function_pipeline` (resolved)
- `14__probe_ptrdiff_reinterpret_longlong_bridge` (resolved)
- `14__probe_recursive_fnptr_mix_runtime` (resolved)

Grouped fix outcomes:

- Added explicit codegen lowering for `__builtin_va_copy`.
- Preserved typedef surface pointer derivations during codegen type resolution.
- Normalized qualifier/VLA-like array compatibility in semantic assignment checks.
- Corrected union LLVM storage/body materialization for by-value ABI semantics.
- Probe snapshot:
  `PROBE_FILTER=14__*` => `resolved=55`, `blocked=0`, `skipped=0`.

Promoted from wave27 probes:

- `14__runtime_variadic_vacopy_forwarder_matrix`
- `14__runtime_variadic_nested_forwarder_table`
- `14__runtime_ptrdiff_struct_char_bridge_matrix`
- `14__runtime_struct_union_by_value_roundtrip_chain`
- `14__runtime_vla_param_cross_function_pipeline`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave27-probe-promotions.json`

### Wave 28: Additional Resolved Probe Promotions (Staging Pass)

Promoted resolved probe-only tests that were still missing from active suite:

- `14__runtime_variadic_width_stress_matrix`
- `14__runtime_variadic_fnptr_dispatch_chain`
- `14__runtime_vla_param_negative_ptrdiff_matrix`
- `14__runtime_vla_rebased_slice_signed_unsigned_mix`
- `14__runtime_fnptr_stateful_table_loop_matrix`
- `14__runtime_fnptr_return_struct_pipeline`
- `14__runtime_ptrdiff_reinterpret_longlong_bridge`
- `14__runtime_recursive_fnptr_mix_runtime`
- `14__runtime_struct_large_return_pipeline`
- `14__runtime_struct_large_return_fnptr_pipeline`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave28-probe-promotions.json`

### Wave 29: Bitfield Probe Promotion (Closed)

Promoted from probe lane:

- `14__runtime_bitfield_unsigned_pack_roundtrip`
- `14__runtime_bitfield_truncation_struct_flow`
- `14__runtime_bitfield_unsigned_mask_merge_chain`
- `14__runtime_bitfield_by_value_roundtrip_simple`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave29-bitfield-promotions.json`

Validation:

- Exact-id checks pass for all 4 promoted ids via `FINAL_FILTER=<id>`.
- Full suite remains green: `make final` => `0 failing, 15 skipped`.

### Wave 30: Function-Pointer Alias Promotions (Closed)

Promoted from probe lane:

- `14__runtime_fnptr_alias_chooser_struct_chain`
- `14__runtime_fnptr_alias_conditional_factory_simple`
- `14__runtime_fnptr_alias_indirect_dispatch`
- `14__runtime_fnptr_typedef_alias_chain_dispatch`
- `14__runtime_fnptr_chooser_roundtrip_call`
- `14__runtime_fnptr_chooser_table_ternary_chain`
- `14__runtime_fnptr_nested_return_dispatch_matrix`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave30-fnptr-alias-promotions.json`

Validation:

- Exact-id checks pass for all 7 promoted ids via `FINAL_FILTER=<id>`.
- Full suite remains green: `make final` => `0 failing, 15 skipped`.

### Wave 31: Variadic Promotion Edge Promotions (Closed)

Promoted from probe lane:

- `14__runtime_variadic_promotion_edges`
- `14__runtime_variadic_promote_char_short_float_mix`
- `14__runtime_variadic_signed_unsigned_char_matrix`
- `14__runtime_variadic_small_types_forward_chain`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave31-variadic-edge-promotions.json`

Validation:

- Exact-id checks pass for all 4 promoted ids via `FINAL_FILTER=<id>`.
- Full suite remains green: `make final` => `0 failing, 15 skipped`.

### Wave 32: 3D/4D VLA Promotion Set (Closed)

Promoted from probe lane:

- `14__runtime_vla_three_dim_index_stride_basic`
- `14__runtime_vla_three_dim_stride_reduce`
- `14__runtime_vla_four_dim_stride_matrix`
- `14__runtime_vla_four_dim_slice_rebase_runtime`
- `14__runtime_vla_four_dim_param_handoff_reduce`
- `14__runtime_vla_four_dim_rebase_weighted_reduce`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave32-vla-depth-promotions.json`

Validation:

- Exact-id checks pass for all 6 promoted ids via `FINAL_FILTER=<id>`.
- Full suite remains green: `make final` => `0 failing, 15 skipped`.

### Wave 33: Numeric Boundary Matrix Promotions (Closed)

Promoted from probe lane:

- `14__runtime_unsigned_div_mod_extremes_matrix`
- `14__runtime_signed_unsigned_cmp_boundary_matrix`
- `14__runtime_float_signed_zero_inf_matrix`
- `14__runtime_cast_chain_width_sign_matrix`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave33-boundary-promotions.json`

Validation:

- Targeted probe run: `resolved=4`, `blocked=0`, `skipped=0`.
- Exact-id checks pass for all 4 promoted ids.
- Full probe-lane snapshot: `resolved=87`, `blocked=0`, `skipped=0`.

### Wave 34: Header Surface Promotions + Fix (Closed)

Promoted from probe lane:

- `14__runtime_header_stddef_ptrdiff_size_t_bridge`
- `14__runtime_header_stdint_intptr_uintptr_roundtrip`
- `14__runtime_header_limits_llong_matrix`
- `14__runtime_header_stdbool_int_bridge`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave34-header-promotions.json`

Wave blocker fixed:

- `_Bool` cast lowering bug fixed in `cg_cast_value`:
  integer/float/pointer to `_Bool` now lowers via truthiness (`!= 0`)
  instead of integer truncation.

Validation:

- Targeted probe run: `resolved=4`, `blocked=0`, `skipped=0`.
- Exact-id checks pass for all 4 promoted ids.

### Wave 38: Control/VLA/ABI Probe Promotions (Closed)

Promoted from probe lane:

- `14__runtime_control_do_while_switch_phi_chain`
- `14__runtime_struct_array_double_by_value_roundtrip`
- `14__runtime_vla_dim_recompute_stride_matrix`
- `14__runtime_pointer_byte_rebase_roundtrip_matrix`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave38-control-vla-abi-promotions.json`

Validation:

- Targeted probe run: `resolved=4`, `blocked=0`, `skipped=0`.
- Exact-id checks pass for all 4 promoted ids.

### Wave 35: ABI Edge Promotions (Closed)

Promoted from probe lane:

- `14__runtime_struct_bitfield_mixed_pass_return`
- `14__runtime_struct_double_int_padding_roundtrip`
- `14__runtime_fnptr_variadic_dispatch_bridge`
- `14__runtime_many_args_struct_scalar_mix`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave35-abi-promotions.json`

Validation:

- Targeted probe run: `resolved=4`, `blocked=0`, `skipped=0`.
- Exact-id checks pass for all 4 promoted ids.

### Wave 36: Policy-Lane Expansion (Closed)

Added directly to active policy-lane coverage:

- `14__runtime_impldef_plain_int_bitfield_signedness_matrix` (`impl_defined: true`)
- `14__runtime_impldef_signed_shift_char_matrix` (`impl_defined: true`)
- `14__runtime_ub_left_shift_negative_lhs_path` (`ub: true`)
- `14__runtime_ub_negate_intmin_path` (`ub: true`)

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave36-policy-lanes.json`

Validation:

- Exact-id checks pass for all 4 tests.
- Policy-lane differential skips are expected and explicit.
- Full suite remains green: `make final` => `0 failing, 19 skipped`.

### Wave 37: Float/Pointer/ABI Probe Promotions (Closed)

Promoted from probe lane:

- `14__runtime_float_negzero_propagation_chain`
- `14__runtime_ptrdiff_one_past_end_matrix`
- `14__runtime_many_args_float_int_struct_mix`
- `14__runtime_variadic_llong_double_bridge`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave37-float-pointer-abi-promotions.json`

Validation:

- Targeted probe run: `resolved=4`, `blocked=0`, `skipped=0`.
- Exact-id checks pass for all 4 promoted ids.

### Wave 49: Layout Policy + Edge Promotions (Closed)

Promoted in this wave:

- `14__runtime_layout_bitfield_container_boundary_matrix` (`impl_defined: true`)
- `14__runtime_layout_enum_underlying_width_bridge` (`impl_defined: true`)
- `14__runtime_layout_ptrdiff_large_span_matrix`
- `14__runtime_layout_nested_vla_rowcol_stride_bridge`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave49-layout-policy-edge-promotions.json`

Validation:

- `make final-wave WAVE=49` => all 4 pass (`2` policy skips expected).
- `make final-runtime` => `0 failing`, `21 skipped`.
- `make final` => `0 failing`, `21 skipped`.

### Wave 50: Bool/Bitfield Probe Promotions (Closed)

Promoted in this wave:

- `14__runtime_bool_scalar_conversion_matrix`
- `14__runtime_bitfield_signed_promotion_matrix`
- `14__runtime_bitfield_compound_assign_bridge`
- `14__runtime_bitfield_bool_bridge_matrix`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave50-bool-bitfield-promotions.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_bool_scalar_conversion_matrix,14__probe_bitfield_signed_promotion_matrix,14__probe_bitfield_compound_assign_bridge,14__probe_bitfield_bool_bridge_matrix`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=99`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=50` => all 4 pass.
- `make final-runtime` => `0 failing`, `21 skipped`.
- `make final` => `0 failing`, `21 skipped`.

### Wave 52: Multi-TU ABI Depth Promotions (Closed)

Promoted in this wave:

- `14__runtime_multitu_fnptr_table_state_bridge`
- `14__runtime_multitu_variadic_fold_bridge`
- `14__runtime_multitu_vla_window_reduce_bridge`
- `14__runtime_multitu_struct_union_route_bridge`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave52-multitu-abi-depth-promotions.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_multitu_fnptr_table_state_bridge,14__probe_multitu_variadic_fold_bridge,14__probe_multitu_vla_window_reduce_bridge,14__probe_multitu_struct_union_route_bridge`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=52` => all 4 pass.

### Wave 53: Strict Layout/Control Promotions (Closed)

Promoted in this wave:

- `14__runtime_control_switch_do_for_state_machine`
- `14__runtime_layout_offset_stride_bridge`
- `14__runtime_pointer_stride_rebase_control_mix`
- `14__runtime_bool_bitmask_control_chain`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave53-layout-control-strict-promotions.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_control_switch_do_for_state_machine,14__probe_layout_offset_stride_bridge,14__probe_pointer_stride_rebase_control_mix,14__probe_bool_bitmask_control_chain`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=107`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=53` => all 4 pass.
- `make final-runtime` => `0 failing`, `21 skipped`.
- `make final` => `0 failing`, `21 skipped`.

### Wave 54: Control/VLA/FnPtr/Ptrdiff Promotions (Closed)

Promoted in this wave:

- `14__runtime_control_goto_cleanup_counter_matrix`
- `14__runtime_vla_sizeof_rebind_stride_matrix`
- `14__runtime_fnptr_array_ternary_reseed_matrix`
- `14__runtime_ptrdiff_signed_index_rebase_matrix`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave54-control-vla-fnptr-ptrdiff-promotions.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_control_goto_cleanup_counter_matrix,14__probe_vla_sizeof_rebind_stride_matrix,14__probe_fnptr_array_ternary_reseed_matrix,14__probe_ptrdiff_signed_index_rebase_matrix`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=54` => all 4 pass.

### Wave 55: Control Diagnostics Hardening Promotions (Closed)

Promoted in this wave:

- `14__diag__continue_outside_loop_reject`
- `14__diag__break_outside_loop_reject`
- `14__diag__case_outside_switch_reject`
- `14__diag__default_outside_switch_reject`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave55-diagnostics-control-hardening-promotions.json`

Validation:

- Targeted diagnostics probe run:
  `PROBE_FILTER=14__probe_diag_continue_outside_loop_reject,14__probe_diag_break_outside_loop_reject,14__probe_diag_case_outside_switch_reject,14__probe_diag_default_outside_switch_reject`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=119`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=55` => all 4 pass.
- `make final-runtime` => `0 failing`, `21 skipped`.

### Wave 56: Multi-TU Storage + VLA/FnPtr Promotions (Closed)

Promoted in this wave:

- `14__runtime_multitu_static_local_counter_bridge`
- `14__runtime_multitu_static_storage_slice_bridge`
- `14__runtime_vla_row_pointer_handoff_rebase_matrix`
- `14__runtime_fnptr_struct_state_reseed_loop_matrix`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave56-multitu-vla-fnptr-promotions.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_multitu_static_local_counter_bridge,14__probe_multitu_static_storage_slice_bridge,14__probe_vla_row_pointer_handoff_rebase_matrix,14__probe_fnptr_struct_state_reseed_loop_matrix`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=123`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=56` => all 4 pass.
- `make final-runtime` => `0 failing`, `21 skipped`.

### Wave 57: Diagnostics Switch/Control Hardening (Closed)

Promoted in this wave:

- `14__diag__switch_duplicate_default_reject`
- `14__diag__switch_duplicate_case_reject`
- `14__diag__switch_nonconst_case_reject`
- `14__diag__continue_in_switch_reject`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave57-diagnostics-switch-control-hardening.json`

Validation:

- Targeted diagnostics probe run:
  `PROBE_FILTER=14__probe_diag_switch_duplicate_default_reject,14__probe_diag_switch_duplicate_case_reject,14__probe_diag_switch_nonconst_case_reject,14__probe_diag_continue_in_switch_reject`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=127`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=57` => all 4 pass.
- `make final-runtime` => `0 failing`, `21 skipped`.

### Wave 58: Runtime ABI/VLA/Control Promotions (Closed)

Promoted in this wave:

- `14__runtime_multitu_fnptr_rotation_bridge`
- `14__runtime_multitu_pair_fold_bridge`
- `14__runtime_vla_column_pointer_stride_mix`
- `14__runtime_switch_goto_reentry_matrix`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave58-runtime-abi-vla-control.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_multitu_fnptr_rotation_bridge,14__probe_multitu_pair_fold_bridge,14__probe_vla_column_pointer_stride_mix,14__probe_switch_goto_reentry_matrix`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=131`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=58` => all 4 pass.
- `make final-runtime` => `0 failing`, `21 skipped`.
- `make final` => `0 failing`, `21 skipped`.

### Wave 59: Restrict/Alias Policy Promotions (Closed)

Promoted in this wave:

- `14__runtime_restrict_nonoverlap_accumulate_matrix`
- `14__runtime_restrict_vla_window_stride_matrix`
- `14__runtime_multitu_restrict_bridge`
- `14__runtime_restrict_alias_overlap_ub_path` (`ub: true`)

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave59-restrict-alias-policy.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_restrict_nonoverlap_accumulate_matrix,14__probe_restrict_vla_window_stride_matrix,14__probe_multitu_restrict_bridge,14__probe_restrict_alias_overlap_ub_path`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=59` => `0 failing`, `1 skipped` (expected UB-policy skip).
- `make final-runtime` => `0 failing`, `22 skipped`.

### Wave 60: Long-Double ABI Promotions (Closed)

Promoted in this wave:

- `14__runtime_long_double_call_chain_matrix`
- `14__runtime_long_double_variadic_bridge`
- `14__runtime_multitu_long_double_bridge`
- `14__runtime_long_double_struct_pass_return`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave60-long-double-abi-promotions.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_long_double_call_chain_matrix,14__probe_long_double_variadic_bridge,14__probe_multitu_long_double_bridge,14__probe_long_double_struct_pass_return`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=60` => all 4 pass.
- `make final-runtime` => `0 failing`, `22 skipped`.
- `make final` => `0 failing`, `22 skipped`.

### Wave 61: Complex Runtime Promotions (Closed)

Promoted in this wave:

- `14__runtime_complex_addsub_eq_matrix`
- `14__runtime_complex_unary_scalar_promotion_chain`
- `14__runtime_complex_param_return_bridge`
- `14__runtime_multitu_complex_bridge`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave61-complex-runtime-promotions.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_complex_addsub_eq_matrix,14__probe_complex_unary_scalar_promotion_chain,14__probe_complex_param_return_bridge,14__probe_multitu_complex_bridge`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=61` => all 4 pass.

### Wave 62: FnPtr Diagnostics Hardening (Closed)

Promoted in this wave:

- `14__diag__fnptr_table_too_many_args_reject`
- `14__diag__fnptr_struct_arg_incompatible_reject`
- `14__diag__fnptr_param_noncallable_reject`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave62-fnptr-diagnostics-hardening.json`

Validation:

- Targeted diagnostics probe run:
  `PROBE_FILTER=14__probe_diag_fnptr_table_too_many_args_reject,14__probe_diag_fnptr_struct_arg_incompatible_reject,14__probe_diag_fnptr_param_noncallable_reject`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=62` => all 3 pass.
- Full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=146`, `blocked=0`, `skipped=0`.
- `make final-runtime` => `0 failing`, `22 skipped`.

### Wave 63: Complex Struct Pass/Return Promotions (Closed)

Promoted in this wave:

- `14__runtime_complex_struct_pass_return_bridge`
- `14__runtime_complex_struct_array_pass_return_chain`
- `14__runtime_complex_nested_struct_field_bridge`
- `14__runtime_complex_struct_ternary_select_bridge`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave63-complex-struct-pass-return-promotions.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_complex_struct_pass_return_bridge,14__probe_complex_struct_array_pass_return_chain,14__probe_complex_nested_struct_field_bridge,14__probe_complex_struct_ternary_select_bridge`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=63` => all 4 pass.

### Wave 64: Complex Struct Multi-TU/Pointer/Copy Promotions (Closed)

Promoted in this wave:

- `14__runtime_multitu_complex_struct_pass_return`
- `14__runtime_multitu_complex_nested_route`
- `14__runtime_complex_struct_pointer_roundtrip_bridge`
- `14__runtime_complex_struct_copy_assign_chain`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave64-complex-struct-multitu-pointer-copy-promotions.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_multitu_complex_struct_pass_return,14__probe_multitu_complex_nested_route,14__probe_complex_struct_pointer_roundtrip_bridge,14__probe_complex_struct_copy_assign_chain`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=64` => all 4 pass.

### Wave 65: Complex Layout Promotions (Closed)

Promoted in this wave:

- `14__runtime_complex_layout_size_align_matrix`
- `14__runtime_complex_layout_nested_offset_matrix`
- `14__runtime_complex_layout_array_stride_matrix`
- `14__runtime_complex_layout_union_overlay_roundtrip`

Promotion artifact:

- `tests/final/meta/14-runtime-surface-wave65-complex-layout-promotions.json`

Validation:

- Targeted wave probe run:
  `PROBE_FILTER=14__probe_complex_layout_size_align_matrix,14__probe_complex_layout_nested_offset_matrix,14__probe_complex_layout_array_stride_matrix,14__probe_complex_layout_union_overlay_roundtrip`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-wave WAVE=65` => all 4 pass.
- Full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_*` => `resolved=158`, `blocked=0`, `skipped=0`.
- `make final-runtime` => `0 failing`, `22 skipped`.

## Execution Flow Per Wave

1. Add cases under `tests/final/cases/` with stable ids.
2. Add metadata into a new wave shard file:
   `tests/final/meta/14-runtime-surface-waveN-<topic>.json`
3. Run targeted pass with exact-id filters:
   `FINAL_FILTER=<test_id> python3 tests/final/run_final.py`
4. If blocked, log in `docs/compiler_behavior_coverage_run_log.md` and do not
   promote probe-only repros yet.
5. After fix, promote into active metadata and regenerate only required
   expectations.
6. Re-run full bucket as an exact-id sweep across all `14` manifest tests.

## Completion Criteria For Bucket 14

- All active `14__*` tests pass.
- No untriaged blocked runtime probes remain.
- Core runtime categories have valid + edge coverage:
  arithmetic, floating point, control flow, memory/layout, calls/ABI.
- UB and implementation-defined cases are explicitly tagged and policy-gated.

## Remaining Optional Backlog (Post-Wave89)

- No closure blockers remain for bucket-14 at current baseline.
- Link-stage diagnostics-json parity is now active for multi-TU link failures.
- Optional future hardening:
  - direct `va_arg(ap, struct ...)` support path (currently unsupported in this lane),
  - brittle struct-array `{0}` shorthand initializer corner paths,
  - additional ABI frontier breadth beyond current reg/stack/long-double/variadic set,
  - additional startup/init-order stress depth if needed by external corpus workloads.

## Wave 76-88 Closure Snapshot

- Wave76 promoted diagnostics-json lanes:
  `14__diagjson__fnptr_table_incompatible_signature_reject`,
  `14__diagjson__switch_struct_condition_reject`,
  `14__diagjson__if_struct_condition_reject`,
  `14__diagjson__while_struct_condition_reject`,
  `14__diagjson__return_struct_to_int_reject`.
- Wave77 promoted diagnostics-json lanes:
  `14__diagjson__complex_lt_reject`,
  `14__diagjson__complex_le_reject`,
  `14__diagjson__complex_gt_reject`,
  `14__diagjson__complex_ge_reject`.
- Wave78 promoted runtime ABI-frontier lanes:
  `14__runtime_abi_reg_stack_frontier_matrix`,
  `14__runtime_abi_mixed_struct_float_boundary`,
  `14__runtime_variadic_abi_reg_stack_frontier`.
- Wave79 promoted multi-TU link-conflict diagnostics lanes:
  `14__diag__multitu_duplicate_external_definition_reject`,
  `14__diag__multitu_extern_type_mismatch_reject`,
  `14__diag__multitu_duplicate_tentative_type_conflict_reject`.
- Wave80 promoted static-local/re-entrancy runtime lanes:
  `14__runtime_static_local_init_once_chain`,
  `14__runtime_static_local_init_recursion_gate`,
  `14__runtime_multitu_static_init_visibility_bridge`.
- Wave81 promoted VLA depth/stress runtime lanes:
  `14__runtime_vla_param_nested_handoff_matrix`,
  `14__runtime_vla_stride_rebind_cross_fn_chain`,
  `14__runtime_vla_ptrdiff_cross_scope_matrix`.
- Wave82 promoted pointer/int bridge runtime lanes:
  `14__runtime_intptr_roundtrip_offset_matrix`,
  `14__runtime_uintptr_mask_rebase_matrix`,
  `14__runtime_ptrdiff_boundary_crosscheck_matrix`.
- Wave83 promoted long-double/complex ABI runtime lanes:
  `14__runtime_long_double_variadic_struct_bridge`,
  `14__runtime_complex_long_double_bridge_matrix`,
  `14__runtime_multitu_long_double_variadic_bridge`.
- Wave84 promoted volatile/restrict sequencing runtime lanes:
  `14__runtime_volatile_restrict_store_order_matrix`,
  `14__runtime_restrict_nonoverlap_rebind_chain`,
  `14__runtime_volatile_fnptr_control_chain`.
- Wave85 promoted header/builtin runtime lanes:
  `14__runtime_header_stdalign_bridge`,
  `14__runtime_header_stdint_limits_crosscheck`,
  `14__runtime_header_null_sizeof_ptrdiff_bridge`.
- Wave86 promoted deterministic smoke-driver runtime lanes:
  `14__runtime_smoke_expr_eval_driver`,
  `14__runtime_smoke_dispatch_table_driver`,
  `14__runtime_smoke_multitu_config_driver`.
- Wave87 promoted repeatability/oracle-hardening runtime lanes:
  `14__runtime_repeatable_output_hash_matrix`,
  `14__runtime_repeatable_diagjson_hash_matrix`,
  `14__runtime_repeatable_multitu_link_hash_matrix`.
- Wave88 promoted ABI/init-order/link-diagnostic hardening lanes:
  `14__runtime_abi_long_double_variadic_regstack_hardening`,
  `14__runtime_multitu_static_init_order_depth_bridge`,
  `14__diag__multitu_duplicate_function_definition_reject`.
- Wave89 promoted link-stage diagnostics-json parity lanes:
  `14__diagjson__multitu_duplicate_external_definition_reject`,
  `14__diagjson__multitu_extern_type_mismatch_reject`,
  `14__diagjson__multitu_duplicate_tentative_type_conflict_reject`,
  `14__diagjson__multitu_duplicate_function_definition_reject`.
- Validation:
  targeted diagjson probes `resolved=9`, `blocked=0`, `skipped=0`;
  targeted Wave78 runtime probes `resolved=3`, `blocked=0`, `skipped=0`;
  targeted Wave79 diagnostics probes `resolved=3`, `blocked=0`, `skipped=0`;
  targeted Wave80 runtime probes `resolved=3`, `blocked=0`, `skipped=0`;
  targeted Wave81 runtime probes `resolved=3`, `blocked=0`, `skipped=0`;
  targeted Wave82 runtime probes `resolved=3`, `blocked=0`, `skipped=0`;
  targeted Wave83 runtime probes `resolved=3`, `blocked=0`, `skipped=0`;
  targeted Wave84 runtime probes `resolved=3`, `blocked=0`, `skipped=0`;
  targeted Wave85 runtime probes `resolved=3`, `blocked=0`, `skipped=0`;
  targeted Wave86 runtime probes `resolved=3`, `blocked=0`, `skipped=0`;
  targeted Wave87 runtime probes `resolved=3`, `blocked=0`, `skipped=0`;
  targeted Wave88 probes `resolved=3`, `blocked=0`, `skipped=0`;
  targeted Wave89 diagnostics-json probes `resolved=4`, `blocked=0`, `skipped=0`;
  `make final-wave WAVE=76` pass;
  `make final-wave WAVE=77` pass;
  `make final-wave WAVE=78` pass;
  `make final-wave WAVE=79` pass;
  `make final-wave WAVE=80` pass;
  `make final-wave WAVE=81` pass;
  `make final-wave WAVE=82` pass;
  `make final-wave WAVE=83` pass;
  `make final-wave WAVE=84` pass;
  `make final-wave WAVE=85` pass;
  `make final-wave WAVE=86` pass;
  `make final-wave WAVE=87` pass;
  `make final-wave WAVE=88` pass;
  `make final-wave WAVE=89` pass;
  `make final-runtime` remains `0 failing`, `22 skipped`.

## Authoritative Next Waves (Post-Wave89)

This roadmap is the active forward plan. Historical wave sections above are kept
as execution history.

Wave89 closure complete.
Next runtime-surface wave targets are currently unassigned/TBD.
