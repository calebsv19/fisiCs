# Torture / Differential

## Status Snapshot (2026-04-01)
- Active tests: `145` (`tests/final/meta/15-torture-differential*.json`)
- Runtime differential lanes: `81`
- Compile-surface stress lanes: `2` (plus broad negative compile-surface lanes)
- Malformed-input negative lanes: `35`
- `.diagjson` parity lanes: `27`
- Multi-TU linkage-negative lanes: `3` (plus additional multi-TU stress positives)
- External-corpus lanes: expanded through Wave40 pinned bundle
- Policy-tagged lanes: `10` (`5` impl-defined + `5` ub)
- Probe snapshot:
  - `PROBE_FILTER=15__probe_*` -> `resolved=136`, `blocked=0`, `skipped=0`
  - `make final-bucket BUCKET=torture-differential` -> `0 failing, 10 skipped` (`145` active)
  - Skip policy: `10` lanes are intentionally differential-skipped (`ub=true` or `impl_defined=true`)
- Next expansion plan: `docs/plans/torture_bucket_15_execution_plan.md`

## Scope
Stress parser, semantic, and runtime paths with large or pathological shapes.

## Validate
- Long-expression parsing and lowering under heavy operator chains.
- Deeply nested statement handling.
- Many declarations in a single scope.
- Large aggregate layout and access behavior.
- Pathological declarator parsing in non-trivial function-pointer forms.
- Malformed input robustness (diagnose, fail closed, no crash).

## Acceptance Checklist
- Runtime stress tests compile, execute, and match expected stdout/stderr.
- Differential checks against clang run when available.
- ABI-sensitive checks are tagged explicitly.
- Malformed input tests report deterministic diagnostics and do not crash.

## Test Cases (initial list)
1) `15__torture__long_expr`
   - 100-term arithmetic expression runtime result.
2) `15__torture__deep_nesting`
   - 12-level nested `if` chain runtime result.
3) `15__torture__many_decls`
   - High local declaration count and aggregate usage.
4) `15__torture__many_globals`
   - High global declaration density and runtime aggregate usage.
5) `15__torture__many_params`
   - Multi-parameter call ABI path stress.
6) `15__torture__large_struct`
   - Larger struct field layout/readback runtime behavior.
7) `15__torture__large_array_stress`
   - Larger local-array and indexed-access stress path.
8) `15__torture__pathological_decl`
   - Complex function-pointer typedef/declarator parse path.
9) `15__torture__path_decl_nested`
   - Runtime nested function-pointer declarator/call path (`choose` + indirect call).
10) `15__torture__malformed_no_crash`
   - Invalid source characters/syntax produce diagnostics without crashing.
11) `15__torture__malformed_unclosed_comment_no_crash`
   - Unterminated block-comment path reports lexer diagnostic without crashing.
12) `15__torture__malformed_unbalanced_block_no_crash`
   - Unbalanced block recovery path reports parser diagnostics without crashing.

## Open Gaps (Tracked)
- No open blockers at current baseline.
- Optional expansion backlog:
  - broader external-corpus depth beyond current pinned fragment bundle,
  - tri-reference differential breadth (gcc optional) is still a subset of total runtime lanes,
  - randomized high-volume fuzz campaign infrastructure beyond deterministic replay lanes,
  - additional implementation-defined ABI policy surfaces beyond current tagged set,
  - more compile-surface-only stress lanes to further balance runtime-heavy coverage.

## Next Wave Targets

### Wave 1: Control + Declarator Depth Expansion
Added runtime torture lanes that increase control-path and declarator complexity:
- `15__torture__deep_switch_loop_state_machine`
- `15__torture__switch_sparse_case_jump_table`
- `15__torture__declarator_depth_runtime_chain`

Wave1 status:
- Targeted probe run:
  `resolved=3`, `blocked=0`, `skipped=0`.
- `make final-manifest MANIFEST=15-torture-differential-wave1-control-decl-depth.json`:
  all 3 Wave1 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => all `15` pass.

### Wave 2: Malformed Robustness Hardening
Wave2 promoted lanes:
- `15__torture__malformed_unterminated_string_no_crash`
- `15__torture__malformed_bad_escape_no_crash`
- `15__torture__malformed_pp_directive_no_crash`
- `15__diagjson__malformed_unbalanced_block_no_crash`
- `15__diagjson__malformed_unclosed_comment_no_crash`

