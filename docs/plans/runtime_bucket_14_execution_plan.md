# Runtime Bucket 14 Next-Step Plan

This is the active execution plan for `tests/final` bucket `14` (runtime
surface) after the current baseline stabilization.

Date: 2026-03-21

## Current Baseline

- Active manifests: `tests/final/meta/14-runtime-surface*.json` (23 shards)
- Active tests: 85
- Runtime+differential tests: 80 (`65` strict + `8` UB policy-lane + `7` implementation-defined policy-lane)
- Current targeted run status: full `python3 tests/final/run_final.py` sweep is green
  with wave22 promotion integrated (`0 failing, 15 skipped`).

## Objective

Expand runtime-behavior confidence from "core paths are stable" to "phase-complete
runtime surface coverage" across arithmetic, floating point, ABI boundaries,
control flow, and memory layout.

## Hard Rules

- Keep bucket work isolated to `14` while probing/expanding.
- Add tests first, collect failures, then fix in grouped batches.
- Keep oracles deterministic (`runtime` + `differential` where safe).
- Tag ABI-sensitive or implementation-defined tests.
- Tag UB tests and exclude them from strict differential assertions.
- No automatic expected-output rewrite outside explicit update flow.

## Behavior Groups and Test Subsets

| Group | Active Coverage (now) | Planned Subset Additions | Status |
| --- | --- | --- | --- |
| Arithmetic semantics | `14__runtime_div_mod_edges`, `14__runtime_unsigned_wraparound`, `14__runtime_signed_div_mod_sign_matrix`, `14__runtime_signed_overflow_ub_path`, `14__runtime_signed_mul_overflow_ub_path`, `14__runtime_signed_sub_overflow_ub_path`, `14__runtime_shift_count_ub_path`, `14__runtime_preinc_intmax_ub_path`, `14__runtime_predec_intmin_ub_path`, `14__runtime_add_assign_intmax_ub_path`, `14__runtime_sub_assign_intmin_ub_path` | continue UB-policy surface expansion as needed | `wave15 UB policy lane expanded` |
| Floating semantics | `14__runtime_float_precision`, `14__runtime_float_infinity`, `14__runtime_nan_propagation`, `14__runtime_nan_comparisons`, `14__runtime_float_cast_roundtrip` | `14__runtime_signed_overflow_ub` (UB-tagged policy lane) | `wave5 stable` |
| Calls / ABI | `14__runtime_function_pointer`, `14__runtime_variadic_calls`, `14__runtime_struct_return`, `14__runtime_struct_mixed_width_pass_return`, `14__runtime_struct_nested_copy_chain`, `14__runtime_many_args_mixed_width`, `14__runtime_struct_with_array_pass_return`, `14__runtime_union_payload_roundtrip`, `14__runtime_fnptr_dispatch_table_mixed`, `14__runtime_variadic_promotions_matrix`, `14__runtime_fnptr_ternary_dispatch_accumulate`, `14__runtime_switch_fnptr_dispatch_chain`, `14__runtime_fnptr_struct_pointer_pipeline`, `14__runtime_fnptr_struct_by_value_dispatch`, `14__runtime_fnptr_typedef_return_direct`, `14__runtime_fnptr_typedef_return_ternary_callee`, `14__runtime_fnptr_expression_callee_chain` | continue broadening expression-callee function-pointer matrix as needed | `wave22 promoted` |
| Memory / layout | `14__runtime_pointer_arith`, `14__runtime_pointer_stride_long_long`, `14__runtime_struct_layout_alignment`, `14__runtime_global_partial_init_zerofill`, `14__runtime_global_designated_sparse`, `14__runtime_pointer_negative_offset`, `14__runtime_vla_ptrdiff_row_size_dynamic`, `14__runtime_vla_stride_indexing`, `14__runtime_alignment_long_double_struct`, `14__runtime_struct_array_byte_stride`, `14__runtime_union_embedded_alignment`, `14__runtime_vla_row_pointer_decay`, `14__runtime_struct_ptrdiff_update_matrix`, `14__runtime_vla_param_matrix_reduce`, `14__runtime_pointer_index_width_signedness`, `14__runtime_vla_param_mixed_signed_unsigned_indices` | continue signed/unsigned index-cast edge expansions as needed | `wave22 promoted` |
| Control flow / execution | `14__runtime_switch_simple`, `14__runtime_switch_loop_control_mix`, `14__runtime_loop_continue_break_phi`, `14__runtime_short_circuit_chain_effects`, `14__runtime_nested_switch_fallthrough_loop`, `14__runtime_short_circuit_side_effect_counter`, `14__runtime_recursion`, `14__runtime_deep_recursion`, `14__runtime_large_stack`, `14__runtime_call_sequence_conditional_mix` | none | `wave16 strict lane expanded` |
| Policy lanes | `14__runtime_signed_overflow_ub_path`, `14__runtime_signed_mul_overflow_ub_path`, `14__runtime_signed_sub_overflow_ub_path`, `14__runtime_shift_count_ub_path`, `14__runtime_preinc_intmax_ub_path`, `14__runtime_predec_intmin_ub_path`, `14__runtime_add_assign_intmax_ub_path`, `14__runtime_sub_assign_intmin_ub_path`, `14__runtime_impldef_signed_right_shift`, `14__runtime_impldef_char_signedness`, `14__runtime_impldef_enum_neg_cast`, `14__runtime_impldef_plain_int_bitfield_sign`, `14__runtime_impldef_enum_size_matrix`, `14__runtime_impldef_signed_char_narrowing`, `14__runtime_impldef_long_double_size_align` | keep expanding implementation-defined and UB policy lanes | `wave20 expanded` |
| Harness surface | `14__runtime_args_env`, `14__runtime_stdin` | none | `stable` |

Current blocked probes:

- `14__probe_variadic_vacopy_forwarder_matrix`
  - unresolved `___builtin_va_copy` symbol at link.
- `14__probe_variadic_nested_forwarder_table`
  - unresolved `___builtin_va_copy` symbol at link.
- `14__probe_struct_large_return_pipeline`
  - runtime mismatch vs clang.
- `14__probe_struct_large_return_fnptr_pipeline`
  - runtime mismatch vs clang.
- `14__probe_ptrdiff_struct_char_bridge_matrix`
  - runtime mismatch vs clang (`typed ptrdiff` corruption on struct pointers).
- `14__probe_struct_union_by_value_roundtrip_chain`
  - runtime mismatch vs clang.
- `14__probe_vla_param_cross_function_pipeline`
  - semantic mismatch on VLA argument compatibility.
- Current probe-lane snapshot: `PROBE_FILTER=14__*` => `resolved=48`, `blocked=7`.

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
