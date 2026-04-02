# Torture Bucket 15 Execution Plan

Date: 2026-04-01

This is the active plan for expanding `tests/final` bucket `15`
(`torture-differential`) after bucket-14 stabilization.

## Current Baseline

- Manifests:
  - `tests/final/meta/15-torture-differential.json`
  - `tests/final/meta/15-torture-differential-wave1-control-decl-depth.json`
  - `tests/final/meta/15-torture-differential-wave2-malformed-diagjson.json`
  - `tests/final/meta/15-torture-differential-wave3-multitu.json`
  - `tests/final/meta/15-torture-differential-wave4-pressure.json`
  - `tests/final/meta/15-torture-differential-wave5-fuzz-corpus-hooks.json`
  - `tests/final/meta/15-torture-differential-wave6-control-layout-multitu.json`
  - `tests/final/meta/15-torture-differential-wave7-external-corpus.json`
  - `tests/final/meta/15-torture-differential-wave8-compile-surface.json`
  - `tests/final/meta/15-torture-differential-wave9-malformed-fuzz-matrix.json`
  - `tests/final/meta/15-torture-differential-wave10-gcc-optional-diff.json`
  - `tests/final/meta/15-torture-differential-wave11-policy-tags.json`
  - `tests/final/meta/15-torture-differential-wave12-malformed-diagjson-parity.json`
  - `tests/final/meta/15-torture-differential-wave13-multitu-stress-expansion.json`
  - `tests/final/meta/15-torture-differential-wave14-multitu-link-failure-matrix.json`
  - `tests/final/meta/15-torture-differential-wave15-runtime-stress-expansion.json`
  - `tests/final/meta/15-torture-differential-wave16-malformed-diagjson-expansion.json`
  - `tests/final/meta/15-torture-differential-wave17-external-corpus-scaleup.json`
  - `tests/final/meta/15-torture-differential-wave18-malformed-pp-multifile.json`
  - `tests/final/meta/15-torture-differential-wave19-gcc-tri-diff-matrix.json`
  - `tests/final/meta/15-torture-differential-wave20-stress-ceiling-expansion.json`
  - `tests/final/meta/15-torture-differential-wave21-policy-matrix-expansion.json`
  - `tests/final/meta/15-torture-differential-wave22-malformed-pp-include-graph.json`
  - `tests/final/meta/15-torture-differential-wave23-malformed-multifile-token-chaos.json`
  - `tests/final/meta/15-torture-differential-wave24-gcc-tri-diff-single-tu.json`
  - `tests/final/meta/15-torture-differential-wave25-gcc-tri-diff-multitu.json`
  - `tests/final/meta/15-torture-differential-wave26-stress-ceiling-expansion-iii.json`
  - `tests/final/meta/15-torture-differential-wave27-multitu-stress-ceiling-iv.json`
  - `tests/final/meta/15-torture-differential-wave28-external-corpus-scaleup-i.json`
  - `tests/final/meta/15-torture-differential-wave29-external-corpus-scaleup-ii.json`
  - `tests/final/meta/15-torture-differential-wave30-policy-matrix-expansion-ii.json`
  - `tests/final/meta/15-torture-differential-wave31-malformed-header-surface-parity.json`
  - `tests/final/meta/15-torture-differential-wave32-gcc-tri-diff-policy-crosscheck.json`
  - `tests/final/meta/15-torture-differential-wave33-malformed-header-chain-diagjson-expansion.json`
  - `tests/final/meta/15-torture-differential-wave34-gcc-tri-diff-header-control-matrix.json`
  - `tests/final/meta/15-torture-differential-wave35-malformed-include-graph-diagjson-followup.json`
  - `tests/final/meta/15-torture-differential-wave36-stress-ceiling-frontier-sweep.json`
  - `tests/final/meta/15-torture-differential-wave37-fuzz-volume-deterministic-replay.json`
  - `tests/final/meta/15-torture-differential-wave38-gcc-tri-diff-breadth-expansion.json`
  - `tests/final/meta/15-torture-differential-wave39-compile-surface-expansion.json`
  - `tests/final/meta/15-torture-differential-wave40-external-corpus-pinned-bundle.json`
