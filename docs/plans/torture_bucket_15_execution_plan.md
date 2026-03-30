# Torture Bucket 15 Execution Plan

Date: 2026-03-29

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
- Active tests: `33`
- Runtime lanes (`run=true`): `22` (all `differential=true`)
- Compile-surface stress lanes: `1` (`15__torture__pathological_decl`)
- Malformed-input negative lanes: `6`
- `.diagjson` parity lanes: `3`
- Multi-TU linkage-negative lanes: `1`
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
  - `15__probe_control_rebind_dispatch_lattice`
  - `15__probe_large_struct_array_checksum_grid`
  - `15__probe_multitu_const_table_crc`
- Validation snapshot:
  - `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py`
    -> `resolved=24`, `blocked=0`, `skipped=0`
  - `make final-bucket BUCKET=torture-differential`
    -> all `33` tests pass

## What Is Covered Now

- Long expression chains and deep nesting runtime behavior.
- Dense declaration/global-count runtime behavior.
- Many-parameter ABI call stress.
- Large struct and large array runtime paths.
- Pathological declarator parse/AST anchor.
- Malformed-input no-crash diagnostics for parser/lexer robustness.

## Key Gaps To Close

- No real external corpus compile lane yet (only embedded/fixed micro corpus lane).
- Block-scope enum constants in runtime/control expressions need dedicated semantic/codegen fix pass.

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