Wave2 status:
- Targeted probe run:
  `resolved=5`, `blocked=0`, `skipped=0` for `15__probe_diag_*` + `15__probe_diagjson_*`.
- `make final-manifest MANIFEST=15-torture-differential-wave2-malformed-diagjson.json`:
  all 5 Wave2 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => all `20` pass.

Wave2 blocker that was closed:
- `diagjson` was empty on lexer-fatal paths (unterminated block comment) because
  lexer errors were stderr-only and not recorded in compiler diagnostics.
- Fixed by bridging lexer fatal errors into compiler diagnostics so
  `--emit-diags-json` remains populated on fail-closed lexer paths.

### Wave 3: Multi-TU Torture Surface
Wave3 promoted lanes:
- `15__torture__multitu_many_globals_crossref`
- `15__torture__multitu_fnptr_dispatch_grid`
- `15__torture__multitu_duplicate_symbol_reject`
- `15__diagjson__multitu_duplicate_symbol_reject`

Wave3 status:
- Targeted probes:
  - `15__probe_multitu_many_globals_crossref` -> resolved
  - `15__probe_multitu_fnptr_dispatch_grid` -> resolved
  - `15__probe_diag_multitu_duplicate_symbol_reject` -> resolved
  - `15__probe_diagjson_multitu_duplicate_symbol_reject` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave3-multitu.json`:
  all 4 Wave3 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => all `24` pass.

### Wave 4: Resource Pressure Lanes
Wave4 promoted lanes:
- `15__torture__deep_recursion_stack_pressure`
- `15__torture__large_vla_stride_pressure`
- `15__torture__many_args_regstack_pressure`

Wave4 status:
- Targeted probes:
  - `15__probe_deep_recursion_stack_pressure` -> resolved
  - `15__probe_large_vla_stride_pressure` -> resolved
  - `15__probe_many_args_regstack_pressure` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave4-pressure.json`:
  all 3 Wave4 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => all `27` pass.

### Wave 5: Fuzz/Corpus Integration Hooks
Wave5 promoted lanes:
- `15__torture__seeded_expr_fuzz_smoke`
- `15__torture__seeded_stmt_fuzz_smoke`
- `15__torture__corpus_micro_compile_smoke`

Wave5 status:
- Targeted probes:
  - `15__probe_seeded_expr_fuzz_smoke` -> resolved
  - `15__probe_seeded_stmt_fuzz_smoke` -> resolved
  - `15__probe_corpus_micro_compile_smoke` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave5-fuzz-corpus-hooks.json`:
  all 3 Wave5 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => all `30` pass.

### Wave 6: Control/Layout + Multi-TU Const-Table Expansion
Wave6 promoted lanes:
- `15__torture__control_rebind_dispatch_lattice`
- `15__torture__large_struct_array_checksum_grid`
- `15__torture__multitu_const_table_crc`

Wave6 status:
- Targeted probes:
  - `15__probe_control_rebind_dispatch_lattice` -> resolved
  - `15__probe_large_struct_array_checksum_grid` -> resolved
  - `15__probe_multitu_const_table_crc` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave6-control-layout-multitu.json`:
  all 3 Wave6 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => all `33` pass.

### Wave 7: External Corpus Deterministic Lane
Wave7 promoted lanes:
- `15__torture__corpus_external_compile_smoke`
- `15__torture__corpus_external_compile_reject`

Wave7 status:
- Targeted probes:
  - `15__probe_corpus_external_compile_smoke` -> resolved
  - `15__probe_diag_corpus_external_compile_reject` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave7-external-corpus.json`:
  all 2 Wave7 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => all `35` pass.

### Wave 8: Compile-Surface Torture Expansion
Wave8 promoted lanes:
- `15__torture__pathological_type_graph`
- `15__torture__pathological_initializer_shape_reject`
- `15__diagjson__pathological_initializer_shape_reject`

Wave8 status:
- Targeted probes:
  - `15__probe_ast_pathological_type_graph` -> resolved
  - `15__probe_diag_pathological_initializer_shape_reject` -> resolved
  - `15__probe_diagjson_pathological_initializer_shape_reject` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave8-compile-surface.json`:
  all 3 Wave8 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => all `38` pass.