- Active tests: `145`
- Runtime lanes (`run=true`): `81` (all `differential=true`)
- Compile-surface stress lanes: `2` (plus expanded negative compile-surface lanes)
- Malformed-input negative lanes: `35`
- `.diagjson` parity lanes: `27`
- External-corpus lanes: `11` (`7` runtime + `4` negative)
- Multi-TU linkage-negative lanes: `3`
- Probe lanes:
  - `15__probe_switch_loop_lite`
  - `15__probe_switch_loop_mod5`
  - `15__probe_path_decl_nested_runtime`
  - `15__probe_deep_switch_loop_state_machine`
  - `15__probe_switch_sparse_case_jump_table`
  - `15__probe_declarator_depth_runtime_chain`
  - `15__probe_diag_malformed_unterminated_string_no_crash`
  - `15__probe_diag_malformed_bad_escape_no_crash`
  - `15__probe_diag_malformed_pp_directive_no_crash`
  - `15__probe_diagjson_malformed_unbalanced_block_no_crash`
  - `15__probe_diagjson_malformed_unclosed_comment_no_crash`
  - `15__probe_multitu_many_globals_crossref`
  - `15__probe_multitu_fnptr_dispatch_grid`
  - `15__probe_diag_multitu_duplicate_symbol_reject`
  - `15__probe_diagjson_multitu_duplicate_symbol_reject`
  - `15__probe_deep_recursion_stack_pressure`
  - `15__probe_large_vla_stride_pressure`
  - `15__probe_many_args_regstack_pressure`
  - `15__probe_seeded_expr_fuzz_smoke`
  - `15__probe_seeded_stmt_fuzz_smoke`
  - `15__probe_corpus_micro_compile_smoke`
  - `15__probe_corpus_external_compile_smoke`
  - `15__probe_ast_pathological_type_graph`
  - `15__probe_control_rebind_dispatch_lattice`
  - `15__probe_large_struct_array_checksum_grid`
  - `15__probe_multitu_const_table_crc`
  - `15__probe_diag_corpus_external_compile_reject`
  - `15__probe_diag_pathological_initializer_shape_reject`
  - `15__probe_diagjson_pathological_initializer_shape_reject`
  - `15__probe_diag_malformed_token_stream_seeded_a_no_crash`
  - `15__probe_diag_malformed_token_stream_seeded_b_no_crash`
  - `15__probe_diag_malformed_token_stream_seeded_c_no_crash`
  - `15__probe_diagjson_malformed_token_stream_seeded_a_no_crash`
  - `15__probe_diagjson_malformed_token_stream_seeded_b_no_crash`
  - `15__probe_diagjson_malformed_token_stream_seeded_c_no_crash`
  - `15__probe_runtime_clang_gcc_tri_diff_smoke`
  - `15__probe_policy_signed_shift_impl_defined_tagged`
  - `15__probe_policy_alias_ub_tagged`
  - `15__probe_multitu_fnptr_state_matrix`
  - `15__probe_multitu_struct_vla_stride_bridge`
  - `15__probe_multitu_ring_digest_pipeline`
  - `15__probe_diag_multitu_duplicate_function_definition_reject`
  - `15__probe_diag_multitu_symbol_type_conflict_reject`
  - `15__probe_diagjson_multitu_duplicate_function_definition_reject`
  - `15__probe_diagjson_multitu_symbol_type_conflict_reject`
  - `15__probe_runtime_vla_fnptr_feedback_matrix`
  - `15__probe_runtime_struct_union_reducer_chain`
  - `15__probe_multitu_state_reseed_pipeline`
  - `15__probe_diag_malformed_invalid_dollar_no_crash`
  - `15__probe_diag_malformed_char_invalid_hex_escape_no_crash`
  - `15__probe_diagjson_malformed_invalid_dollar_no_crash`
  - `15__probe_diagjson_malformed_char_invalid_hex_escape_no_crash`
  - `15__probe_corpus_external_parser_fragment_a_smoke`
  - `15__probe_corpus_external_parser_fragment_b_smoke`
  - `15__probe_diag_corpus_external_macro_guard_reject`
  - `15__probe_diag_malformed_pp_unterminated_if_chain_no_crash`
  - `15__probe_diag_malformed_pp_macro_paste_fragments_no_crash`
  - `15__probe_diagjson_malformed_pp_unterminated_if_chain_no_crash`
  - `15__probe_diagjson_malformed_pp_macro_paste_fragments_no_crash`
  - `15__probe_runtime_clang_gcc_tri_diff_control_checksum`
  - `15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge`
  - `15__probe_runtime_clang_gcc_tri_diff_abi_args_matrix`
  - `15__probe_deep_recursion_stack_pressure_ii`
  - `15__probe_many_decls_density_pressure_ii`
  - `15__probe_large_struct_array_pressure_ii`
  - `15__probe_policy_impldef_signed_shift_matrix_tagged`
  - `15__probe_policy_impldef_signed_char_matrix_tagged`
  - `15__probe_policy_ub_unsequenced_write_tagged`
  - `15__probe_policy_ub_alias_overlap_tagged`
  - `15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash`
  - `15__probe_diag_malformed_pp_include_cycle_guarded_no_crash`
  - `15__probe_diagjson_malformed_pp_nested_ifdef_chain_seeded_d_no_crash`
  - `15__probe_diagjson_malformed_pp_include_cycle_guarded_no_crash`
  - `15__probe_diag_malformed_multifile_header_tail_garbage_no_crash`
  - `15__probe_diag_malformed_multifile_macro_arity_mismatch_no_crash`
  - `15__probe_diagjson_malformed_multifile_header_tail_garbage_no_crash`
  - `15__probe_diagjson_malformed_multifile_macro_arity_mismatch_no_crash`
  - `15__probe_runtime_clang_gcc_tri_diff_loop_state_crc_matrix`
  - `15__probe_runtime_clang_gcc_tri_diff_struct_array_stride_crc_matrix`
  - `15__probe_runtime_clang_gcc_tri_diff_pointer_mix_checksum_matrix`
  - `15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge`
  - `15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline`
  - `15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge`
  - `15__probe_deep_recursion_stack_pressure_iii`
  - `15__probe_many_decls_density_pressure_iii`
  - `15__probe_large_struct_array_pressure_iii`
  - `15__probe_multitu_large_table_crc_pressure`
  - `15__probe_multitu_recursive_dispatch_pressure`
  - `15__probe_multitu_layout_stride_pressure`
  - `15__probe_corpus_external_parser_fragment_c_smoke`
  - `15__probe_corpus_external_parser_fragment_d_smoke`
  - `15__probe_diag_corpus_external_macro_chain_reject`
  - `15__probe_corpus_external_decl_chain_smoke`
  - `15__probe_corpus_external_typedef_graph_smoke`
  - `15__probe_diag_corpus_external_include_guard_mismatch_reject`
  - `15__probe_policy_impldef_signed_shift_width_matrix_tagged`
  - `15__probe_policy_impldef_char_promotion_matrix_tagged`
  - `15__probe_policy_ub_signed_overflow_chain_tagged`
  - `15__probe_policy_ub_eval_order_call_sidefx_tagged`
  - `15__probe_diag_malformed_header_guard_tail_mismatch_no_crash`
  - `15__probe_diag_malformed_header_macro_cycle_chain_no_crash`
  - `15__probe_diagjson_malformed_header_guard_tail_mismatch_no_crash`
  - `15__probe_runtime_clang_gcc_tri_diff_policy_shift_char_matrix`
  - `15__probe_runtime_clang_gcc_tri_diff_policy_struct_abi_matrix`
  - `15__probe_runtime_clang_gcc_tri_diff_header_control_dispatch_matrix`
  - `15__probe_runtime_clang_gcc_tri_diff_header_layout_fold_matrix`
  - `15__probe_diag_malformed_include_graph_guard_collision_no_crash`
  - `15__probe_diagjson_malformed_include_graph_guard_collision_no_crash`
  - `15__probe_diagjson_malformed_include_graph_macro_arity_chain_no_crash`
  - `15__probe_diagjson_malformed_header_macro_cycle_chain_no_crash`
  - `15__probe_diag_malformed_header_guard_tail_mismatch_chain_no_crash`
  - `15__probe_diagjson_malformed_header_guard_tail_mismatch_chain_no_crash`
- Validation snapshot:
  - `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py`
    -> `resolved=136`, `blocked=0`, `skipped=0`
  - `make final-bucket BUCKET=torture-differential`
    -> `0 failing, 10 skipped` (`145` active tests)

## Pause Stocktake (2026-04-01)

Historical stocktake captured before Waves 36-40.
Post-wave40 state (current):

- Strong coverage now:
  - deterministic runtime stress lanes (single-TU + multi-TU) with clang differential,
  - malformed fail-closed no-crash matrix with paired `.diagjson` parity lanes,
  - explicit UB / implementation-defined tagging with differential skip policy,
  - optional gcc tri-reference lanes, including expanded breadth from Wave38,
  - stress-ceiling frontier sweep family (Wave36),
  - deterministic fuzz-volume replay family (Wave37),
  - expanded compile-surface negatives with `.diagjson` parity (Wave39),
  - expanded pinned external-corpus bundle (Wave40).

Remaining optional backlog (non-blocking):
1. Further widen gcc tri-reference lanes so coverage is less runtime-subset concentrated.
2. Expand pinned external corpus beyond current Wave40 fragment set.
3. Add randomized-volume harness mode with deterministic replay capture/staging.
4. Grow compile-surface-only stress lanes to balance runtime-heavy distribution.
5. Extend implementation-defined policy matrix depth for additional ABI surfaces.

## What Is Covered Now

- Long expression chains and deep nesting runtime behavior.
- Dense declaration/global-count runtime behavior.
- Many-parameter ABI call stress.
- Large struct and large array runtime paths.
- Pathological declarator parse/AST anchor.
- Additional pathological type-graph AST lane and initializer rejection lanes.
- Malformed-input no-crash diagnostics for parser/lexer robustness.
- Deterministic seeded malformed token-stream matrix with `.diagjson` parity.
- Expanded seeded malformed matrix parity across A/B/C lanes.
- Optional GCC differential lane with tri-reference runtime probe (fisics/clang/gcc).
- Explicit policy-tagged lanes for UB and implementation-defined behavior.
- Expanded multi-TU stress matrix beyond baseline dispatch/CRC lanes.
- Expanded multi-TU link-failure diagnostics + `.diagjson` parity matrix.
- External-corpus styled deterministic compile/run lane plus fail-closed negative fragment.

## Key Remaining Gap

- Link-stage `.diagjson` completeness is tracked separately in
  `docs/plans/link_stage_diagjson_enablement_plan.md` (not a bucket-15 blocker).

## Wave Plan

## Wave 1: Control + Declarator Depth

Goal: increase parser/control complexity with deterministic runtime or AST checks.

Promoted tests:
- `15__torture__deep_switch_loop_state_machine`
- `15__torture__switch_sparse_case_jump_table`
- `15__torture__declarator_depth_runtime_chain`

Shard target:
- `tests/final/meta/15-torture-differential-wave1-control-decl-depth.json`

Validation:
- `PROBE_FILTER=15__probe_deep_switch_loop_state_machine,15__probe_switch_sparse_case_jump_table,15__probe_declarator_depth_runtime_chain python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave1-control-decl-depth.json`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> all `15` pass

## Wave 2: Malformed Robustness + JSON Parity

Goal: harden fail-closed malformed handling and add structured diagnostics checks.

Promoted tests:
- `15__torture__malformed_unterminated_string_no_crash`
- `15__torture__malformed_bad_escape_no_crash`
- `15__torture__malformed_pp_directive_no_crash`
- `15__diagjson__malformed_unbalanced_block_no_crash`
- `15__diagjson__malformed_unclosed_comment_no_crash`

