# Runtime Bucket 14 Next-Step Plan

This is the active execution plan for `tests/final` bucket `14` (runtime
surface) after the current baseline stabilization.

Date: 2026-03-18

## Current Baseline

- Active manifests: `tests/final/meta/14-runtime-surface*.json` (6 shards)
- Active tests: 38
- Runtime+differential tests: 33
- Current targeted run status: promoted wave6 ids pass exact-id validation

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
| Arithmetic semantics | `14__runtime_div_mod_edges`, `14__runtime_unsigned_wraparound`, `14__runtime_signed_div_mod_sign_matrix` | `14__runtime_signed_overflow_ub` (UB-tagged policy lane) | `in_progress` |
| Floating semantics | `14__runtime_float_precision`, `14__runtime_float_infinity`, `14__runtime_nan_propagation`, `14__runtime_nan_comparisons`, `14__runtime_float_cast_roundtrip` | `14__runtime_signed_overflow_ub` (UB-tagged policy lane) | `wave5 stable` |
| Calls / ABI | `14__runtime_function_pointer`, `14__runtime_variadic_calls`, `14__runtime_struct_return`, `14__runtime_struct_mixed_width_pass_return`, `14__runtime_struct_nested_copy_chain`, `14__runtime_many_args_mixed_width`, `14__runtime_struct_with_array_pass_return`, `14__runtime_union_payload_roundtrip`, `14__runtime_fnptr_dispatch_table_mixed`, `14__runtime_variadic_promotions_matrix` | none | `wave6 stable` |
| Memory / layout | `14__runtime_pointer_arith`, `14__runtime_pointer_stride_long_long`, `14__runtime_struct_layout_alignment`, `14__runtime_global_partial_init_zerofill`, `14__runtime_global_designated_sparse` | `14__runtime_pointer_negative_offset`, `14__runtime_vla_stride_indexing`, `14__runtime_alignment_long_double_struct` | `wave7 planned` |
| Control flow / execution | `14__runtime_switch_simple`, `14__runtime_switch_loop_control_mix`, `14__runtime_loop_continue_break_phi`, `14__runtime_short_circuit_chain_effects`, `14__runtime_recursion`, `14__runtime_deep_recursion`, `14__runtime_large_stack` | `14__runtime_nested_switch_fallthrough_loop`, `14__runtime_short_circuit_side_effect_counter` | `wave7 planned` |
| Harness surface | `14__runtime_args_env`, `14__runtime_stdin` | none | `stable` |

Current blocked probes:

- none in active wave5/wave6 lanes (`14__probe_*` triage set is green).

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

Notes:

- Ensure control-flow tests verify side effects, not just final scalar output.
- Prefer integer-only assertions where possible for deterministic diff behavior.

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