### Wave 9: Malformed Fuzz-Smoke Matrix
Wave9 promoted lanes:
- `15__torture__malformed_token_stream_seeded_a_no_crash`
- `15__torture__malformed_token_stream_seeded_b_no_crash`
- `15__diagjson__malformed_token_stream_seeded_a_no_crash`

Wave9 status:
- Targeted probes:
  - `15__probe_diag_malformed_token_stream_seeded_a_no_crash` -> resolved
  - `15__probe_diag_malformed_token_stream_seeded_b_no_crash` -> resolved
  - `15__probe_diagjson_malformed_token_stream_seeded_a_no_crash` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave9-malformed-fuzz-matrix.json`:
  all 3 Wave9 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => all `41` pass.

### Wave 10: Optional GCC Differential Lane
Wave10 promoted lanes:
- `15__torture__clang_gcc_tri_diff_smoke`

Wave10 status:
- Targeted probes:
  - `15__probe_runtime_clang_gcc_tri_diff_smoke` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave10-gcc-optional-diff.json`:
  all 1 Wave10 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => all `42` pass.

### Wave 11: UB / Impl-Defined Policy Tagging
Wave11 promoted lanes:
- `15__torture__policy_signed_shift_impl_defined_tagged`
- `15__torture__policy_alias_ub_tagged`

Wave11 status:
- Targeted probes:
  - `15__probe_policy_signed_shift_impl_defined_tagged` -> resolved
  - `15__probe_policy_alias_ub_tagged` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave11-policy-tags.json`:
  `0 failing, 2 skipped` (policy skip-gated differential).
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`44` active).

### Wave 12: Malformed Matrix DiagJSON Parity Expansion
Wave12 promoted lanes:
- `15__torture__malformed_token_stream_seeded_c_no_crash`
- `15__diagjson__malformed_token_stream_seeded_b_no_crash`
- `15__diagjson__malformed_token_stream_seeded_c_no_crash`

Wave12 status:
- Targeted probes:
  - `15__probe_diag_malformed_token_stream_seeded_c_no_crash` -> resolved
  - `15__probe_diagjson_malformed_token_stream_seeded_b_no_crash` -> resolved
  - `15__probe_diagjson_malformed_token_stream_seeded_c_no_crash` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave12-malformed-diagjson-parity.json`:
  all 3 Wave12 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`47` active).

### Wave 13: Multi-TU Stress Expansion
Wave13 promoted lanes:
- `15__torture__multitu_fnptr_state_matrix`
- `15__torture__multitu_struct_vla_stride_bridge`
- `15__torture__multitu_ring_digest_pipeline`

Wave13 status:
- Targeted probes:
  - `15__probe_multitu_fnptr_state_matrix` -> resolved
  - `15__probe_multitu_struct_vla_stride_bridge` -> resolved
  - `15__probe_multitu_ring_digest_pipeline` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave13-multitu-stress-expansion.json`:
  all 3 Wave13 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`50` active).

### Wave 14: Multi-TU Link-Failure Matrix
Wave14 promoted lanes:
- `15__torture__multitu_duplicate_function_definition_reject`
- `15__torture__multitu_symbol_type_conflict_reject`
- `15__diagjson__multitu_duplicate_function_definition_reject`
- `15__diagjson__multitu_symbol_type_conflict_reject`

Wave14 status:
- Targeted probes:
  - `15__probe_diag_multitu_duplicate_function_definition_reject` -> resolved
  - `15__probe_diag_multitu_symbol_type_conflict_reject` -> resolved
  - `15__probe_diagjson_multitu_duplicate_function_definition_reject` -> resolved
  - `15__probe_diagjson_multitu_symbol_type_conflict_reject` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave14-multitu-link-failure-matrix.json`:
  all 4 Wave14 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`54` active).

### Wave 15: Runtime Stress Expansion
Wave15 promoted lanes:
- `15__torture__runtime_vla_fnptr_feedback_matrix`
- `15__torture__runtime_struct_union_reducer_chain`
- `15__torture__multitu_state_reseed_pipeline`