Shard target:
- `tests/final/meta/15-torture-differential-wave2-malformed-diagjson.json`

Validation:
- `PROBE_FILTER=15__probe_diag_*,15__probe_diagjson_* python3 tests/final/probes/run_probes.py`
  -> `resolved=5`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave2-malformed-diagjson.json`
  -> all `5` pass
- `make final-bucket BUCKET=torture-differential`
  -> all `20` pass

Wave2 fix shipped:
- Lexer fatal errors are now mirrored into compiler diagnostics so
  `--emit-diags-json` is populated for fail-closed lexer malformed paths
  (including unterminated block comment inputs).

## Wave 3: Multi-TU Torture Surface

Goal: add cross-translation-unit stress in this bucket.

Promoted tests:
- `15__torture__multitu_many_globals_crossref`
- `15__torture__multitu_fnptr_dispatch_grid`
- `15__torture__multitu_duplicate_symbol_reject`
- `15__diagjson__multitu_duplicate_symbol_reject`

Shard target:
- `tests/final/meta/15-torture-differential-wave3-multitu.json`

Validation:
- Targeted probe runs:
  - `PROBE_FILTER=15__probe_multitu_many_globals_crossref ...` -> resolved
  - `PROBE_FILTER=15__probe_multitu_fnptr_dispatch_grid ...` -> resolved
  - `PROBE_FILTER=15__probe_diag_multitu_duplicate_symbol_reject ...` -> resolved
  - `PROBE_FILTER=15__probe_diagjson_multitu_duplicate_symbol_reject ...` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave3-multitu.json`
  -> all `4` pass
- `make final-bucket BUCKET=torture-differential`
  -> all `24` pass

## Wave 4: Resource Pressure

Goal: push deterministic depth/size pressure paths.

Promoted tests:
- `15__torture__deep_recursion_stack_pressure`
- `15__torture__large_vla_stride_pressure`
- `15__torture__many_args_regstack_pressure`

Shard target:
- `tests/final/meta/15-torture-differential-wave4-pressure.json`

Validation:
- Targeted probe runs:
  - `PROBE_FILTER=15__probe_deep_recursion_stack_pressure ...` -> resolved
  - `PROBE_FILTER=15__probe_large_vla_stride_pressure ...` -> resolved
  - `PROBE_FILTER=15__probe_many_args_regstack_pressure ...` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave4-pressure.json`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> all `27` pass

## Wave 5: Deterministic Fuzz/Corpus Hooks

Goal: prepare optional high-volume lanes without making the suite flaky.

Promoted tests:
- `15__torture__seeded_expr_fuzz_smoke`
- `15__torture__seeded_stmt_fuzz_smoke`
- `15__torture__corpus_micro_compile_smoke`

Shard target:
- `tests/final/meta/15-torture-differential-wave5-fuzz-corpus-hooks.json`

Validation:
- `PROBE_FILTER=15__probe_seeded_expr_fuzz_smoke,15__probe_seeded_stmt_fuzz_smoke,15__probe_corpus_micro_compile_smoke python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave5-fuzz-corpus-hooks.json`
  -> all `3` pass
- `make final-wave WAVE=5 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> all `30` pass

## Wave 6: Control/Layout + Multi-TU Const-Table

Goal: deepen control-flow and memory-layout stress while adding another
cross-translation-unit runtime lane.

Promoted tests:
- `15__torture__control_rebind_dispatch_lattice`
- `15__torture__large_struct_array_checksum_grid`
- `15__torture__multitu_const_table_crc`

Shard target:
- `tests/final/meta/15-torture-differential-wave6-control-layout-multitu.json`

Validation:
- `PROBE_FILTER=15__probe_control_rebind_dispatch_lattice,15__probe_large_struct_array_checksum_grid,15__probe_multitu_const_table_crc python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave6-control-layout-multitu.json`
  -> all `3` pass
- `make final-wave WAVE=6 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> all `33` pass

## Wave 7: External Corpus Deterministic Lane

Goal: add one real external-corpus styled lane with deterministic fixture pinning.

Promoted tests:
- `15__torture__corpus_external_compile_smoke`
- `15__torture__corpus_external_compile_reject`

Shard target:
- `tests/final/meta/15-torture-differential-wave7-external-corpus.json`

Validation:
- `PROBE_FILTER=15__probe_corpus_external_compile_smoke,15__probe_diag_corpus_external_compile_reject python3 tests/final/probes/run_probes.py`
  -> `resolved=2`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave7-external-corpus.json`
  -> all `2` pass
- `make final-wave WAVE=7 WAVE_BUCKET=15-torture-differential`
  -> all `2` pass
- `make final-bucket BUCKET=torture-differential`
  -> all `35` pass

## Wave 8: Compile-Surface Torture Expansion

Goal: broaden non-runtime stress beyond one declarator lane.

Promoted tests:
- `15__torture__pathological_type_graph`
- `15__torture__pathological_initializer_shape_reject`
- `15__diagjson__pathological_initializer_shape_reject`

Shard target:
- `tests/final/meta/15-torture-differential-wave8-compile-surface.json`

Validation:
- `PROBE_FILTER=15__probe_ast_pathological_type_graph,15__probe_diag_pathological_initializer_shape_reject,15__probe_diagjson_pathological_initializer_shape_reject python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave8-compile-surface.json`
  -> all `3` pass
- `make final-wave WAVE=8 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> all `38` pass

## Wave 9: Malformed Fuzz-Smoke Matrix

Goal: deterministic malformed-input matrix that guarantees no-crash + diagnostics.

Promoted tests:
- `15__torture__malformed_token_stream_seeded_a_no_crash`
- `15__torture__malformed_token_stream_seeded_b_no_crash`
- `15__diagjson__malformed_token_stream_seeded_a_no_crash`

Shard target:
- `tests/final/meta/15-torture-differential-wave9-malformed-fuzz-matrix.json`

Validation:
- `PROBE_FILTER=15__probe_diag_malformed_token_stream_seeded_a_no_crash,15__probe_diag_malformed_token_stream_seeded_b_no_crash,15__probe_diagjson_malformed_token_stream_seeded_a_no_crash python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave9-malformed-fuzz-matrix.json`
  -> all `3` pass
- `make final-wave WAVE=9 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> all `41` pass

## Wave 10: Optional GCC Differential Lane

Goal: add a second external reference where available while preserving determinism.

Promoted tests:
- `15__torture__clang_gcc_tri_diff_smoke`

Probe lane:
- `15__probe_runtime_clang_gcc_tri_diff_smoke`

Shard target:
- `tests/final/meta/15-torture-differential-wave10-gcc-optional-diff.json`

Validation:
- `PROBE_FILTER=15__probe_runtime_clang_gcc_tri_diff_smoke python3 tests/final/probes/run_probes.py`
  -> `resolved=1`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave10-gcc-optional-diff.json`
  -> all `1` pass
- `make final-wave WAVE=10 WAVE_BUCKET=15-torture-differential`
  -> all `1` pass
- `make final-bucket BUCKET=torture-differential`
  -> all `42` pass

Notes:
- Tool availability is detected at runtime.
- Missing GCC is `skipped`, not `pass`.

## Wave 11: UB / Impl-Defined Policy Tagging

Goal: make bucket-15 policy intent explicit for differential gating and audits.

Promoted tests:
- `15__torture__policy_signed_shift_impl_defined_tagged`
- `15__torture__policy_alias_ub_tagged`

Probe lanes:
- `15__probe_policy_signed_shift_impl_defined_tagged`
- `15__probe_policy_alias_ub_tagged`

Shard target:
- `tests/final/meta/15-torture-differential-wave11-policy-tags.json`

Validation:
- `PROBE_FILTER=15__probe_policy_signed_shift_impl_defined_tagged,15__probe_policy_alias_ub_tagged python3 tests/final/probes/run_probes.py`
  -> `resolved=2`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave11-policy-tags.json`
  -> `0 failing, 2 skipped` (skip-gated by policy fields)
