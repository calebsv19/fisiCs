# Torture / Differential

## Status Snapshot (2026-03-29)
- Active tests: `33` (`tests/final/meta/15-torture-differential*.json`)
- Runtime differential lanes: `22`
- Compile-surface stress lanes: `1` (`15__torture__pathological_decl`)
- Malformed-input negative lanes: `6`
- `.diagjson` parity lanes: `3`
- Multi-TU linkage-negative lanes: `1`
- Probe snapshot:
  - `PROBE_FILTER=15__probe_*` -> `resolved=24`, `blocked=0`, `skipped=0`
  - `make final-bucket BUCKET=torture-differential` -> all `33` pass
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
- No open blockers at current baseline, but coverage breadth is still shallow.
- No real external corpus lane yet (current corpus smoke is embedded/fixed).
- Block-scope enum constants in runtime/control expressions still need dedicated fix pass.

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