Wave15 status:
- Targeted probes:
  - `15__probe_runtime_vla_fnptr_feedback_matrix` -> resolved
  - `15__probe_runtime_struct_union_reducer_chain` -> resolved
  - `15__probe_multitu_state_reseed_pipeline` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave15-runtime-stress-expansion.json`:
  all 3 Wave15 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`57` active).

### Wave 16: Malformed + DiagJSON Expansion
Wave16 promoted lanes:
- `15__torture__malformed_invalid_dollar_no_crash`
- `15__torture__malformed_char_invalid_hex_escape_no_crash`
- `15__diagjson__malformed_invalid_dollar_no_crash`
- `15__diagjson__malformed_char_invalid_hex_escape_no_crash`

Wave16 status:
- Targeted probes:
  - `15__probe_diag_malformed_invalid_dollar_no_crash` -> resolved
  - `15__probe_diag_malformed_char_invalid_hex_escape_no_crash` -> resolved
  - `15__probe_diagjson_malformed_invalid_dollar_no_crash` -> resolved
  - `15__probe_diagjson_malformed_char_invalid_hex_escape_no_crash` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave16-malformed-diagjson-expansion.json`:
  all 4 Wave16 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`61` active).

### Wave 17: External Corpus Scale-Up
Wave17 promoted lanes:
- `15__torture__corpus_external_parser_fragment_a_smoke`
- `15__torture__corpus_external_parser_fragment_b_smoke`
- `15__torture__corpus_external_macro_guard_reject`

Wave17 status:
- Targeted probes:
  - `15__probe_corpus_external_parser_fragment_a_smoke` -> resolved
  - `15__probe_corpus_external_parser_fragment_b_smoke` -> resolved
  - `15__probe_diag_corpus_external_macro_guard_reject` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave17-external-corpus-scaleup.json`:
  all 3 Wave17 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`64` active).

### Wave 18: Malformed PP + Multi-File Expansion
Wave18 promoted lanes:
- `15__torture__malformed_pp_unterminated_if_chain_no_crash`
- `15__torture__malformed_pp_macro_paste_fragments_no_crash`
- `15__diagjson__malformed_pp_unterminated_if_chain_no_crash`
- `15__diagjson__malformed_pp_macro_paste_fragments_no_crash`

Wave18 status:
- Targeted probes:
  - `15__probe_diag_malformed_pp_unterminated_if_chain_no_crash` -> resolved
  - `15__probe_diag_malformed_pp_macro_paste_fragments_no_crash` -> resolved
  - `15__probe_diagjson_malformed_pp_unterminated_if_chain_no_crash` -> resolved
  - `15__probe_diagjson_malformed_pp_macro_paste_fragments_no_crash` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave18-malformed-pp-multifile.json`:
  all 4 Wave18 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`68` active).

### Wave 19: GCC Tri-Differential Breadth
Wave19 promoted lanes:
- `15__torture__clang_gcc_tri_diff_control_checksum`
- `15__torture__clang_gcc_tri_diff_multitu_state_bridge`
- `15__torture__clang_gcc_tri_diff_abi_args_matrix`

Wave19 status:
- Targeted probes:
  - `15__probe_runtime_clang_gcc_tri_diff_control_checksum` -> resolved
  - `15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge` -> resolved
  - `15__probe_runtime_clang_gcc_tri_diff_abi_args_matrix` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave19-gcc-tri-diff-matrix.json`:
  all 3 Wave19 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`71` active).

### Wave 20: Stress Ceiling Expansion
Wave20 promoted lanes:
- `15__torture__deep_recursion_stack_pressure_ii`
- `15__torture__many_decls_density_pressure_ii`
- `15__torture__large_struct_array_pressure_ii`

Wave20 status:
- Targeted probes:
  - `15__probe_deep_recursion_stack_pressure_ii` -> resolved
  - `15__probe_many_decls_density_pressure_ii` -> resolved
  - `15__probe_large_struct_array_pressure_ii` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave20-stress-ceiling-expansion.json`:
  all 3 Wave20 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`74` active).

### Wave 21: Policy-Tagged Matrix Expansion
Wave21 promoted lanes:
- `15__torture__policy_impldef_signed_shift_matrix_tagged`
- `15__torture__policy_impldef_signed_char_matrix_tagged`
- `15__torture__policy_ub_unsequenced_write_tagged`
- `15__torture__policy_ub_alias_overlap_tagged`

Wave21 status:
- Targeted probes:
  - `15__probe_policy_impldef_signed_shift_matrix_tagged` -> resolved
  - `15__probe_policy_impldef_signed_char_matrix_tagged` -> resolved
  - `15__probe_policy_ub_unsequenced_write_tagged` -> resolved
  - `15__probe_policy_ub_alias_overlap_tagged` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave21-policy-matrix-expansion.json`:
  `0 failing, 4 skipped` (policy skip-gated differential).
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 6 skipped` (`78` active).