- `make final-wave WAVE=11 WAVE_BUCKET=15-torture-differential`
  -> `0 failing, 2 skipped`
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 2 skipped` (`44` tests active)

Notes:
- Uses explicit metadata tags (`ub`, `impl_defined`) with conservative differential rules.

## Wave 12: Malformed Matrix DiagJSON Parity Expansion

Goal: broaden deterministic malformed seeded coverage and close `.diagjson` parity symmetry.

Promoted tests:
- `15__torture__malformed_token_stream_seeded_c_no_crash`
- `15__diagjson__malformed_token_stream_seeded_b_no_crash`
- `15__diagjson__malformed_token_stream_seeded_c_no_crash`

Probe lanes:
- `15__probe_diag_malformed_token_stream_seeded_c_no_crash`
- `15__probe_diagjson_malformed_token_stream_seeded_b_no_crash`
- `15__probe_diagjson_malformed_token_stream_seeded_c_no_crash`

Shard target:
- `tests/final/meta/15-torture-differential-wave12-malformed-diagjson-parity.json`

Validation:
- `PROBE_FILTER=15__probe_diag_malformed_token_stream_seeded_c_no_crash,15__probe_diagjson_malformed_token_stream_seeded_b_no_crash,15__probe_diagjson_malformed_token_stream_seeded_c_no_crash python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave12-malformed-diagjson-parity.json`
  -> all `3` pass
- `make final-wave WAVE=12 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 2 skipped` (`47` tests active)

## Wave 13: Multi-TU Stress Expansion

Goal: broaden high-density cross-TU runtime stress lanes with stateful function-pointer,
VLA/struct stride, and static ring-pipeline coverage.

Promoted tests:
- `15__torture__multitu_fnptr_state_matrix`
- `15__torture__multitu_struct_vla_stride_bridge`
- `15__torture__multitu_ring_digest_pipeline`

Probe lanes:
- `15__probe_multitu_fnptr_state_matrix`
- `15__probe_multitu_struct_vla_stride_bridge`
- `15__probe_multitu_ring_digest_pipeline`

Shard target:
- `tests/final/meta/15-torture-differential-wave13-multitu-stress-expansion.json`

Validation:
- `PROBE_FILTER=15__probe_multitu_fnptr_state_matrix,15__probe_multitu_struct_vla_stride_bridge,15__probe_multitu_ring_digest_pipeline python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave13-multitu-stress-expansion.json`
  -> all `3` pass
- `make final-wave WAVE=13 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 2 skipped` (`50` tests active)

## Wave 14: Multi-TU Link-Failure Matrix

Goal: increase fail-closed link-stage negative coverage and keep `.diagjson` parity
explicit for additional multi-TU collision lanes.

Promoted tests:
- `15__torture__multitu_duplicate_function_definition_reject`
- `15__torture__multitu_symbol_type_conflict_reject`
- `15__diagjson__multitu_duplicate_function_definition_reject`
- `15__diagjson__multitu_symbol_type_conflict_reject`

Probe lanes:
- `15__probe_diag_multitu_duplicate_function_definition_reject`
- `15__probe_diag_multitu_symbol_type_conflict_reject`
- `15__probe_diagjson_multitu_duplicate_function_definition_reject`
- `15__probe_diagjson_multitu_symbol_type_conflict_reject`

Shard target:
- `tests/final/meta/15-torture-differential-wave14-multitu-link-failure-matrix.json`

Validation:
- `PROBE_FILTER=15__probe_diag_multitu_duplicate_function_definition_reject,15__probe_diag_multitu_symbol_type_conflict_reject,15__probe_diagjson_multitu_duplicate_function_definition_reject,15__probe_diagjson_multitu_symbol_type_conflict_reject python3 tests/final/probes/run_probes.py`
  -> `resolved=4`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave14-multitu-link-failure-matrix.json`
  -> all `4` pass
- `make final-wave WAVE=14 WAVE_BUCKET=15-torture-differential`
  -> all `4` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 2 skipped` (`54` tests active)

## Wave 15: Runtime Stress Expansion

Goal: deepen runtime differential pressure with VLA/function-pointer feedback,
struct/union reducer chaining, and a fresh multi-TU reseed pipeline lane.

Promoted tests:
- `15__torture__runtime_vla_fnptr_feedback_matrix`
- `15__torture__runtime_struct_union_reducer_chain`
- `15__torture__multitu_state_reseed_pipeline`

Probe lanes:
- `15__probe_runtime_vla_fnptr_feedback_matrix`
- `15__probe_runtime_struct_union_reducer_chain`
- `15__probe_multitu_state_reseed_pipeline`

Shard target:
- `tests/final/meta/15-torture-differential-wave15-runtime-stress-expansion.json`

Validation:
- `PROBE_FILTER=15__probe_runtime_vla_fnptr_feedback_matrix,15__probe_runtime_struct_union_reducer_chain,15__probe_multitu_state_reseed_pipeline python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave15-runtime-stress-expansion.json`
  -> all `3` pass
- `make final-wave WAVE=15 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 2 skipped` (`57` tests active)

## Wave 16: Malformed + DiagJSON Expansion

Goal: expand malformed lexer-fatal no-crash lanes with explicit text-diagnostic
and `.diagjson` parity checks.

Promoted tests:
- `15__torture__malformed_invalid_dollar_no_crash`
- `15__torture__malformed_char_invalid_hex_escape_no_crash`
- `15__diagjson__malformed_invalid_dollar_no_crash`
- `15__diagjson__malformed_char_invalid_hex_escape_no_crash`

Probe lanes:
- `15__probe_diag_malformed_invalid_dollar_no_crash`
- `15__probe_diag_malformed_char_invalid_hex_escape_no_crash`
- `15__probe_diagjson_malformed_invalid_dollar_no_crash`
- `15__probe_diagjson_malformed_char_invalid_hex_escape_no_crash`

Shard target:
- `tests/final/meta/15-torture-differential-wave16-malformed-diagjson-expansion.json`

Validation:
- `PROBE_FILTER=15__probe_diag_malformed_invalid_dollar_no_crash,15__probe_diag_malformed_char_invalid_hex_escape_no_crash,15__probe_diagjson_malformed_invalid_dollar_no_crash,15__probe_diagjson_malformed_char_invalid_hex_escape_no_crash python3 tests/final/probes/run_probes.py`
  -> `resolved=4`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave16-malformed-diagjson-expansion.json`
  -> all `4` pass
- `make final-wave WAVE=16 WAVE_BUCKET=15-torture-differential`
  -> all `4` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 2 skipped` (`61` tests active)

## Wave 17: External Corpus Scale-Up

Goal: expand deterministic real-world corpus coverage beyond current smoke/reject pair.

Promoted lanes:
- `15__torture__corpus_external_parser_fragment_a_smoke`
- `15__torture__corpus_external_parser_fragment_b_smoke`
- `15__torture__corpus_external_macro_guard_reject`

Probe lanes:
- `15__probe_corpus_external_parser_fragment_a_smoke`
- `15__probe_corpus_external_parser_fragment_b_smoke`
- `15__probe_diag_corpus_external_macro_guard_reject`

Shard target:
- `tests/final/meta/15-torture-differential-wave17-external-corpus-scaleup.json`

Validation:
- `PROBE_FILTER=15__probe_corpus_external_parser_fragment_a_smoke,15__probe_corpus_external_parser_fragment_b_smoke,15__probe_diag_corpus_external_macro_guard_reject python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave17-external-corpus-scaleup.json`
  -> all `3` pass
- `make final-wave WAVE=17 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 2 skipped` (`64` tests active)

## Wave 18: Malformed Matrix Expansion (PP + Multi-File)

Goal: broaden malformed no-crash coverage into preprocessor-heavy and multi-file surfaces.

Promoted lanes:
- `15__torture__malformed_pp_unterminated_if_chain_no_crash`
- `15__torture__malformed_pp_macro_paste_fragments_no_crash`
- `15__diagjson__malformed_pp_unterminated_if_chain_no_crash`
- `15__diagjson__malformed_pp_macro_paste_fragments_no_crash`

Probe lanes:
- `15__probe_diag_malformed_pp_unterminated_if_chain_no_crash`
- `15__probe_diag_malformed_pp_macro_paste_fragments_no_crash`
- `15__probe_diagjson_malformed_pp_unterminated_if_chain_no_crash`
- `15__probe_diagjson_malformed_pp_macro_paste_fragments_no_crash`

Shard target:
- `tests/final/meta/15-torture-differential-wave18-malformed-pp-multifile.json`

Validation:
- `PROBE_FILTER=15__probe_diag_malformed_pp_unterminated_if_chain_no_crash,15__probe_diag_malformed_pp_macro_paste_fragments_no_crash,15__probe_diagjson_malformed_pp_unterminated_if_chain_no_crash,15__probe_diagjson_malformed_pp_macro_paste_fragments_no_crash python3 tests/final/probes/run_probes.py`
  -> `resolved=4`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave18-malformed-pp-multifile.json`
  -> all `4` pass
