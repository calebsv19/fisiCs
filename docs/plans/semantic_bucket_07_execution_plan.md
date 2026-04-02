# Semantic Bucket 07 Execution Plan

Date: 2026-04-02

This plan tracks wave-based expansion for `tests/final` bucket `07`
(`types-conversions`) with probe-first promotion.

## Current Baseline

- Active manifests:
  - `tests/final/meta/07-types-conversions.json`
  - `tests/final/meta/07-types-conversions-wave2-semantic.json`
  - `tests/final/meta/07-types-conversions-wave3-conv-diagjson.json`
  - `tests/final/meta/07-types-conversions-wave4-conv-matrix.json`
  - `tests/final/meta/07-types-conversions-wave5-blocker-fixes.json`
  - `tests/final/meta/07-types-conversions-wave6-constexpr-positive.json`
  - `tests/final/meta/07-types-conversions-wave7-aggregate-union-depth.json`
  - `tests/final/meta/07-types-conversions-wave8-cast-constexpr-edge.json`
  - `tests/final/meta/07-types-conversions-wave9-policy-aggregate-edge.json`
  - `tests/final/meta/07-types-conversions-wave10-policy-designator-constexpr-depth.json`
  - `tests/final/meta/07-types-conversions-wave11-constexpr-diagjson-depth.json`
  - `tests/final/meta/07-types-conversions-wave12-diff-diagjson-constexpr-frontier.json`
  - `tests/final/meta/07-types-conversions-wave13-strict-memberdiag-constexpr-depth.json`
  - `tests/final/meta/07-types-conversions-wave14-strict-conv-diagjson-constexpr-frontier.json`
  - `tests/final/meta/07-types-conversions-wave15-strict-pointer-diff-depth.json`
  - `tests/final/meta/07-types-conversions-wave16-strict-diagjson-cast-aggregate-depth.json`
  - `tests/final/meta/07-types-conversions-wave17-constexpr-coupled-ice-depth.json`
- Active tests: `100`
- Current validation:
  - `make final-prefix PREFIX=07__` -> all pass
  - `make final-wave WAVE=2 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=3 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=4 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=5 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=6 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=7 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=8 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=9 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=10 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=11 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=12 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=13 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=14 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=15 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=16 WAVE_BUCKET=07-types-conversions` -> all pass
  - `make final-wave WAVE=17 WAVE_BUCKET=07-types-conversions` -> all pass
  - `PROBE_FILTER=07__probe_* python3 tests/final/probes/run_probes.py`
    -> `resolved=91`, `blocked=0`, `skipped=0`

## Wave 2 (Completed)

Promoted tests:

- `07__assign_struct_to_int_reject`
- `07__agg__member_access`
- `07__agg__offsets`
- `07__agg__invalid_member_reject`
- `07__constexpr__array_size_reject`
- `07__constexpr__case_label_reject`
- `07__constexpr__static_init_reject`

Probe-first notes:

- Initial probe blocker was a strict static-initializer diagnostic substring
  mismatch; matcher was adjusted to expected emitted wording
  (`not a constant expression`).
- Re-run after matcher fix: no blocked probes.

## Wave 3 (Completed)

Promoted tests:

- `07__conv__pointer_assign_float_reject`
- `07__conv__pointer_init_struct_reject`
- `07__signed_unsigned_compare_boundary_runtime`
- `07__agg_nested_offsets_runtime`
- `07__diagjson__assign_struct_to_int_reject`
- `07__diagjson__constexpr_array_size_reject`
- `07__diagjson__constexpr_case_label_reject`
- `07__diagjson__constexpr_static_init_reject`
- `07__diagjson__conv_pointer_assign_float_reject`
- `07__diagjson__conv_pointer_init_struct_reject`

## Wave 4 (Completed)

Promoted tests:

- `07__signed_unsigned_compare_long_boundary_runtime`
- `07__conv__pointer_init_nonzero_int_reject`
- `07__conv__assign_pointer_to_int_reject`
- `07__diagjson__conv_pointer_init_nonzero_int_reject`
- `07__diagjson__conv_assign_pointer_to_int_reject`

## Wave 5 (Completed)

Promoted tests:

- `07__null_pointer_constant_runtime`
- `07__diagjson__agg_invalid_member_reject`

Fixes merged:

- null-pointer constant handling in ternary expression typing,
- semantic invalid-member error emission, which also unblocked diagjson parity.

## Wave 6 (Completed)

Promoted tests:

- `07__constexpr_array_size_positive_runtime`
- `07__constexpr_case_label_positive_runtime`
- `07__constexpr_static_init_positive_runtime`

## Wave 7 (Completed)

Promoted tests:

- `07__agg_union_nested_member_runtime`
- `07__agg_union_copy_update_runtime`
- `07__agg_union_missing_nested_field_reject`
- `07__agg_union_arrow_nonptr_reject`
- `07__diagjson__agg_union_missing_nested_field_reject`

## Wave 8 (Completed)

Promoted tests:

- `07__cast_ptr_uintptr_roundtrip_runtime`
- `07__constexpr_enum_cast_sizeof_runtime`
- `07__constexpr_large_bound_runtime`
- `07__cast_struct_to_int_reject`
- `07__cast_int_to_struct_reject`
- `07__diagjson__cast_struct_to_int_reject`
- `07__diagjson__cast_int_to_struct_reject`

## Wave 9 (Completed)

Promoted tests:

- `07__policy_impldef_ptr_ulong_roundtrip_runtime`
- `07__policy_impldef_int_to_ptr_nonzero_runtime`
- `07__constexpr_fold_ternary_bitwise_runtime`
- `07__agg_dot_scalar_base_reject`
- `07__agg_arrow_ptr_to_scalar_reject`
- `07__agg_dot_array_base_reject`
- `07__diagjson__agg_dot_scalar_base_reject`
- `07__diagjson__agg_arrow_ptr_to_scalar_reject`

Fixes merged:

- tightened semantic member-access validation so `.`/`->` on non-aggregate bases
  fail at semantic stage with explicit operator diagnostics.

## Wave 10 (Completed)

Promoted tests:

- `07__policy_impldef_ptr_slong_roundtrip_runtime`
- `07__policy_explicit_nonzero_int_cast_ptr_runtime`
- `07__constexpr_multidim_fold_runtime`
- `07__designator_array_index_nonconst_reject`
- `07__designator_array_index_negative_reject`
- `07__designator_array_index_oob_reject`
- `07__designator_nested_unknown_field_reject`
- `07__diagjson__designator_array_index_nonconst_reject`
- `07__diagjson__designator_array_index_oob_reject`

Fixes merged:

- recursive designated-initializer field validation for nested aggregate paths
  in `src/Syntax/analyze_decls.c`, so unknown nested fields fail closed.

## Wave 11 (Completed)

Promoted tests:

- `07__constexpr_array_chain_depth_runtime`
- `07__constexpr_case_chain_depth_runtime`
- `07__diagjson__designator_nested_unknown_field_reject`

Fixes merged:

- none required (all Wave11 probes resolved directly).

## Wave 12 (Completed)

Promoted tests:

- `07__policy_strict_zero_cast_matrix_runtime`
- `07__constexpr_static_chain_depth_runtime`
- `07__constexpr_switch_cast_sizeof_depth_runtime`
- `07__diagjson__designator_array_index_nonconst_reject_strict`
- `07__diagjson__designator_array_index_oob_reject_strict`
- `07__diagjson__designator_nested_unknown_field_reject_strict`

Fixes merged:

- none required (all Wave12 probes resolved directly).

## Wave 13 (Completed)

Promoted tests:

- `07__policy_strict_fnptr_decay_null_runtime`
- `07__policy_strict_array_decay_ptrdiff_runtime`
- `07__constexpr_static_ternary_bitcast_depth_runtime`
- `07__diagjson__agg_dot_scalar_base_reject_strict`
- `07__diagjson__agg_arrow_ptr_to_scalar_reject_strict`
- `07__diagjson__agg_dot_array_base_reject_strict`

Fixes merged:

- none in compiler code; one probe expression tightened from `*ap` to `&(*ap)[0]`
  to preserve strict cross-compiler equivalence in the array-decay lane.

## Wave 14 (Completed)

Promoted tests:

- `07__policy_strict_const_ptr_roundtrip_runtime`
- `07__policy_strict_null_conditional_matrix_runtime`
- `07__policy_strict_fnptr_table_null_dispatch_runtime`
- `07__constexpr_array_static_shared_chain_depth_runtime`
- `07__constexpr_switch_nested_ternary_bitwise_depth_runtime`
- `07__constexpr_static_cast_sizeof_ternary_depth_runtime`
- `07__diagjson__conv_pointer_assign_float_reject_strict`
- `07__diagjson__conv_pointer_init_struct_reject_strict`
- `07__diagjson__conv_pointer_init_nonzero_int_reject_strict`
- `07__diagjson__conv_assign_pointer_to_int_reject_strict`

Fixes merged:

- none required (all Wave14 probes resolved directly).

## Wave 15 (Completed)

Promoted tests:

- `07__policy_strict_const_compare_matrix_runtime`
- `07__policy_strict_const_chain_assignment_runtime`
- `07__policy_strict_ptr_compare_one_past_runtime`

Fixes merged:

- none required (all Wave15 probes resolved directly).

## Wave 16 (Completed)

Promoted tests:

- `07__diagjson__cast_struct_to_int_reject_strict`
- `07__diagjson__cast_int_to_struct_reject_strict`
- `07__diagjson__agg_union_missing_nested_field_reject_strict`
- `07__diagjson__agg_invalid_member_reject_strict`

Fixes merged:

- none required (all Wave16 probes resolved directly).

## Wave 17 (Completed)

Promoted tests:

- `07__constexpr_shared_ice_matrix_runtime`
- `07__constexpr_shared_ice_matrix_nested_runtime`
- `07__constexpr_shared_ice_cross_surface_depth_runtime`

Fixes merged:

- none required (all Wave17 probes resolved directly).

## Next Wave Targets (Optional Wave 18 Queue)

1. Differential pointer-domain extension:
   - add strict qualifier-preserving pointer-comparison lanes with additional
     one-past and conditional selection matrix depth.
2. DiagJSON strict parity extension:
   - add strict code/line parity for any remaining aggregate/conversion
     rejection paths not yet covered by strict manifests.
3. Constexpr frontier VI:
   - add another cross-surface ICE lane that increases nesting depth while
     keeping deterministic runtime outputs.

## Execution Rules

1. Add probe lanes first (`07__probe_*`).
2. Run full bucket probe filter before promotion:
   `PROBE_FILTER=07__probe_* python3 tests/final/probes/run_probes.py`
3. Promote only resolved probes into `07-types-conversions-waveN-*.json`.
4. Validate with:
   - `make final-manifest MANIFEST=<wave-manifest>.json`
   - `make final-wave WAVE=<n> WAVE_BUCKET=07-types-conversions`
   - `make final-prefix PREFIX=07__`