### Wave 22: Malformed Matrix Expansion I (PP + Include Graph)
Wave22 promoted lanes:
- `15__torture__malformed_pp_nested_ifdef_chain_seeded_d_no_crash`
- `15__torture__malformed_pp_include_cycle_guarded_no_crash`
- `15__diagjson__malformed_pp_nested_ifdef_chain_seeded_d_no_crash`
- `15__diagjson__malformed_pp_include_cycle_guarded_no_crash`

Wave22 status:
- Targeted probes:
  - `15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash` -> resolved
  - `15__probe_diag_malformed_pp_include_cycle_guarded_no_crash` -> resolved
  - `15__probe_diagjson_malformed_pp_nested_ifdef_chain_seeded_d_no_crash` -> resolved
  - `15__probe_diagjson_malformed_pp_include_cycle_guarded_no_crash` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave22-malformed-pp-include-graph.json`:
  all 4 Wave22 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 6 skipped` (`82` active).

### Wave 23: Malformed Matrix Expansion II (multi-file token chaos)
Wave23 promoted lanes:
- `15__torture__malformed_multifile_header_tail_garbage_no_crash`
- `15__torture__malformed_multifile_macro_arity_mismatch_no_crash`
- `15__diagjson__malformed_multifile_header_tail_garbage_no_crash`
- `15__diagjson__malformed_multifile_macro_arity_mismatch_no_crash`

Wave23 status:
- Targeted probes:
  - `15__probe_diag_malformed_multifile_header_tail_garbage_no_crash` -> resolved
  - `15__probe_diag_malformed_multifile_macro_arity_mismatch_no_crash` -> resolved
  - `15__probe_diagjson_malformed_multifile_header_tail_garbage_no_crash` -> resolved
  - `15__probe_diagjson_malformed_multifile_macro_arity_mismatch_no_crash` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave23-malformed-multifile-token-chaos.json`:
  all 4 Wave23 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 6 skipped` (`86` active).

### Wave 24: Tri-Compiler Differential Breadth I (single-TU)
Wave24 promoted lanes:
- `15__torture__clang_gcc_tri_diff_loop_state_crc_matrix`
- `15__torture__clang_gcc_tri_diff_struct_array_stride_crc_matrix`
- `15__torture__clang_gcc_tri_diff_pointer_mix_checksum_matrix`

Wave24 status:
- Targeted probes:
  - `15__probe_runtime_clang_gcc_tri_diff_loop_state_crc_matrix` -> resolved
  - `15__probe_runtime_clang_gcc_tri_diff_struct_array_stride_crc_matrix` -> resolved
  - `15__probe_runtime_clang_gcc_tri_diff_pointer_mix_checksum_matrix` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave24-gcc-tri-diff-single-tu.json`:
  all 3 Wave24 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 6 skipped` (`89` active).

### Wave 25: Tri-Compiler Differential Breadth II (multi-TU)
Wave25 promoted lanes:
- `15__torture__clang_gcc_tri_diff_multitu_fnptr_table_bridge`
- `15__torture__clang_gcc_tri_diff_multitu_const_seed_pipeline`
- `15__torture__clang_gcc_tri_diff_multitu_layout_digest_bridge`