- `make final-wave WAVE=18 WAVE_BUCKET=15-torture-differential`
  -> all `4` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 2 skipped` (`68` tests active)

## Latest Completed + Next Wave

## Wave 19: GCC Tri-Differential Breadth

Goal: expand optional clang+gcc runtime parity from one lane to a small stable matrix.

Promoted lanes:
- `15__torture__clang_gcc_tri_diff_control_checksum`
- `15__torture__clang_gcc_tri_diff_multitu_state_bridge`
- `15__torture__clang_gcc_tri_diff_abi_args_matrix`

Probe lanes:
- `15__probe_runtime_clang_gcc_tri_diff_control_checksum`
- `15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge`
- `15__probe_runtime_clang_gcc_tri_diff_abi_args_matrix`

Shard target:
- `tests/final/meta/15-torture-differential-wave19-gcc-tri-diff-matrix.json`

Validation:
- `PROBE_FILTER=15__probe_runtime_clang_gcc_tri_diff_control_checksum,15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge,15__probe_runtime_clang_gcc_tri_diff_abi_args_matrix python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave19-gcc-tri-diff-matrix.json`
  -> all `3` pass
- `make final-wave WAVE=19 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 2 skipped` (`71` tests active)

## Wave 20: Stress Ceiling Expansion

Goal: raise deterministic pressure ceilings for recursion, declaration density, and aggregates.

Promoted lanes:
- `15__torture__deep_recursion_stack_pressure_ii`
- `15__torture__many_decls_density_pressure_ii`
- `15__torture__large_struct_array_pressure_ii`

Probe lanes:
- `15__probe_deep_recursion_stack_pressure_ii`
- `15__probe_many_decls_density_pressure_ii`
- `15__probe_large_struct_array_pressure_ii`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave20-stress-ceiling-expansion.json`

Validation:
- `PROBE_FILTER=15__probe_deep_recursion_stack_pressure_ii,15__probe_many_decls_density_pressure_ii,15__probe_large_struct_array_pressure_ii python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave20-stress-ceiling-expansion.json`
  -> all `3` pass
- `make final-wave WAVE=20 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 2 skipped` (`74` tests active)

## Wave 21: Policy-Tagged Matrix Expansion

Goal: increase explicit `ub` / `impl_defined` audit coverage while preserving fail-closed gating.

Promoted lanes:
- `15__torture__policy_impldef_signed_shift_matrix_tagged`
- `15__torture__policy_impldef_signed_char_matrix_tagged`
- `15__torture__policy_ub_unsequenced_write_tagged`
- `15__torture__policy_ub_alias_overlap_tagged`

Probe lanes:
- `15__probe_policy_impldef_signed_shift_matrix_tagged`
- `15__probe_policy_impldef_signed_char_matrix_tagged`
- `15__probe_policy_ub_unsequenced_write_tagged`
- `15__probe_policy_ub_alias_overlap_tagged`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave21-policy-matrix-expansion.json`

Validation:
- `PROBE_FILTER=15__probe_policy_impldef_signed_shift_matrix_tagged,15__probe_policy_impldef_signed_char_matrix_tagged,15__probe_policy_ub_unsequenced_write_tagged,15__probe_policy_ub_alias_overlap_tagged python3 tests/final/probes/run_probes.py`
  -> `resolved=4`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave21-policy-matrix-expansion.json`
  -> `0 failing, 4 skipped` (policy skip-gated differential)
- `make final-wave WAVE=21 WAVE_BUCKET=15-torture-differential`
  -> `0 failing, 4 skipped`
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 6 skipped` (`78` tests active)

## Wave 22: Malformed Matrix Expansion I (PP + Include Graph)

Goal: increase fail-closed malformed coverage on preprocessor-heavy paths.

Promoted lanes:
- `15__torture__malformed_pp_nested_ifdef_chain_seeded_d_no_crash`
- `15__torture__malformed_pp_include_cycle_guarded_no_crash`
- `15__diagjson__malformed_pp_nested_ifdef_chain_seeded_d_no_crash`
- `15__diagjson__malformed_pp_include_cycle_guarded_no_crash`

Probe lanes:
- `15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash`
- `15__probe_diag_malformed_pp_include_cycle_guarded_no_crash`
- `15__probe_diagjson_malformed_pp_nested_ifdef_chain_seeded_d_no_crash`
- `15__probe_diagjson_malformed_pp_include_cycle_guarded_no_crash`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave22-malformed-pp-include-graph.json`

Validation:
- `PROBE_FILTER=15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash,15__probe_diag_malformed_pp_include_cycle_guarded_no_crash,15__probe_diagjson_malformed_pp_nested_ifdef_chain_seeded_d_no_crash,15__probe_diagjson_malformed_pp_include_cycle_guarded_no_crash python3 tests/final/probes/run_probes.py`
  -> `resolved=4`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave22-malformed-pp-include-graph.json`
  -> all `4` pass
- `make final-wave WAVE=22 WAVE_BUCKET=15-torture-differential`
  -> all `4` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 6 skipped` (`82` tests active)

## Wave 23: Malformed Matrix Expansion II (multi-file token chaos)

Goal: add deterministic malformed multi-file lanes and `.diagjson` parity.

Promoted lanes:
- `15__torture__malformed_multifile_header_tail_garbage_no_crash`
- `15__torture__malformed_multifile_macro_arity_mismatch_no_crash`
- `15__diagjson__malformed_multifile_header_tail_garbage_no_crash`
- `15__diagjson__malformed_multifile_macro_arity_mismatch_no_crash`

Probe lanes:
- `15__probe_diag_malformed_multifile_header_tail_garbage_no_crash`
- `15__probe_diag_malformed_multifile_macro_arity_mismatch_no_crash`
- `15__probe_diagjson_malformed_multifile_header_tail_garbage_no_crash`
- `15__probe_diagjson_malformed_multifile_macro_arity_mismatch_no_crash`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave23-malformed-multifile-token-chaos.json`

Validation:
- `PROBE_FILTER=15__probe_diag_malformed_multifile_header_tail_garbage_no_crash,15__probe_diag_malformed_multifile_macro_arity_mismatch_no_crash,15__probe_diagjson_malformed_multifile_header_tail_garbage_no_crash,15__probe_diagjson_malformed_multifile_macro_arity_mismatch_no_crash python3 tests/final/probes/run_probes.py`
  -> `resolved=4`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave23-malformed-multifile-token-chaos.json`
  -> all `4` pass