Wave25 status:
- Targeted probes:
  - `15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge` -> resolved
  - `15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline` -> resolved
  - `15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave25-gcc-tri-diff-multitu.json`:
  all 3 Wave25 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 6 skipped` (`92` active).

### Wave 26: Stress Ceiling Expansion III
Wave26 promoted lanes:
- `15__torture__deep_recursion_stack_pressure_iii`
- `15__torture__many_decls_density_pressure_iii`
- `15__torture__large_struct_array_pressure_iii`

Wave26 status:
- Targeted probes:
  - `15__probe_deep_recursion_stack_pressure_iii` -> resolved
  - `15__probe_many_decls_density_pressure_iii` -> resolved
  - `15__probe_large_struct_array_pressure_iii` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave26-stress-ceiling-expansion-iii.json`:
  all 3 Wave26 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 6 skipped` (`95` active).

### Wave 27: Stress Ceiling Expansion IV (multi-TU pressure)
Wave27 promoted lanes:
- `15__torture__multitu_large_table_crc_pressure`
- `15__torture__multitu_recursive_dispatch_pressure`
- `15__torture__multitu_layout_stride_pressure`

Wave27 status:
- Targeted probes:
  - `15__probe_multitu_large_table_crc_pressure` -> resolved
  - `15__probe_multitu_recursive_dispatch_pressure` -> resolved
  - `15__probe_multitu_layout_stride_pressure` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave27-multitu-stress-ceiling-iv.json`:
  all 3 Wave27 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 6 skipped` (`98` active).

### Wave 28: External Corpus Scale-Up I
Wave28 promoted lanes:
- `15__torture__corpus_external_parser_fragment_c_smoke`
- `15__torture__corpus_external_parser_fragment_d_smoke`
- `15__torture__corpus_external_macro_chain_reject`

Wave28 status:
- Targeted probes:
  - `15__probe_corpus_external_parser_fragment_c_smoke` -> resolved
  - `15__probe_corpus_external_parser_fragment_d_smoke` -> resolved
  - `15__probe_diag_corpus_external_macro_chain_reject` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave28-external-corpus-scaleup-i.json`:
  all 3 Wave28 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 6 skipped` (`101` active).

### Wave 29: External Corpus Scale-Up II
Wave29 promoted lanes:
- `15__torture__corpus_external_decl_chain_smoke`
- `15__torture__corpus_external_typedef_graph_smoke`
- `15__torture__corpus_external_include_guard_mismatch_reject`

Wave29 status:
- Targeted probes:
  - `15__probe_corpus_external_decl_chain_smoke` -> resolved
  - `15__probe_corpus_external_typedef_graph_smoke` -> resolved
  - `15__probe_diag_corpus_external_include_guard_mismatch_reject` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave29-external-corpus-scaleup-ii.json`:
  all 3 Wave29 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 6 skipped` (`104` active).

### Wave 30: Policy Matrix Expansion II
Wave30 promoted lanes:
- `15__torture__policy_impldef_signed_shift_width_matrix_tagged`
- `15__torture__policy_impldef_char_promotion_matrix_tagged`
- `15__torture__policy_ub_signed_overflow_chain_tagged`
- `15__torture__policy_ub_eval_order_call_sidefx_tagged`

Wave30 status:
- Targeted probes:
  - `15__probe_policy_impldef_signed_shift_width_matrix_tagged` -> resolved
  - `15__probe_policy_impldef_char_promotion_matrix_tagged` -> resolved
  - `15__probe_policy_ub_signed_overflow_chain_tagged` -> resolved
  - `15__probe_policy_ub_eval_order_call_sidefx_tagged` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave30-policy-matrix-expansion-ii.json`:
  `0 failing, 4 skipped` (policy skip-gated differential).
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped` (`108` active).

### Wave 31: Malformed Header-Surface Parity Follow-up
Wave31 promoted lanes:
- `15__torture__malformed_header_guard_tail_mismatch_no_crash`
- `15__torture__malformed_header_macro_cycle_chain_no_crash`
- `15__diagjson__malformed_header_guard_tail_mismatch_no_crash`

Wave31 status:
- Targeted probes:
  - `15__probe_diag_malformed_header_guard_tail_mismatch_no_crash` -> resolved
  - `15__probe_diag_malformed_header_macro_cycle_chain_no_crash` -> resolved
  - `15__probe_diagjson_malformed_header_guard_tail_mismatch_no_crash` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave31-malformed-header-surface-parity.json`:
  all 3 Wave31 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped` (`111` active).

### Wave 32: Optional GCC Tri-Diff Policy Crosscheck
Wave32 promoted lanes:
- `15__torture__clang_gcc_tri_diff_policy_shift_char_matrix`
- `15__torture__clang_gcc_tri_diff_policy_struct_abi_matrix`

Wave32 status:
- Targeted probes:
  - `15__probe_runtime_clang_gcc_tri_diff_policy_shift_char_matrix` -> resolved
  - `15__probe_runtime_clang_gcc_tri_diff_policy_struct_abi_matrix` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave32-gcc-tri-diff-policy-crosscheck.json`:
  all 2 Wave32 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped` (`113` active).

### Wave 33: Malformed Header-Chain DiagJSON Expansion
Wave33 promoted lanes:
- `15__diagjson__malformed_header_macro_cycle_chain_no_crash`
- `15__torture__malformed_header_guard_tail_mismatch_chain_no_crash`
- `15__diagjson__malformed_header_guard_tail_mismatch_chain_no_crash`

Wave33 status:
- Targeted probes:
  - `15__probe_diag_malformed_header_macro_cycle_chain_no_crash` -> resolved
  - `15__probe_diagjson_malformed_header_macro_cycle_chain_no_crash` -> resolved
  - `15__probe_diag_malformed_header_guard_tail_mismatch_chain_no_crash` -> resolved
  - `15__probe_diagjson_malformed_header_guard_tail_mismatch_chain_no_crash` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave33-malformed-header-chain-diagjson-expansion.json`:
  all 3 Wave33 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped` (`116` active).

### Wave 34: Optional GCC Tri-Diff Header/Control Matrix
Wave34 promoted lanes:
- `15__torture__clang_gcc_tri_diff_header_control_dispatch_matrix`
- `15__torture__clang_gcc_tri_diff_header_layout_fold_matrix`

Wave34 status:
- Targeted probes:
  - `15__probe_runtime_clang_gcc_tri_diff_header_control_dispatch_matrix` -> resolved
  - `15__probe_runtime_clang_gcc_tri_diff_header_layout_fold_matrix` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave34-gcc-tri-diff-header-control-matrix.json`:
  all 2 Wave34 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped` (`118` active).

### Wave 35: Malformed Include-Graph DiagJSON Follow-up
Wave35 promoted lanes:
- `15__torture__malformed_include_graph_guard_collision_no_crash`
- `15__diagjson__malformed_include_graph_guard_collision_no_crash`
- `15__diagjson__malformed_include_graph_macro_arity_chain_no_crash`

Wave35 status:
- Targeted probes:
  - `15__probe_diag_malformed_include_graph_guard_collision_no_crash` -> resolved
  - `15__probe_diagjson_malformed_include_graph_guard_collision_no_crash` -> resolved
  - `15__probe_diagjson_malformed_include_graph_macro_arity_chain_no_crash` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave35-malformed-include-graph-diagjson-followup.json`:
  all 3 Wave35 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped` (`121` active).

### Wave 36: Stress-Ceiling Frontier Sweep
Wave36 promoted lanes:
- `15__torture__ceiling_recursion_depth_sweep`
- `15__torture__ceiling_decl_density_sweep`
- `15__torture__ceiling_aggregate_size_sweep`
- `15__torture__ceiling_multitu_layout_pressure_sweep`

Wave36 status:
- Targeted probes:
  - `15__probe_ceiling_recursion_depth_sweep` -> resolved
  - `15__probe_ceiling_decl_density_sweep` -> resolved
  - `15__probe_ceiling_aggregate_size_sweep` -> resolved
  - `15__probe_ceiling_multitu_layout_pressure_sweep` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave36-stress-ceiling-frontier-sweep.json`:
  all 4 Wave36 tests pass.
- `make final-wave WAVE=36 WAVE_BUCKET=15-torture-differential`:
  all 4 Wave36 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped` (`125` active).

### Wave 37: Fuzz-Volume Deterministic Replay
Wave37 promoted lanes:
- `15__torture__fuzz_seeded_expr_volume_replay`
- `15__torture__fuzz_seeded_stmt_volume_replay`
- `15__torture__fuzz_seeded_malformed_volume_replay_no_crash`
- `15__diagjson__fuzz_seeded_malformed_volume_replay_no_crash`

Wave37 status:
- Targeted probes:
  - `15__probe_fuzz_seeded_expr_volume_replay` -> resolved
  - `15__probe_fuzz_seeded_stmt_volume_replay` -> resolved
  - `15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash` -> resolved
  - `15__probe_diagjson_fuzz_seeded_malformed_volume_replay_no_crash` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave37-fuzz-volume-deterministic-replay.json`:
  all 4 Wave37 tests pass.
- `make final-wave WAVE=37 WAVE_BUCKET=15-torture-differential`:
  all 4 Wave37 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped` (`129` active).

### Wave 38: GCC Tri-Reference Breadth Expansion
Wave38 promoted lanes:
- `15__torture__clang_gcc_tri_diff_control_flow_lattice_matrix`
- `15__torture__clang_gcc_tri_diff_abi_variadic_regstack_matrix`
- `15__torture__clang_gcc_tri_diff_vla_stride_rebase_matrix`
- `15__torture__clang_gcc_tri_diff_struct_union_layout_bridge`

Wave38 status:
- Targeted probes:
  - `15__probe_runtime_clang_gcc_tri_diff_control_flow_lattice_matrix` -> resolved
  - `15__probe_runtime_clang_gcc_tri_diff_abi_variadic_regstack_matrix` -> resolved
  - `15__probe_runtime_clang_gcc_tri_diff_vla_stride_rebase_matrix` -> resolved
  - `15__probe_runtime_clang_gcc_tri_diff_struct_union_layout_bridge` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave38-gcc-tri-diff-breadth-expansion.json`:
  all 4 Wave38 tests pass.