- `make final-wave WAVE=23 WAVE_BUCKET=15-torture-differential`
  -> all `4` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 6 skipped` (`86` tests active)

## Wave 24: Tri-Compiler Differential Breadth I (single-TU)

Goal: broaden clang+gcc parity on deterministic single-TU runtime lanes.

Promoted lanes:
- `15__torture__clang_gcc_tri_diff_loop_state_crc_matrix`
- `15__torture__clang_gcc_tri_diff_struct_array_stride_crc_matrix`
- `15__torture__clang_gcc_tri_diff_pointer_mix_checksum_matrix`

Probe lanes:
- `15__probe_runtime_clang_gcc_tri_diff_loop_state_crc_matrix`
- `15__probe_runtime_clang_gcc_tri_diff_struct_array_stride_crc_matrix`
- `15__probe_runtime_clang_gcc_tri_diff_pointer_mix_checksum_matrix`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave24-gcc-tri-diff-single-tu.json`

Validation:
- `PROBE_FILTER=15__probe_runtime_clang_gcc_tri_diff_loop_state_crc_matrix,15__probe_runtime_clang_gcc_tri_diff_struct_array_stride_crc_matrix,15__probe_runtime_clang_gcc_tri_diff_pointer_mix_checksum_matrix python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave24-gcc-tri-diff-single-tu.json`
  -> all `3` pass
- `make final-wave WAVE=24 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 6 skipped` (`89` active tests)

## Wave 25: Tri-Compiler Differential Breadth II (multi-TU)

Goal: broaden clang+gcc parity on stable multi-TU ABI/runtime lanes.

Promoted lanes:
- `15__torture__clang_gcc_tri_diff_multitu_fnptr_table_bridge`
- `15__torture__clang_gcc_tri_diff_multitu_const_seed_pipeline`
- `15__torture__clang_gcc_tri_diff_multitu_layout_digest_bridge`

Probe lanes:
- `15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge`
- `15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline`
- `15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave25-gcc-tri-diff-multitu.json`

Validation:
- `PROBE_FILTER=15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge,15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline,15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave25-gcc-tri-diff-multitu.json`
  -> all `3` pass
- `make final-wave WAVE=25 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 6 skipped` (`92` active tests)

## Wave 26: Stress Ceiling Expansion III

Goal: raise deterministic depth/size pressure ceilings again.

Promoted lanes:
- `15__torture__deep_recursion_stack_pressure_iii`
- `15__torture__many_decls_density_pressure_iii`
- `15__torture__large_struct_array_pressure_iii`

Probe lanes:
- `15__probe_deep_recursion_stack_pressure_iii`
- `15__probe_many_decls_density_pressure_iii`
- `15__probe_large_struct_array_pressure_iii`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave26-stress-ceiling-expansion-iii.json`

Validation:
- `PROBE_FILTER=15__probe_deep_recursion_stack_pressure_iii,15__probe_many_decls_density_pressure_iii,15__probe_large_struct_array_pressure_iii python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave26-stress-ceiling-expansion-iii.json`
  -> all `3` pass
- `make final-wave WAVE=26 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 6 skipped` (`95` active tests)

## Wave 27: Stress Ceiling Expansion IV (multi-TU pressure)

Goal: add heavier cross-TU pressure lanes with deterministic checksums.

Promoted lanes:
- `15__torture__multitu_large_table_crc_pressure`
- `15__torture__multitu_recursive_dispatch_pressure`
- `15__torture__multitu_layout_stride_pressure`

Probe lanes:
- `15__probe_multitu_large_table_crc_pressure`
- `15__probe_multitu_recursive_dispatch_pressure`
- `15__probe_multitu_layout_stride_pressure`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave27-multitu-stress-ceiling-iv.json`

Validation:
- `PROBE_FILTER=15__probe_multitu_large_table_crc_pressure,15__probe_multitu_recursive_dispatch_pressure,15__probe_multitu_layout_stride_pressure python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave27-multitu-stress-ceiling-iv.json`
  -> all `3` pass
- `make final-wave WAVE=27 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 6 skipped` (`98` active tests)

## Wave 28: External Corpus Scale-Up I

Goal: expand deterministic real-world fragment coverage.

Promoted lanes:
- `15__torture__corpus_external_parser_fragment_c_smoke`
- `15__torture__corpus_external_parser_fragment_d_smoke`
- `15__torture__corpus_external_macro_chain_reject`

Probe lanes:
- `15__probe_corpus_external_parser_fragment_c_smoke`
- `15__probe_corpus_external_parser_fragment_d_smoke`
- `15__probe_diag_corpus_external_macro_chain_reject`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave28-external-corpus-scaleup-i.json`

Validation:
- `PROBE_FILTER=15__probe_corpus_external_parser_fragment_c_smoke,15__probe_corpus_external_parser_fragment_d_smoke,15__probe_diag_corpus_external_macro_chain_reject python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave28-external-corpus-scaleup-i.json`
  -> all `3` pass
- `make final-wave WAVE=28 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 6 skipped` (`101` active tests)

## Queued Wave List (34-35)

This section tracks the latest completed waves plus the next queued
probe/promote batches.

## Wave 29: External Corpus Scale-Up II (Completed)

Goal: extend corpus compile-surface and negative fragment breadth.

Promoted lanes:
- `15__torture__corpus_external_decl_chain_smoke`
- `15__torture__corpus_external_typedef_graph_smoke`
- `15__torture__corpus_external_include_guard_mismatch_reject`

Probe lanes:
- `15__probe_corpus_external_decl_chain_smoke`
- `15__probe_corpus_external_typedef_graph_smoke`
- `15__probe_diag_corpus_external_include_guard_mismatch_reject`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave29-external-corpus-scaleup-ii.json`

Validation:
- `PROBE_FILTER=15__probe_corpus_external_decl_chain_smoke,15__probe_corpus_external_typedef_graph_smoke,15__probe_diag_corpus_external_include_guard_mismatch_reject python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave29-external-corpus-scaleup-ii.json`
  -> all `3` pass
- `make final-wave WAVE=29 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 6 skipped` (`104` active tests)

## Wave 30: Policy Matrix Expansion II (Completed)

Goal: increase explicit UB/impl-defined audit lanes while preserving skip-gated
differential policy behavior.

Promoted lanes:
- `15__torture__policy_impldef_signed_shift_width_matrix_tagged`
- `15__torture__policy_impldef_char_promotion_matrix_tagged`
- `15__torture__policy_ub_signed_overflow_chain_tagged`
- `15__torture__policy_ub_eval_order_call_sidefx_tagged`

Probe lanes:
- `15__probe_policy_impldef_signed_shift_width_matrix_tagged`
- `15__probe_policy_impldef_char_promotion_matrix_tagged`
- `15__probe_policy_ub_signed_overflow_chain_tagged`
- `15__probe_policy_ub_eval_order_call_sidefx_tagged`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave30-policy-matrix-expansion-ii.json`

Validation:
- `PROBE_FILTER=15__probe_policy_impldef_signed_shift_width_matrix_tagged,15__probe_policy_impldef_char_promotion_matrix_tagged,15__probe_policy_ub_signed_overflow_chain_tagged,15__probe_policy_ub_eval_order_call_sidefx_tagged python3 tests/final/probes/run_probes.py`
  -> `resolved=4`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave30-policy-matrix-expansion-ii.json`
  -> `0 failing, 4 skipped`
- `make final-wave WAVE=30 WAVE_BUCKET=15-torture-differential`
  -> `0 failing, 4 skipped`
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 10 skipped` (`108` active tests)

## Wave 31: Malformed/Header-Surface Parity Follow-up (Completed)

Goal: extend deterministic fail-closed malformed/header interactions with text
diagnostics and selected `.diagjson` parity where stable.

Promoted lanes:
- `15__torture__malformed_header_guard_tail_mismatch_no_crash`
- `15__torture__malformed_header_macro_cycle_chain_no_crash`
- `15__diagjson__malformed_header_guard_tail_mismatch_no_crash`

Probe lanes:
- `15__probe_diag_malformed_header_guard_tail_mismatch_no_crash`
- `15__probe_diag_malformed_header_macro_cycle_chain_no_crash`
- `15__probe_diagjson_malformed_header_guard_tail_mismatch_no_crash`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave31-malformed-header-surface-parity.json`

Validation:
- `PROBE_FILTER=15__probe_diag_malformed_header_guard_tail_mismatch_no_crash,15__probe_diag_malformed_header_macro_cycle_chain_no_crash,15__probe_diagjson_malformed_header_guard_tail_mismatch_no_crash python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave31-malformed-header-surface-parity.json`
  -> all `3` pass
- `make final-wave WAVE=31 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 10 skipped` (`111` active tests)

## Wave 32: Optional GCC Tri-Diff Policy Crosscheck (Completed)

Goal: add optional GCC crosscheck lanes for host-stable policy surfaces while
keeping UB/impl-defined lanes explicitly skip-gated in final differential mode.

Promoted lanes:
- `15__torture__clang_gcc_tri_diff_policy_shift_char_matrix`
- `15__torture__clang_gcc_tri_diff_policy_struct_abi_matrix`

Probe lanes:
- `15__probe_runtime_clang_gcc_tri_diff_policy_shift_char_matrix`
- `15__probe_runtime_clang_gcc_tri_diff_policy_struct_abi_matrix`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave32-gcc-tri-diff-policy-crosscheck.json`

Validation:
- `PROBE_FILTER=15__probe_runtime_clang_gcc_tri_diff_policy_shift_char_matrix,15__probe_runtime_clang_gcc_tri_diff_policy_struct_abi_matrix python3 tests/final/probes/run_probes.py`
  -> `resolved=2`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave32-gcc-tri-diff-policy-crosscheck.json`
  -> all `2` pass
- `make final-wave WAVE=32 WAVE_BUCKET=15-torture-differential`
  -> all `2` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 10 skipped` (`113` active tests)

## Wave 33: Malformed Header-Chain DiagJSON Expansion (Completed)

Goal: extend malformed-header deterministic no-crash coverage with additional
structured diagnostics parity lanes.

Promoted lanes:
- `15__diagjson__malformed_header_macro_cycle_chain_no_crash`
- `15__torture__malformed_header_guard_tail_mismatch_chain_no_crash`
- `15__diagjson__malformed_header_guard_tail_mismatch_chain_no_crash`

Probe lanes:
- `15__probe_diag_malformed_header_macro_cycle_chain_no_crash`
- `15__probe_diagjson_malformed_header_macro_cycle_chain_no_crash`
- `15__probe_diag_malformed_header_guard_tail_mismatch_chain_no_crash`
- `15__probe_diagjson_malformed_header_guard_tail_mismatch_chain_no_crash`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave33-malformed-header-chain-diagjson-expansion.json`

Validation:
- `PROBE_FILTER=15__probe_diag_malformed_header_macro_cycle_chain_no_crash,15__probe_diagjson_malformed_header_macro_cycle_chain_no_crash,15__probe_diag_malformed_header_guard_tail_mismatch_chain_no_crash,15__probe_diagjson_malformed_header_guard_tail_mismatch_chain_no_crash python3 tests/final/probes/run_probes.py`
  -> `resolved=4`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave33-malformed-header-chain-diagjson-expansion.json`
  -> all `3` pass
- `make final-wave WAVE=33 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 10 skipped` (`116` active tests)

## Wave 34: Optional GCC Tri-Diff Header/Control Matrix (Completed)

Goal: add an additional optional gcc tri-diff pair focused on
header-influenced control/dataflow surfaces with deterministic runtime outputs.

Promoted lanes:
- `15__torture__clang_gcc_tri_diff_header_control_dispatch_matrix`
- `15__torture__clang_gcc_tri_diff_header_layout_fold_matrix`

Probe lanes:
- `15__probe_runtime_clang_gcc_tri_diff_header_control_dispatch_matrix`
- `15__probe_runtime_clang_gcc_tri_diff_header_layout_fold_matrix`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave34-gcc-tri-diff-header-control-matrix.json`

Validation:
- `PROBE_FILTER=15__probe_runtime_clang_gcc_tri_diff_header_control_dispatch_matrix,15__probe_runtime_clang_gcc_tri_diff_header_layout_fold_matrix python3 tests/final/probes/run_probes.py`
  -> `resolved=2`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave34-gcc-tri-diff-header-control-matrix.json`
  -> all `2` pass
- `make final-wave WAVE=34 WAVE_BUCKET=15-torture-differential`
  -> all `2` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 10 skipped` (`118` active tests)

## Wave 35: Malformed Include-Graph DiagJSON Follow-up (Completed)

Goal: extend malformed include-graph no-crash coverage with additional
structured diagnostics parity checks on multi-file include edge chains.

Promoted lanes:
- `15__torture__malformed_include_graph_guard_collision_no_crash`
- `15__diagjson__malformed_include_graph_guard_collision_no_crash`
- `15__diagjson__malformed_include_graph_macro_arity_chain_no_crash`

Probe lanes:
- `15__probe_diag_malformed_include_graph_guard_collision_no_crash`
- `15__probe_diagjson_malformed_include_graph_guard_collision_no_crash`
- `15__probe_diagjson_malformed_include_graph_macro_arity_chain_no_crash`

Promoted shard:
- `tests/final/meta/15-torture-differential-wave35-malformed-include-graph-diagjson-followup.json`

Validation:
- `PROBE_FILTER=15__probe_diag_malformed_include_graph_guard_collision_no_crash,15__probe_diagjson_malformed_include_graph_guard_collision_no_crash,15__probe_diagjson_malformed_include_graph_macro_arity_chain_no_crash python3 tests/final/probes/run_probes.py`
  -> `resolved=3`, `blocked=0`, `skipped=0`
- `make final-manifest MANIFEST=15-torture-differential-wave35-malformed-include-graph-diagjson-followup.json`
  -> all `3` pass
- `make final-wave WAVE=35 WAVE_BUCKET=15-torture-differential`
  -> all `3` pass
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 10 skipped` (`121` active tests)

## Planned Expansion Waves (36-40)

These five waves map 1:1 to the five unresolved stress areas from the stocktake.

## Wave 36: Stress-Ceiling Frontier Sweep

Goal: add explicit graded frontier lanes for recursion/declaration/aggregate
size pressure with deterministic outputs and bounded runtime.

Status: completed (2026-04-01).

Planned lanes:
- `15__torture__ceiling_recursion_depth_sweep`
- `15__torture__ceiling_decl_density_sweep`
- `15__torture__ceiling_aggregate_size_sweep`
- `15__torture__ceiling_multitu_layout_pressure_sweep`

Planned probes:
- `15__probe_ceiling_recursion_depth_sweep`
- `15__probe_ceiling_decl_density_sweep`
- `15__probe_ceiling_aggregate_size_sweep`
- `15__probe_ceiling_multitu_layout_pressure_sweep`

Planned shard:
- `tests/final/meta/15-torture-differential-wave36-stress-ceiling-frontier-sweep.json`

Validation:
- `PROBE_FILTER=15__probe_ceiling_ python3 tests/final/probes/run_probes.py`
  -> `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-manifest MANIFEST=15-torture-differential-wave36-stress-ceiling-frontier-sweep.json`
  -> all `4` pass.
- `make final-wave WAVE=36 WAVE_BUCKET=15-torture-differential`
  -> all `4` pass.
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 10 skipped` (`125` active tests).

## Wave 37: Fuzz-Volume Deterministic Replay

Goal: move from fuzz smoke to bounded replay-volume lanes with fixed seed sets,
deterministic reductions, and no-crash guarantees.

Status: completed (2026-04-01).

Planned lanes:
- `15__torture__fuzz_seeded_expr_volume_replay`
- `15__torture__fuzz_seeded_stmt_volume_replay`
- `15__torture__fuzz_seeded_malformed_volume_replay_no_crash`
- `15__diagjson__fuzz_seeded_malformed_volume_replay_no_crash`