- `make final-wave WAVE=38 WAVE_BUCKET=15-torture-differential`:
  all 4 Wave38 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped` (`133` active).

### Wave 39: Compile-Surface Stress Expansion
Wave39 promoted lanes:
- `15__torture__pathological_decl_graph_surface`
- `15__torture__pathological_initializer_rewrite_surface_reject`
- `15__torture__pathological_switch_case_surface_reject`
- `15__diagjson__pathological_initializer_rewrite_surface_reject`
- `15__diagjson__pathological_switch_case_surface_reject`

Wave39 status:
- Targeted probes:
  - `15__probe_ast_pathological_decl_graph_surface` -> resolved
  - `15__probe_diag_pathological_initializer_rewrite_surface_reject` -> resolved
  - `15__probe_diag_pathological_switch_case_surface_reject` -> resolved
  - `15__probe_diagjson_pathological_initializer_rewrite_surface_reject` -> resolved
  - `15__probe_diagjson_pathological_switch_case_surface_reject` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave39-compile-surface-expansion.json`:
  all 5 Wave39 tests pass.
- `make final-wave WAVE=39 WAVE_BUCKET=15-torture-differential`:
  all 5 Wave39 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped` (`138` active).

### Wave 40: External-Corpus Pinned Bundle Expansion
Wave40 promoted lanes:
- `15__torture__corpus_pinned_fragment_e_smoke`
- `15__torture__corpus_pinned_fragment_f_smoke`
- `15__torture__corpus_pinned_fragment_g_smoke`
- `15__torture__corpus_pinned_macro_include_chain_reject`
- `15__torture__corpus_pinned_typedef_decl_cycle_reject`
- `15__diagjson__corpus_pinned_macro_include_chain_reject`
- `15__diagjson__corpus_pinned_typedef_decl_cycle_reject`

Wave40 status:
- Targeted probes:
  - `15__probe_corpus_pinned_fragment_e_smoke` -> resolved
  - `15__probe_corpus_pinned_fragment_f_smoke` -> resolved
  - `15__probe_corpus_pinned_fragment_g_smoke` -> resolved
  - `15__probe_diag_corpus_pinned_macro_include_chain_reject` -> resolved
  - `15__probe_diag_corpus_pinned_typedef_decl_cycle_reject` -> resolved
  - `15__probe_diagjson_corpus_pinned_macro_include_chain_reject` -> resolved
  - `15__probe_diagjson_corpus_pinned_typedef_decl_cycle_reject` -> resolved
- `make final-manifest MANIFEST=15-torture-differential-wave40-external-corpus-pinned-bundle.json`:
  all 7 Wave40 tests pass.
- `make final-wave WAVE=40 WAVE_BUCKET=15-torture-differential`:
  all 7 Wave40 tests pass.
- Full bucket-15 slice:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped` (`145` active).

### Next Wave Targets
- Bucket-15 wave campaign complete through Wave40.
- Final gate before commit boundary:
  - `make final-bucket BUCKET=torture-differential`
  - `make final`

See detailed execution order and shard names in:
`docs/plans/torture_bucket_15_execution_plan.md`.

## Execution Flow
1. Add probe lanes first (`15__probe_*`) and collect full blocker set.
2. Fix blockers in grouped batches.
3. Promote passing lanes into `15-torture-differential-waveN-*.json` shards.
4. Gate with:
   - `make final-manifest MANIFEST=15-torture-differential-waveN-*.json`
   - `make final-bucket BUCKET=torture-differential`
   - `make final` before major commit boundaries.

## Expected Outputs
- Runtime stdout/stderr/exit expectations for executable stress tests.
- AST/diagnostic expectations for compile-surface stress tests.