Planned probes:
- `15__probe_fuzz_seeded_expr_volume_replay`
- `15__probe_fuzz_seeded_stmt_volume_replay`
- `15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash`
- `15__probe_diagjson_fuzz_seeded_malformed_volume_replay_no_crash`

Planned shard:
- `tests/final/meta/15-torture-differential-wave37-fuzz-volume-deterministic-replay.json`

Validation:
- `PROBE_FILTER=15__probe_fuzz_seeded_expr_volume_replay,15__probe_fuzz_seeded_stmt_volume_replay,15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash,15__probe_diagjson_fuzz_seeded_malformed_volume_replay_no_crash python3 tests/final/probes/run_probes.py`
  -> `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-manifest MANIFEST=15-torture-differential-wave37-fuzz-volume-deterministic-replay.json`
  -> all `4` pass.
- `make final-wave WAVE=37 WAVE_BUCKET=15-torture-differential`
  -> all `4` pass.
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 10 skipped` (`129` active tests).

## Wave 38: GCC Tri-Reference Breadth Expansion

Goal: widen gcc optional tri-reference coverage across additional runtime classes
so tri-reference coverage is less concentrated in one family.

Status: completed (2026-04-01).

Planned lanes:
- `15__torture__clang_gcc_tri_diff_control_flow_lattice_matrix`
- `15__torture__clang_gcc_tri_diff_abi_variadic_regstack_matrix`
- `15__torture__clang_gcc_tri_diff_vla_stride_rebase_matrix`
- `15__torture__clang_gcc_tri_diff_struct_union_layout_bridge`

Planned probes:
- `15__probe_runtime_clang_gcc_tri_diff_control_flow_lattice_matrix`
- `15__probe_runtime_clang_gcc_tri_diff_abi_variadic_regstack_matrix`
- `15__probe_runtime_clang_gcc_tri_diff_vla_stride_rebase_matrix`
- `15__probe_runtime_clang_gcc_tri_diff_struct_union_layout_bridge`

Planned shard:
- `tests/final/meta/15-torture-differential-wave38-gcc-tri-diff-breadth-expansion.json`

Validation:
- `PROBE_FILTER=15__probe_runtime_clang_gcc_tri_diff_control_flow_lattice_matrix,15__probe_runtime_clang_gcc_tri_diff_abi_variadic_regstack_matrix,15__probe_runtime_clang_gcc_tri_diff_vla_stride_rebase_matrix,15__probe_runtime_clang_gcc_tri_diff_struct_union_layout_bridge python3 tests/final/probes/run_probes.py`
  -> `resolved=4`, `blocked=0`, `skipped=0`.
- `make final-manifest MANIFEST=15-torture-differential-wave38-gcc-tri-diff-breadth-expansion.json`
  -> all `4` pass.
- `make final-wave WAVE=38 WAVE_BUCKET=15-torture-differential`
  -> all `4` pass.
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 10 skipped` (`133` active tests).

## Wave 39: Compile-Surface Stress Expansion

Goal: increase non-runtime stress depth with additional pathological AST/semantic
surface lanes and paired structured diagnostics checks.

Status: completed (2026-04-01).

Planned lanes:
- `15__torture__pathological_decl_graph_surface`
- `15__torture__pathological_initializer_rewrite_surface_reject`
- `15__torture__pathological_switch_case_surface_reject`
- `15__diagjson__pathological_initializer_rewrite_surface_reject`
- `15__diagjson__pathological_switch_case_surface_reject`

Planned probes:
- `15__probe_ast_pathological_decl_graph_surface`
- `15__probe_diag_pathological_initializer_rewrite_surface_reject`
- `15__probe_diag_pathological_switch_case_surface_reject`
- `15__probe_diagjson_pathological_initializer_rewrite_surface_reject`
- `15__probe_diagjson_pathological_switch_case_surface_reject`

Planned shard:
- `tests/final/meta/15-torture-differential-wave39-compile-surface-expansion.json`

Validation:
- `PROBE_FILTER=15__probe_ast_pathological_decl_graph_surface,15__probe_diag_pathological_initializer_rewrite_surface_reject,15__probe_diag_pathological_switch_case_surface_reject,15__probe_diagjson_pathological_initializer_rewrite_surface_reject,15__probe_diagjson_pathological_switch_case_surface_reject python3 tests/final/probes/run_probes.py`
  -> `resolved=5`, `blocked=0`, `skipped=0`.
- `make final-manifest MANIFEST=15-torture-differential-wave39-compile-surface-expansion.json`
  -> all `5` pass.
- `make final-wave WAVE=39 WAVE_BUCKET=15-torture-differential`
  -> all `5` pass.
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 10 skipped` (`138` active tests).

## Wave 40: External-Corpus Pinned Bundle Expansion

Goal: expand from small curated fragments to a larger pinned corpus bundle while
keeping deterministic, CI-bounded, reviewable behavior.

Status: completed (2026-04-01).

Planned lanes:
- `15__torture__corpus_pinned_fragment_e_smoke`
- `15__torture__corpus_pinned_fragment_f_smoke`
- `15__torture__corpus_pinned_fragment_g_smoke`
- `15__torture__corpus_pinned_macro_include_chain_reject`
- `15__torture__corpus_pinned_typedef_decl_cycle_reject`
- `15__diagjson__corpus_pinned_macro_include_chain_reject`
- `15__diagjson__corpus_pinned_typedef_decl_cycle_reject`

Planned probes:
- `15__probe_corpus_pinned_fragment_e_smoke`
- `15__probe_corpus_pinned_fragment_f_smoke`
- `15__probe_corpus_pinned_fragment_g_smoke`
- `15__probe_diag_corpus_pinned_macro_include_chain_reject`
- `15__probe_diag_corpus_pinned_typedef_decl_cycle_reject`
- `15__probe_diagjson_corpus_pinned_macro_include_chain_reject`
- `15__probe_diagjson_corpus_pinned_typedef_decl_cycle_reject`

Planned shard:
- `tests/final/meta/15-torture-differential-wave40-external-corpus-pinned-bundle.json`

Validation:
- `PROBE_FILTER=15__probe_corpus_pinned_fragment_e_smoke,15__probe_corpus_pinned_fragment_f_smoke,15__probe_corpus_pinned_fragment_g_smoke,15__probe_diag_corpus_pinned_macro_include_chain_reject,15__probe_diag_corpus_pinned_typedef_decl_cycle_reject,15__probe_diagjson_corpus_pinned_macro_include_chain_reject,15__probe_diagjson_corpus_pinned_typedef_decl_cycle_reject python3 tests/final/probes/run_probes.py`
  -> `resolved=7`, `blocked=0`, `skipped=0`.
- `make final-manifest MANIFEST=15-torture-differential-wave40-external-corpus-pinned-bundle.json`
  -> all `7` pass.
- `make final-wave WAVE=40 WAVE_BUCKET=15-torture-differential`
  -> all `7` pass.
- `make final-bucket BUCKET=torture-differential`
  -> `0 failing, 10 skipped` (`145` active tests).

## Recommended Run Order

1. Final verify: `make final-bucket BUCKET=torture-differential` and `make final`.

## Execution Rules

1. Probe first, gather full blocker set for the wave.
2. Fix blockers in grouped batches (not one-off churn).
3. Promote only passing lanes to wave shard.
4. Validate each wave with:
   - `make final-manifest MANIFEST=<wave-shard>.json`
   - `make final-bucket BUCKET=torture-differential`
   - `make final` before major commit boundaries

Wave-specific selector:
- `make final-wave WAVE=<n> WAVE_BUCKET=15-torture-differential`

## Exit Criteria For Bucket-15 Stable

- No blocked `15__probe_*` lanes.
- All active `15__` tests passing in `make final-bucket BUCKET=torture-differential`.
- Malformed robustness includes both text diagnostics and selected `.diagjson` parity.
- At least one stable multi-TU torture lane and one resource-pressure lane.
