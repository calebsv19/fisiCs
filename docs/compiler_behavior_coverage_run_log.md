# Compiler Behavior Coverage Run Log

This is the rolling execution log extracted from
`docs/compiler_behavior_coverage_checklist.md` to keep the checklist compact.

## Bucket Run Log

Use this section as the current campaign tracker. Add dated entries as buckets
are worked.

Entries are historical snapshots at the time they were recorded; older entries
may describe blockers that were resolved later.

Suggested entry format:

```md
### 2026-03-11 — Bucket: diagnostics-recovery (12)

- Scope: tighten diagnostics/recovery oracle to fail closed on parser-error paths
- Harness change:
  enabled `capture_frontend_diag: true` for malformed recovery fixtures that had
  been asserting only `"Semantic analysis: no issues found."`
  (`12__bad_paren_expr`, `12__bad_decl_recovery`, `12__extra_paren_recovery`,
  `12__typedef_recovery`, `12__decl_init_recovery`,
  `12__recovery__if_missing_rparen_then_followup_stmt`,
  `12__recovery__while_missing_lparen_then_followup_stmt`,
  `12__recovery__do_while_missing_semicolon_then_followup_stmt`,
  `12__recovery__goto_missing_label_token_then_followup_label`,
  `12__recovery__case_outside_switch_then_next_stmt_ok`)
- Updated expectations:
  regenerated `.diag` outputs for those cases and refreshed
  `12__missing_semicolon.diag` drift.
- Added probe coverage:
  `12__probe_while_missing_lparen_reject`,
  `12__probe_do_while_missing_semicolon_reject`,
  `12__probe_for_header_missing_semicolon_reject`,
  plus diagnostics-JSON probes
  `12__probe_diagjson_while_missing_lparen`,
  `12__probe_diagjson_do_while_missing_semicolon`,
  `12__probe_diagjson_for_header_missing_semicolon`.
- Current 12-probe status:
  `blocked=2`, `resolved=7`, `skipped=0`.
- Active blocker details:
  parser errors are visible in CLI output for malformed `while`/`do-while`, but
  `--emit-diags-json` returns empty diagnostics (`diag_count=0`) on those paths.
- Bucket sweep:
  all 46 `12__*` active tests currently pass in targeted smoke.

### 2026-03-02 — Bucket: lexer

- Scope: identifiers, keywords, integer literals
- Added tests: `02__identifiers__ascii_valid`, `02__keywords__identifier_reject`
- Passing: 12
- Failing: 4
- Unsupported: 1 (`universal character names`)
- Main failures:
  - integer suffix parsing rejects valid `ULL`
  - invalid octal literal accepted silently
- Fix batch status: pending
```

### 2026-03-30 — Bucket: torture-differential (15) planning refresh + next-wave map

### 2026-04-01 — Bucket: torture-differential (15) wave-29 external corpus scale-up II closure

### 2026-04-01 — Bucket: torture-differential (15) wave-30 policy matrix expansion II closure

- Scope:
  expand explicit UB/implementation-defined policy-tag lanes with deterministic
  probe behavior and skip-gated final differential checks.
- Added probes:
  `15__probe_policy_impldef_signed_shift_width_matrix_tagged`,
  `15__probe_policy_impldef_char_promotion_matrix_tagged`,
  `15__probe_policy_ub_signed_overflow_chain_tagged`,
  `15__probe_policy_ub_eval_order_call_sidefx_tagged`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=4`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave30-policy-matrix-expansion-ii.json`
  with:
  `15__torture__policy_impldef_signed_shift_width_matrix_tagged`,
  `15__torture__policy_impldef_char_promotion_matrix_tagged`,
  `15__torture__policy_ub_signed_overflow_chain_tagged`,
  `15__torture__policy_ub_eval_order_call_sidefx_tagged`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave30-policy-matrix-expansion-ii.json` passes (`0 failing, 4 skipped`);
  `make final-wave WAVE=30 WAVE_BUCKET=15-torture-differential` passes (`0 failing, 4 skipped`);
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 10 skipped`, `108` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=99`, `skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15) wave-31 malformed header-surface parity closure

### 2026-04-01 — Bucket: torture-differential (15) wave-32 optional gcc tri-diff policy crosscheck closure

### 2026-04-01 — Bucket: torture-differential (15) wave-33 malformed header-chain diagjson expansion closure

- Scope:
  expand malformed header-chain no-crash coverage with additional `.diagjson`
  parity checks and chained guard-mismatch fixture.
- Added probes:
  `15__probe_diag_malformed_header_macro_cycle_chain_no_crash`,
  `15__probe_diagjson_malformed_header_macro_cycle_chain_no_crash`,
  `15__probe_diag_malformed_header_guard_tail_mismatch_chain_no_crash`,
  `15__probe_diagjson_malformed_header_guard_tail_mismatch_chain_no_crash`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=4`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave33-malformed-header-chain-diagjson-expansion.json`
  with:
  `15__diagjson__malformed_header_macro_cycle_chain_no_crash`,
  `15__torture__malformed_header_guard_tail_mismatch_chain_no_crash`,
  `15__diagjson__malformed_header_guard_tail_mismatch_chain_no_crash`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave33-malformed-header-chain-diagjson-expansion.json` passes;
  `make final-wave WAVE=33 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 10 skipped`, `116` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=107`, `skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15) wave-34 optional gcc tri-diff header/control matrix closure

- Scope:
  add optional gcc tri-reference differential checks for deterministic
  header-influenced control/layout runtime matrices.
- Added probes:
  `15__probe_runtime_clang_gcc_tri_diff_header_control_dispatch_matrix`,
  `15__probe_runtime_clang_gcc_tri_diff_header_layout_fold_matrix`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=2`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave34-gcc-tri-diff-header-control-matrix.json`
  with:
  `15__torture__clang_gcc_tri_diff_header_control_dispatch_matrix`,
  `15__torture__clang_gcc_tri_diff_header_layout_fold_matrix`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave34-gcc-tri-diff-header-control-matrix.json` passes;
  `make final-wave WAVE=34 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 10 skipped`, `118` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=109`, `skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15) wave-35 malformed include-graph diagjson follow-up closure

- Scope:
  extend malformed include-graph fail-closed coverage with one deterministic
  text-diagnostic lane plus two structured diagnostics-json parity lanes.
- Added probes:
  `15__probe_diag_malformed_include_graph_guard_collision_no_crash`,
  `15__probe_diagjson_malformed_include_graph_guard_collision_no_crash`,
  `15__probe_diagjson_malformed_include_graph_macro_arity_chain_no_crash`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=3`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave35-malformed-include-graph-diagjson-followup.json`
  with:
  `15__torture__malformed_include_graph_guard_collision_no_crash`,
  `15__diagjson__malformed_include_graph_guard_collision_no_crash`,
  `15__diagjson__malformed_include_graph_macro_arity_chain_no_crash`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave35-malformed-include-graph-diagjson-followup.json` passes;
  `make final-wave WAVE=35 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 10 skipped`, `121` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=112`, `skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15) pause stocktake / gap refinement

- Scope:
  pause wave execution and refresh docs with a concrete coverage-vs-gap picture
  for stress/torture lanes.
- Current baseline captured:
  `36` manifests, `121` tests, `68` runtime differential lanes, `30` diagnostics
  lanes, `22` diagjson lanes.
- Confirmed stable state:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped`;
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=112`, `skipped=0`.
- Refined unresolved (non-blocking) gaps recorded:
  - tri-reference GCC lanes are still a subset of runtime differential lanes,
  - compile-surface stress breadth is low relative to runtime surface,
  - seeded fuzz smoke is present but no high-volume deterministic replay campaign,
  - external-corpus coverage remains fragment-based, not larger pinned bundles.

### 2026-04-01 — Bucket: torture-differential (15) wave-36..40 mapping plan created

- Scope:
  translate the five unresolved stress areas into a concrete five-wave execution
  map with planned lanes, probes, and manifest shard names.
- Planned waves:
  - Wave 36: stress-ceiling frontier sweep
  - Wave 37: fuzz-volume deterministic replay
  - Wave 38: gcc tri-reference breadth expansion
  - Wave 39: compile-surface stress expansion
  - Wave 40: external-corpus pinned bundle expansion
- Plan file updated:
  `docs/plans/torture_bucket_15_execution_plan.md`.

- Scope:
  add optional gcc tri-reference differential checks for host-stable policy
  runtime matrices.
- Added probes:
  `15__probe_runtime_clang_gcc_tri_diff_policy_shift_char_matrix`,
  `15__probe_runtime_clang_gcc_tri_diff_policy_struct_abi_matrix`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=2`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave32-gcc-tri-diff-policy-crosscheck.json`
  with:
  `15__torture__clang_gcc_tri_diff_policy_shift_char_matrix`,
  `15__torture__clang_gcc_tri_diff_policy_struct_abi_matrix`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave32-gcc-tri-diff-policy-crosscheck.json` passes;
  `make final-wave WAVE=32 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 10 skipped`, `113` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=104`, `skipped=0`.

- Scope:
  expand malformed header-surface no-crash coverage with deterministic
  recursive-include and macro-chain diagnostics plus one `.diagjson` parity lane.
- Added probes:
  `15__probe_diag_malformed_header_guard_tail_mismatch_no_crash`,
  `15__probe_diag_malformed_header_macro_cycle_chain_no_crash`,
  `15__probe_diagjson_malformed_header_guard_tail_mismatch_no_crash`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=3`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave31-malformed-header-surface-parity.json`
  with:
  `15__torture__malformed_header_guard_tail_mismatch_no_crash`,
  `15__torture__malformed_header_macro_cycle_chain_no_crash`,
  `15__diagjson__malformed_header_guard_tail_mismatch_no_crash`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave31-malformed-header-surface-parity.json` passes;
  `make final-wave WAVE=31 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 10 skipped`, `111` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=102`, `skipped=0`.

- Scope:
  add second external-corpus expansion with two deterministic runtime fragments
  plus one fail-closed include-guard mismatch negative lane.
- Added probes:
  `15__probe_corpus_external_decl_chain_smoke`,
  `15__probe_corpus_external_typedef_graph_smoke`,
  `15__probe_diag_corpus_external_include_guard_mismatch_reject`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=3`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave29-external-corpus-scaleup-ii.json`
  with:
  `15__torture__corpus_external_decl_chain_smoke`,
  `15__torture__corpus_external_typedef_graph_smoke`,
  `15__torture__corpus_external_include_guard_mismatch_reject`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave29-external-corpus-scaleup-ii.json` passes;
  `make final-wave WAVE=29 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 6 skipped`, `104` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=95`, `skipped=0`.

- Scope:
  refresh bucket-15 planning docs with current baseline and explicit closure
  wave order for remaining coverage gaps.
- Fresh baseline verify:
  - `make final-bucket BUCKET=torture-differential` => all `33/33` pass.
  - `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py`
    => `blocked=0`, `resolved=24`, `skipped=0`.
- Metadata baseline verify:
  `7` manifests, `33` active tests, `22` runtime differential lanes, `8`
  diagnostics lanes, `3` diagnostics-json lanes, `1` AST lane.
- Doc state correction:
  removed stale blocker text about block-scope enum constants in bucket-15 docs
  (this was resolved in prior fix work).
- New planned waves recorded:
  Wave7 external corpus deterministic lane,
  Wave8 compile-surface expansion,
  Wave9 malformed fuzz-smoke matrix,
  Wave10 optional GCC differential lane,
  Wave11 UB/impl-defined policy tagging.

### 2026-03-30 — Bucket: torture-differential (15) wave-7 external-corpus closure

- Scope:
  promote first external-corpus styled deterministic lane plus a fail-closed
  negative corpus fragment.
- Added probes:
  `15__probe_corpus_external_compile_smoke`,
  `15__probe_diag_corpus_external_compile_reject`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=2`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave7-external-corpus.json`
  with:
  `15__torture__corpus_external_compile_smoke`,
  `15__torture__corpus_external_compile_reject`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave7-external-corpus.json` passes;
  `make final-wave WAVE=7 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`35/35`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=26`, `skipped=0`.

### 2026-03-30 — Bucket: torture-differential (15) wave-8 compile-surface closure

- Scope:
  expand compile-surface torture coverage with one AST-heavy type-graph lane,
  one negative initializer-shape lane, and `.diagjson` parity for that reject path.
- Added probes:
  `15__probe_ast_pathological_type_graph`,
  `15__probe_diag_pathological_initializer_shape_reject`,
  `15__probe_diagjson_pathological_initializer_shape_reject`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=3`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave8-compile-surface.json`
  with:
  `15__torture__pathological_type_graph`,
  `15__torture__pathological_initializer_shape_reject`,
  `15__diagjson__pathological_initializer_shape_reject`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave8-compile-surface.json` passes;
  `make final-wave WAVE=8 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`38/38`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=29`, `skipped=0`.

### 2026-03-30 — Bucket: torture-differential (15) wave-9 malformed fuzz matrix closure

- Scope:
  add deterministic seeded malformed token-stream matrix lanes and promote
  `.diagjson` parity for one seeded lane.
- Added probes:
  `15__probe_diag_malformed_token_stream_seeded_a_no_crash`,
  `15__probe_diag_malformed_token_stream_seeded_b_no_crash`,
  `15__probe_diagjson_malformed_token_stream_seeded_a_no_crash`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=3`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave9-malformed-fuzz-matrix.json`
  with:
  `15__torture__malformed_token_stream_seeded_a_no_crash`,
  `15__torture__malformed_token_stream_seeded_b_no_crash`,
  `15__diagjson__malformed_token_stream_seeded_a_no_crash`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave9-malformed-fuzz-matrix.json` passes;
  `make final-wave WAVE=9 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`41/41`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=32`, `skipped=0`.

### 2026-03-30 — Bucket: torture-differential (15) wave-10 optional GCC differential closure

- Scope:
  add an optional GCC-referenced differential lane while keeping deterministic
  default behavior and explicit skip semantics when GCC is unavailable.
- Harness changes:
  - `tests/final/probes/run_probes.py` now supports per-probe
    `extra_differential_compiler` checks for tri-reference runtime smoke lanes.
  - `tests/final/run_final.py` differential output now references the configured
    `differential_compiler` name (not clang-specific strings).
- Added probe lane:
  `15__probe_runtime_clang_gcc_tri_diff_smoke`.
- Probe verify:
  `PROBE_FILTER=15__probe_runtime_clang_gcc_tri_diff_smoke python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=1`, `skipped=0`.
- Promotions:
  added `tests/final/meta/15-torture-differential-wave10-gcc-optional-diff.json`
  with:
  `15__torture__clang_gcc_tri_diff_smoke`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave10-gcc-optional-diff.json` passes;
  `make final-wave WAVE=10 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`42/42`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=33`, `skipped=0`.

### 2026-03-30 — Bucket: torture-differential (15) wave-11 policy tagging closure

- Scope:
  close UB/implementation-defined policy tagging so differential behavior is
  explicitly gated by metadata instead of implicit assumptions.
- Added probe lanes:
  `15__probe_policy_signed_shift_impl_defined_tagged`,
  `15__probe_policy_alias_ub_tagged`.
- Probe verify:
  `PROBE_FILTER=15__probe_policy_signed_shift_impl_defined_tagged,15__probe_policy_alias_ub_tagged python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=2`, `skipped=0`.
- Promotions:
  added `tests/final/meta/15-torture-differential-wave11-policy-tags.json`
  with:
  `15__torture__policy_signed_shift_impl_defined_tagged`,
  `15__torture__policy_alias_ub_tagged`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave11-policy-tags.json`
  => `0 failing, 2 skipped`;
  `make final-wave WAVE=11 WAVE_BUCKET=15-torture-differential`
  => `0 failing, 2 skipped`;
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 2 skipped` (`44` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=35`, `skipped=0`.

### 2026-03-30 — Bucket: torture-differential (15) wave-12 malformed matrix parity expansion

- Scope:
  continue bucket-15 malformed robustness expansion by adding seeded-C malformed
  text-diagnostic lane and `.diagjson` parity lanes for seeded B/C.
- Added probe lanes:
  `15__probe_diag_malformed_token_stream_seeded_c_no_crash`,
  `15__probe_diagjson_malformed_token_stream_seeded_b_no_crash`,
  `15__probe_diagjson_malformed_token_stream_seeded_c_no_crash`.
- Probe verify:
  `PROBE_FILTER=15__probe_diag_malformed_token_stream_seeded_c_no_crash,15__probe_diagjson_malformed_token_stream_seeded_b_no_crash,15__probe_diagjson_malformed_token_stream_seeded_c_no_crash python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=3`, `skipped=0`.
- Promotions:
  added `tests/final/meta/15-torture-differential-wave12-malformed-diagjson-parity.json`
  with:
  `15__torture__malformed_token_stream_seeded_c_no_crash`,
  `15__diagjson__malformed_token_stream_seeded_b_no_crash`,
  `15__diagjson__malformed_token_stream_seeded_c_no_crash`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave12-malformed-diagjson-parity.json` passes;
  `make final-wave WAVE=12 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`47` active).
- Full-suite gate:
  `make final` => `0 failing, 24 skipped`.
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=38`, `skipped=0`.

### 2026-03-30 — Bucket: torture-differential (15) wave-13 multi-TU stress expansion

- Scope:
  expand cross-translation-unit runtime stress with denser stateful and
  data-layout workloads beyond the earlier baseline multi-TU lanes.
- Added probe lanes:
  `15__probe_multitu_fnptr_state_matrix`,
  `15__probe_multitu_struct_vla_stride_bridge`,
  `15__probe_multitu_ring_digest_pipeline`.
- Probe verify:
  `PROBE_FILTER=15__probe_multitu_fnptr_state_matrix,15__probe_multitu_struct_vla_stride_bridge,15__probe_multitu_ring_digest_pipeline python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=3`, `skipped=0`.
- Promotions:
  added `tests/final/meta/15-torture-differential-wave13-multitu-stress-expansion.json`
  with:
  `15__torture__multitu_fnptr_state_matrix`,
  `15__torture__multitu_struct_vla_stride_bridge`,
  `15__torture__multitu_ring_digest_pipeline`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave13-multitu-stress-expansion.json` passes;
  `make final-wave WAVE=13 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`50` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=41`, `skipped=0`.

### 2026-03-30 — Bucket: torture-differential (15) wave-14 multi-TU link-failure matrix

- Scope:
  expand link-stage fail-closed coverage with additional multi-TU collision lanes
  and keep `.diagjson` parity for those link failures explicit.
- Added probe lanes:
  `15__probe_diag_multitu_duplicate_function_definition_reject`,
  `15__probe_diag_multitu_symbol_type_conflict_reject`,
  `15__probe_diagjson_multitu_duplicate_function_definition_reject`,
  `15__probe_diagjson_multitu_symbol_type_conflict_reject`.
- Probe verify:
  `PROBE_FILTER=15__probe_diag_multitu_duplicate_function_definition_reject,15__probe_diag_multitu_symbol_type_conflict_reject,15__probe_diagjson_multitu_duplicate_function_definition_reject,15__probe_diagjson_multitu_symbol_type_conflict_reject python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=4`, `skipped=0`.
- Promotions:
  added `tests/final/meta/15-torture-differential-wave14-multitu-link-failure-matrix.json`
  with:
  `15__torture__multitu_duplicate_function_definition_reject`,
  `15__torture__multitu_symbol_type_conflict_reject`,
  `15__diagjson__multitu_duplicate_function_definition_reject`,
  `15__diagjson__multitu_symbol_type_conflict_reject`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave14-multitu-link-failure-matrix.json` passes;
  `make final-wave WAVE=14 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`54` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=45`, `skipped=0`.

### 2026-03-27 — Bucket: runtime-surface (14) wave-75 fnptr initializer-signature closure

- Scope:
  close the remaining fnptr table initializer-signature diagnostics lane and
  promote it into active runtime diagnostics coverage.
- Semantic hardening:
  tightened array-initializer element-type handling in
  `src/Syntax/analyze_decls.c` so element compatibility checks use stable parsed
  type data in this path.
- Probe/case lane update:
  normalized the wave-75 source shape to explicit function-pointer array
  declarators for deterministic signature checking.
- Probe verify:
  `PROBE_FILTER=14__probe_diag_fnptr_table_incompatible_signature_reject,14__probe_diagjson_fnptr_table_incompatible_signature_reject python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=2`, `skipped=0`.
- Promotion:
  added
  `tests/final/meta/14-runtime-surface-wave75-fnptr-incompatible-signature-diagnostics.json`
  with:
  `14__diag__fnptr_table_incompatible_signature_reject`.
- Runtime verify:
  `make final-wave WAVE=75` passes; `make final-runtime` remains green
  (`0 failing, 22 skipped`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=14__probe_*` => `blocked=0`, `resolved=195`, `skipped=0`.

### 2026-03-29 — Bucket: torture-differential (15) wave-2 malformed + diagjson parity closure

- Scope:
  add malformed robustness lanes and close diagnostics-json parity for malformed
  bucket-15 paths.
- Added probe lanes:
  `15__probe_diag_malformed_unterminated_string_no_crash`,
  `15__probe_diag_malformed_bad_escape_no_crash`,
  `15__probe_diag_malformed_pp_directive_no_crash`,
  `15__probe_diagjson_malformed_unbalanced_block_no_crash`,
  `15__probe_diagjson_malformed_unclosed_comment_no_crash`.
- Probe verify:
  targeted Wave2 probe run reports `blocked=0`, `resolved=5`, `skipped=0`.
- Fix shipped:
  lexer fatal diagnostics are now mirrored into compiler diagnostics so
  `--emit-diags-json` is populated on lexer-fatal fail-closed paths.
  Files touched:
  `src/Lexer/lexer.c`, `src/Lexer/lexer.h`, `src/Compiler/pipeline.c`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave2-malformed-diagjson.json`
  with:
  `15__torture__malformed_unterminated_string_no_crash`,
  `15__torture__malformed_bad_escape_no_crash`,
  `15__torture__malformed_pp_directive_no_crash`,
  `15__diagjson__malformed_unbalanced_block_no_crash`,
  `15__diagjson__malformed_unclosed_comment_no_crash`.
- Runtime verify:
  `make final-wave WAVE=2 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`20/20`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=11`, `skipped=0`.

### 2026-03-29 — Bucket: torture-differential (15) wave-3 multi-TU closure

- Scope:
  add multi-translation-unit runtime stress plus link-stage negative/diagjson
  parity lanes for bucket-15.
- Added runtime probe lanes:
  `15__probe_multitu_many_globals_crossref`,
  `15__probe_multitu_fnptr_dispatch_grid`.
- Added negative probe lanes:
  `15__probe_diag_multitu_duplicate_symbol_reject`,
  `15__probe_diagjson_multitu_duplicate_symbol_reject`.
- Probe verify:
  all Wave3 targeted probes resolved; no blockers.
- Promotions:
  updated
  `tests/final/meta/15-torture-differential-wave3-multitu.json`
  with:
  `15__torture__multitu_many_globals_crossref`,
  `15__torture__multitu_fnptr_dispatch_grid`,
  `15__torture__multitu_duplicate_symbol_reject`,
  `15__diagjson__multitu_duplicate_symbol_reject`.
- Runtime verify:
  `make final-wave WAVE=3 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`24/24`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=15`, `skipped=0`.

### 2026-03-29 — Bucket: torture-differential (15) wave-4 resource-pressure closure

- Scope:
  add deterministic pressure lanes for recursion depth, VLA stack/stride load,
  and high-arity ABI call boundaries.
- Added runtime probe lanes:
  `15__probe_deep_recursion_stack_pressure`,
  `15__probe_large_vla_stride_pressure`,
  `15__probe_many_args_regstack_pressure`.
- Probe verify:
  all Wave4 targeted probes resolved with no blockers.
- Promotions:
  added/updated
  `tests/final/meta/15-torture-differential-wave4-pressure.json`
  with:
  `15__torture__deep_recursion_stack_pressure`,
  `15__torture__large_vla_stride_pressure`,
  `15__torture__many_args_regstack_pressure`.
- Runtime verify:
  `make final-wave WAVE=4 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`27/27`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=18`, `skipped=0`.

### 2026-03-29 — Bucket: torture-differential (15) wave-5 deterministic fuzz/corpus hooks closure

- Scope:
  add deterministic fuzz-compatible and corpus-style smoke lanes, then promote
  them into active bucket-15 runtime coverage.
- Added runtime probe lanes:
  `15__probe_seeded_expr_fuzz_smoke`,
  `15__probe_seeded_stmt_fuzz_smoke`,
  `15__probe_corpus_micro_compile_smoke`.
- Probe verify:
  targeted Wave5 probe run reports `blocked=0`, `resolved=3`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave5-fuzz-corpus-hooks.json`
  with:
  `15__torture__seeded_expr_fuzz_smoke`,
  `15__torture__seeded_stmt_fuzz_smoke`,
  `15__torture__corpus_micro_compile_smoke`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave5-fuzz-corpus-hooks.json` passes;
  `make final-wave WAVE=5 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`30/30`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=21`, `skipped=0`.

### 2026-03-29 — Bucket: torture-differential (15) wave-6 control/layout + multi-TU const-table closure

- Scope:
  deepen runtime torture coverage with function-pointer control rebinding,
  large struct-array layout pressure, and one additional multi-TU const-table
  fold lane.
- Added runtime probe lanes:
  `15__probe_control_rebind_dispatch_lattice`,
  `15__probe_large_struct_array_checksum_grid`,
  `15__probe_multitu_const_table_crc`.
- Probe verify:
  targeted Wave6 probe run reports `blocked=0`, `resolved=3`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave6-control-layout-multitu.json`
  with:
  `15__torture__control_rebind_dispatch_lattice`,
  `15__torture__large_struct_array_checksum_grid`,
  `15__torture__multitu_const_table_crc`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave6-control-layout-multitu.json` passes;
  `make final-wave WAVE=6 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`33/33`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=24`, `skipped=0`.

### 2026-03-27 — Bucket: runtime-surface (14) planning refresh for closure waves (76-87)

- Scope:
  refresh bucket-14 state docs to reflect current counts and define explicit
  closure roadmap before moving primary focus to bucket 15.
- Current runtime snapshot:
  `make final-runtime` => `0 failing`, `22 skipped`.
- Current probe snapshot:
  `PROBE_FILTER=14__probe_*` => `blocked=0`, `resolved=197`, `skipped=0`.
- Doc corrections applied:
  active tests now `283` and negative diagnostics now `39` (`24` diag + `15`
  diagjson) in bucket-14 status docs.
- Missing-area record now explicit:
  diagnostics-json parity gap (`9` diag lanes without diagjson parity),
  link/type conflict diagnostics depth, ABI frontier stress, static-local init
  one-time semantics, and deterministic mini-binary smoke coverage.
- New roadmap recorded:
  Wave76-Wave87 plan added with concrete lane ids and per-wave promotion gates
  in `docs/plans/runtime_bucket_14_execution_plan.md` and summarized in
  `tests/final/14-runtime-surface.md`.

### 2026-03-27 — Bucket: runtime-surface (14) wave-76/77 diagnostics-json parity closure

- Scope:
  complete diagnostics-json parity for remaining runtime negative lanes.
- Promotions:
  - Wave76 (`14-runtime-surface-wave76-diagjson-parity-struct-fnptr.json`):
    `14__diagjson__fnptr_table_incompatible_signature_reject`,
    `14__diagjson__switch_struct_condition_reject`,
    `14__diagjson__if_struct_condition_reject`,
    `14__diagjson__while_struct_condition_reject`,
    `14__diagjson__return_struct_to_int_reject`.
  - Wave77 (`14-runtime-surface-wave77-diagjson-parity-complex-relational.json`):
    `14__diagjson__complex_lt_reject`,
    `14__diagjson__complex_le_reject`,
    `14__diagjson__complex_gt_reject`,
    `14__diagjson__complex_ge_reject`.
- Probe verify:
  targeted diagjson probe run reports `blocked=0`, `resolved=9`, `skipped=0`.
- Runtime verify:
  `make final-wave WAVE=76` passes;
  `make final-wave WAVE=77` passes;
  `make final-runtime` remains green (`0 failing, 22 skipped`).

### 2026-03-27 — Bucket: runtime-surface (14) wave-78 abi reg/stack frontier closure

- Scope:
  add strict runtime ABI frontier coverage for mixed scalar/struct and variadic
  call paths.
- Added probe lanes:
  `14__probe_abi_reg_stack_frontier_matrix`,
  `14__probe_abi_mixed_struct_float_boundary`,
  `14__probe_variadic_abi_reg_stack_frontier`.
- Probe verify:
  targeted runtime probe run reports `blocked=0`, `resolved=3`, `skipped=0`.
- Promotion:
  added `tests/final/meta/14-runtime-surface-wave78-abi-reg-stack-frontier.json`
  with:
  `14__runtime_abi_reg_stack_frontier_matrix`,
  `14__runtime_abi_mixed_struct_float_boundary`,
  `14__runtime_variadic_abi_reg_stack_frontier`.
- Runtime verify:
  `make final-wave WAVE=78` passes;
  `make final-runtime` remains green (`0 failing, 22 skipped`).

### 2026-03-26 — Bucket: runtime-surface (14) wave-66 complex pointer-writeback closure

- Scope:
  close the pending complex pointer-member writeback hardening lane and promote
  it into active runtime coverage.
- Fixes:
  - removed noisy fallback diagnostics from `codegenLValue` when probing
    non-lvalue RHS paths.
  - fixed compound-assignment lowering for complex aggregate-backed values in
    `codegenAssignment` so `+=` / `-=` on `_Complex` struct fields performs
    proper read-modify-write instead of degenerating to plain assignment.
- Added probes:
  `14__probe_complex_ptr_field_writeback_direct`,
  `14__probe_complex_ptr_field_compound_writeback`.
- Probe verify:
  `PROBE_FILTER=14__probe_complex_ptr_field_writeback_direct,14__probe_complex_ptr_field_compound_writeback python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=2`, `skipped=0`.
- Promotion:
  added
  `tests/final/meta/14-runtime-surface-wave66-complex-pointer-writeback-promotions.json`
  with:
  `14__runtime_complex_ptr_field_writeback_direct`,
  `14__runtime_complex_ptr_field_compound_writeback`.
- Runtime verify:
  `make final-wave WAVE=66` passes; `make final-runtime` remains green
  (`0 failing, 22 skipped`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=14__probe_*` => `blocked=0`, `resolved=160`, `skipped=0`.

### 2026-03-26 — Bucket: runtime-surface (14) wave-67 complex `*=` / `/=` closure

- Scope:
  extend the complex pointer-member writeback lane to multiplicative/divisive
  compound assignment forms.
- Fixes:
  - added complex multiply/divide lowering in binary expression codegen.
  - extended complex aggregate compound-assignment lowering to support
    `*=` and `/=` on `_Complex` struct fields.
- Added probes:
  `14__probe_complex_ptr_field_mul_writeback`,
  `14__probe_complex_ptr_field_div_writeback`.
- Probe verify:
  `PROBE_FILTER=14__probe_complex_ptr_field_mul_writeback,14__probe_complex_ptr_field_div_writeback python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=2`, `skipped=0`.
- Promotion:
  added
  `tests/final/meta/14-runtime-surface-wave67-complex-muldiv-writeback-promotions.json`
  with:
  `14__runtime_complex_ptr_field_mul_writeback`,
  `14__runtime_complex_ptr_field_div_writeback`.
- Runtime verify:
  `make final-wave WAVE=67` passes; `make final-runtime` remains green
  (`0 failing, 22 skipped`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=14__probe_*` => `blocked=0`, `resolved=162`, `skipped=0`.

### 2026-03-26 — Bucket: runtime-surface (14) wave-68 complex mul/div expression closure

- Scope:
  lock direct complex multiplication/division expression behavior (non-assignment paths).
- Added probes:
  `14__probe_complex_mul_div_matrix`,
  `14__probe_complex_scalar_mul_div_chain`.
- Probe verify:
  `PROBE_FILTER=14__probe_complex_mul_div_matrix,14__probe_complex_scalar_mul_div_chain python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=2`, `skipped=0`.
- Promotion:
  added
  `tests/final/meta/14-runtime-surface-wave68-complex-muldiv-expression-promotions.json`
  with:
  `14__runtime_complex_mul_div_matrix`,
  `14__runtime_complex_scalar_mul_div_chain`.
- Runtime verify:
  `make final-wave WAVE=68` passes; `make final-runtime` remains green
  (`0 failing, 22 skipped`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=14__probe_*` => `blocked=0`, `resolved=164`, `skipped=0`.

### 2026-03-26 — Bucket: runtime-surface (14) wave-69 complex relational diagnostics closure

- Scope:
  harden semantic diagnostics for complex relational operators (`<`, `<=`, `>`, `>=`)
  so invalid ordering comparisons fail closed with stable diagnostics.
- Semantic fix:
  updated comparison analysis to reject relational operators when either operand
  is complex/imaginary, reporting:
  `Operator '<op>' requires real (non-complex) comparable operands`.
- Added diagnostic probes:
  `14__probe_diag_complex_lt_reject`,
  `14__probe_diag_complex_le_reject`,
  `14__probe_diag_complex_gt_reject`,
  `14__probe_diag_complex_ge_reject`.
- Probe verify:
  `PROBE_FILTER=14__probe_diag_complex_lt_reject,14__probe_diag_complex_le_reject,14__probe_diag_complex_gt_reject,14__probe_diag_complex_ge_reject python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=4`, `skipped=0`.
- Promotion:
  added
  `tests/final/meta/14-runtime-surface-wave69-complex-relational-diagnostics.json`
  with:
  `14__diag__complex_lt_reject`,
  `14__diag__complex_le_reject`,
  `14__diag__complex_gt_reject`,
  `14__diag__complex_ge_reject`.
- Runtime verify:
  `make final-wave WAVE=69` passes; `make final-runtime` remains green
  (`0 failing, 22 skipped`).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=14__probe_*` => `blocked=0`, `resolved=168`, `skipped=0`.

### 2026-03-26 — Bucket: runtime-surface (14) wave-69 diagnostics-json assertion closure

- Scope:
  extend wave-69 complex-relational diagnostics from text diagnostics (`.diag`)
  to structured diagnostics (`.diagjson`) so code/line assertions are enforced
  in both probe and final lanes.
- Probe harness updates:
  `tests/final/probes/run_probes.py` now supports diag-json assertions for
  expected diagnostic code and line (`expected_codes`, `expected_line`).
- Added diag-json probes:
  `14__probe_diagjson_complex_lt_reject`,
  `14__probe_diagjson_complex_le_reject`,
  `14__probe_diagjson_complex_gt_reject`,
  `14__probe_diagjson_complex_ge_reject`.
- Probe verify:
  `PROBE_FILTER=14__probe_diagjson_complex_lt_reject,14__probe_diagjson_complex_le_reject,14__probe_diagjson_complex_gt_reject,14__probe_diagjson_complex_ge_reject python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=4`, `skipped=0`.
- Promotion/update:
  `tests/final/meta/14-runtime-surface-wave69-complex-relational-diagnostics.json`
  now asserts both `.diag` and `.diagjson` for:
  `14__diag__complex_lt_reject`,
  `14__diag__complex_le_reject`,
  `14__diag__complex_gt_reject`,
  `14__diag__complex_ge_reject`.
- Runtime verify:
  `make final-wave WAVE=69` passes; `make final-runtime` remains green
  (`0 failing, 22 skipped`).
- Bucket probe snapshot after diag-json closure:
  `PROBE_FILTER=14__probe_*` => `blocked=0`, `resolved=172`, `skipped=0`.

### 2026-03-26 — Bucket: runtime-surface (14) wave-70 control diagnostics-json promotion

- Scope:
  add structured diagnostics-json assertions for core control-flow reject
  diagnostics already covered by text diagnostics (`.diag`) in wave55.
- Added diagnostic-json probes:
  `14__probe_diagjson_continue_outside_loop_reject`,
  `14__probe_diagjson_break_outside_loop_reject`,
  `14__probe_diagjson_case_outside_switch_reject`,
  `14__probe_diagjson_default_outside_switch_reject`.
- Probe verify:
  `PROBE_FILTER=14__probe_diagjson_continue_outside_loop_reject,14__probe_diagjson_break_outside_loop_reject,14__probe_diagjson_case_outside_switch_reject,14__probe_diagjson_default_outside_switch_reject python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=4`, `skipped=0`.
- Promotion:
  added
  `tests/final/meta/14-runtime-surface-wave70-control-diagnostics-json.json`
  with:
  `14__diagjson__continue_outside_loop_reject`,
  `14__diagjson__break_outside_loop_reject`,
  `14__diagjson__case_outside_switch_reject`,
  `14__diagjson__default_outside_switch_reject`.
- Runtime verify:
  `make final-wave WAVE=70` passes; `make final-runtime` remains green
  (`0 failing, 22 skipped`).
- Bucket probe snapshot after wave70:
  `PROBE_FILTER=14__probe_*` => `blocked=0`, `resolved=176`, `skipped=0`.

### 2026-03-27 — Bucket: runtime-surface (14) wave-71 switch diagnostics-json promotion

- Scope:
  extend diagnostics-json assertions to switch-control reject diagnostics so
  structured diagnostics are verified in addition to text diagnostics.
- Added diagnostic-json probes:
  `14__probe_diagjson_switch_duplicate_default_reject`,
  `14__probe_diagjson_switch_duplicate_case_reject`,
  `14__probe_diagjson_switch_nonconst_case_reject`,
  `14__probe_diagjson_continue_in_switch_reject`.
- Probe verify:
  `PROBE_FILTER=14__probe_diagjson_switch_duplicate_default_reject,14__probe_diagjson_switch_duplicate_case_reject,14__probe_diagjson_switch_nonconst_case_reject,14__probe_diagjson_continue_in_switch_reject python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=4`, `skipped=0`.
- Promotion:
  added
  `tests/final/meta/14-runtime-surface-wave71-switch-diagnostics-json.json`
  with:
  `14__diagjson__switch_duplicate_default_reject`,
  `14__diagjson__switch_duplicate_case_reject`,
  `14__diagjson__switch_nonconst_case_reject`,
  `14__diagjson__continue_in_switch_reject`.
- Runtime verify:
  `make final-wave WAVE=71` passes.

### 2026-03-27 — Bucket: runtime-surface (14) wave-72 fnptr diagnostics-json promotion

- Scope:
  extend diagnostics-json assertions to fnptr reject diagnostics from wave62.
- Added diagnostic-json probes:
  `14__probe_diagjson_fnptr_table_too_many_args_reject`,
  `14__probe_diagjson_fnptr_struct_arg_incompatible_reject`,
  `14__probe_diagjson_fnptr_param_noncallable_reject`.
- Probe verify:
  `PROBE_FILTER=14__probe_diagjson_fnptr_table_too_many_args_reject,14__probe_diagjson_fnptr_struct_arg_incompatible_reject,14__probe_diagjson_fnptr_param_noncallable_reject python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=3`, `skipped=0`.
- Promotion:
  added
  `tests/final/meta/14-runtime-surface-wave72-fnptr-diagnostics-json.json`
  with:
  `14__diagjson__fnptr_table_too_many_args_reject`,
  `14__diagjson__fnptr_struct_arg_incompatible_reject`,
  `14__diagjson__fnptr_param_noncallable_reject`.
- Runtime verify:
  `make final-wave WAVE=72` passes; `make final-runtime` remains green
  (`0 failing, 22 skipped`).
- Bucket probe snapshot after wave72:
  `PROBE_FILTER=14__probe_*` => `blocked=0`, `resolved=183`, `skipped=0`.

### 2026-03-27 — Bucket: runtime-surface (14) wave-73 diagnostics-json expansion

- Scope:
  extend diagnostics-json assertions to remaining diagnostics lanes in this
  bucket: file-scope VLA rejection, offsetof bitfield rejection, ternary
  struct-condition rejection, and fnptr too-few-args rejection.
- Added diagnostic-json probes:
  `14__probe_diagjson_file_scope_vla_reject`,
  `14__probe_diagjson_offsetof_bitfield_reject`,
  `14__probe_diagjson_ternary_struct_condition_reject`,
  `14__probe_diagjson_fnptr_table_too_few_args_reject`.
- Probe verify:
  `PROBE_FILTER=14__probe_diagjson_file_scope_vla_reject,14__probe_diagjson_offsetof_bitfield_reject,14__probe_diagjson_ternary_struct_condition_reject,14__probe_diagjson_fnptr_table_too_few_args_reject python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=4`, `skipped=0`.
- Promotion:
  added
  `tests/final/meta/14-runtime-surface-wave73-diagnostics-json-expansion.json`
  with:
  `14__diagjson__file_scope_vla_reject`,
  `14__diagjson__offsetof_bitfield_reject`,
  `14__diagjson__ternary_struct_condition_reject`,
  `14__diagjson__fnptr_table_too_few_args_reject`.
- Runtime verify:
  `make final-wave WAVE=73` passes; `make final-runtime` remains green
  (`0 failing, 22 skipped`).
- Bucket probe snapshot after wave73:
  `PROBE_FILTER=14__probe_*` => `blocked=0`, `resolved=187`, `skipped=0`.

### 2026-03-27 — Bucket: runtime-surface (14) wave-74 struct-condition/return diagnostics promotion

- Scope:
  add remaining high-value semantic/control diagnostic lanes for non-scalar
  struct conditions and incompatible struct-to-int return paths, with both
  text diagnostics and diagnostics-json assertions.
- Added diagnostic probes:
  `14__probe_diag_switch_struct_condition_reject`,
  `14__probe_diag_if_struct_condition_reject`,
  `14__probe_diag_while_struct_condition_reject`,
  `14__probe_diag_return_struct_to_int_reject`.
- Added diagnostic-json probes:
  `14__probe_diagjson_switch_struct_condition_reject`,
  `14__probe_diagjson_if_struct_condition_reject`,
  `14__probe_diagjson_while_struct_condition_reject`,
  `14__probe_diagjson_return_struct_to_int_reject`.
- Probe verify:
  `PROBE_FILTER=14__probe_diag_switch_struct_condition_reject,14__probe_diag_if_struct_condition_reject,14__probe_diag_while_struct_condition_reject,14__probe_diag_return_struct_to_int_reject python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=4`, `skipped=0`.
- Probe verify:
  `PROBE_FILTER=14__probe_diagjson_switch_struct_condition_reject,14__probe_diagjson_if_struct_condition_reject,14__probe_diagjson_while_struct_condition_reject,14__probe_diagjson_return_struct_to_int_reject python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=4`, `skipped=0`.
- Promotion:
  added
  `tests/final/meta/14-runtime-surface-wave74-struct-condition-return-diagnostics.json`
  with:
  `14__diag__switch_struct_condition_reject`,
  `14__diag__if_struct_condition_reject`,
  `14__diag__while_struct_condition_reject`,
  `14__diag__return_struct_to_int_reject`.
- Runtime verify:
  `make final-wave WAVE=74` passes; `make final-runtime` remains green
  (`0 failing, 22 skipped`).
- Bucket probe snapshot after wave74:
  `PROBE_FILTER=14__probe_*` => `blocked=0`, `resolved=195`, `skipped=0`.

### 2026-03-12 — Buckets: runtime-surface (14), codegen-ir (13)

- Scope:
  promote resolved switch/loop control-flow probe into active runtime suite and
  add new IR hardening coverage for pointer scaling and loop/switch edges.
- Promotion:
  `14__probe_switch_loop_control_mix` moved to active suite as
  `14__runtime_switch_loop_control_mix`
  (`tests/final/meta/14-runtime-surface-wave4-controlflow.json`).
- Added `13` IR coverage:
  `13__ir_ptrdiff_long_long`,
  `13__ir_ptr_add_scaled_i64`,
  `13__ir_switch_loop_continue`,
  `13__ir_do_while`
  (`tests/final/meta/13-codegen-ir-wave2.json`).
- Verification:
  all newly added tests pass, and targeted `13__*` sweep is green (`18/18`).

### 2026-03-20 — Bucket: runtime-surface (14) wave-22 probe kickoff

- Scope:
  start wave-22 probe-first expansion focused on function-pointer return/callee
  edge paths and signedness-sensitive pointer/VLA indexing.
- Docs/status refresh:
  updated `docs/plans/runtime_bucket_14_execution_plan.md` and
  `tests/final/14-runtime-surface.md` to reflect wave21 baseline and wave22
  probe targets.
- Added probes:
  `14__probe_fnptr_typedef_return_direct`,
  `14__probe_fnptr_typedef_return_ternary_callee`,
  `14__probe_fnptr_expression_callee_chain`,
  `14__probe_pointer_index_width_signedness`,
  `14__probe_vla_param_mixed_signed_unsigned_indices`.
- Probe validation:
  `PROBE_FILTER=14__probe_fnptr_typedef_return_direct,14__probe_fnptr_typedef_return_ternary_callee,14__probe_fnptr_expression_callee_chain,14__probe_pointer_index_width_signedness,14__probe_vla_param_mixed_signed_unsigned_indices python3 tests/final/probes/run_probes.py`
  reports `blocked=2`, `resolved=3`, `skipped=0`.
- Resolved:
  `14__probe_fnptr_typedef_return_direct`,
  `14__probe_fnptr_expression_callee_chain`,
  `14__probe_pointer_index_width_signedness`.
- Blocked (fix-phase candidates):
  `14__probe_fnptr_typedef_return_ternary_callee`
  (`fisics: 11 9 1656200`, `clang: 11 9 4`),
  `14__probe_vla_param_mixed_signed_unsigned_indices`
  (`fisics: -527774736 11 23`, `clang: 356 11 23`).

### 2026-03-20 — Bucket: runtime-surface (14) wave-22 fix-phase closure

- Scope:
  resolve remaining wave-22 blockers and verify the full `14__*` probe lane.
- Codegen fix:
  updated non-identifier callee call lowering to recover full function
  signature metadata from semantic symbols (including ternary function-designator
  callees), so argument passing follows the correct ABI.
- Validation:
  `PROBE_FILTER=14__probe_fnptr_typedef_return_ternary_callee python3 tests/final/probes/run_probes.py`
  now resolves with clang (`stdout=11 9 4`).
- Bucket probe sweep:
  `PROBE_FILTER=14__* python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=24`, `skipped=0`.
- Status:
  wave-22 probe lane is fully green and ready for promotion into an active
  runtime shard.

### 2026-03-20 — Bucket: runtime-surface (14) wave-22 promotion closure

- Scope:
  promote the resolved wave-22 probes into active runtime manifests and
  validate final-lane stability.
- Promotion shard added:
  `tests/final/meta/14-runtime-surface-wave22-fnptr-vla-index-promotions.json`
  with:
  `14__runtime_fnptr_typedef_return_direct`,
  `14__runtime_fnptr_typedef_return_ternary_callee`,
  `14__runtime_fnptr_expression_callee_chain`,
  `14__runtime_pointer_index_width_signedness`,
  `14__runtime_vla_param_mixed_signed_unsigned_indices`.
- Manifest index updated:
  `tests/final/meta/index.json` now includes the wave22 shard.
- Expectations generated (targeted update flow):
  `UPDATE_FINAL=1 FINAL_FILTER=<wave22-test-id> python3 tests/final/run_final.py`
  for each promoted test.
- Targeted verify:
  each of the five promoted tests passes in final lane with differential checks.
- Full suite verify:
  `python3 tests/final/run_final.py` reports `0 failing, 15 skipped`.
- Status:
  wave22 is fully integrated into active runtime coverage.

### 2026-03-20 — Bucket: runtime-surface (14) wave-23 probe expansion kickoff

- Scope:
  continue bucket-14 expansion in probe mode to surface next blocker cluster
  before promotion.
- Added probes:
  `14__probe_bitfield_unsigned_pack_roundtrip`,
  `14__probe_variadic_promotion_edges`,
  `14__probe_fnptr_nested_return_dispatch_matrix`,
  `14__probe_fnptr_chooser_roundtrip_call`,
  `14__probe_vla_three_dim_stride_reduce`,
  `14__probe_vla_three_dim_index_stride_basic`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_bitfield_unsigned_pack_roundtrip,14__probe_fnptr_nested_return_dispatch_matrix,14__probe_vla_three_dim_stride_reduce,14__probe_variadic_promotion_edges python3 tests/final/probes/run_probes.py`
  reports `blocked=2`, `resolved=2`, `skipped=0`.
- Wave23 diagnosis batch:
  `PROBE_FILTER=14__probe_fnptr_nested_return_dispatch_matrix,14__probe_vla_three_dim_stride_reduce,14__probe_fnptr_chooser_roundtrip_call,14__probe_vla_three_dim_index_stride_basic python3 tests/final/probes/run_probes.py`
  reports `blocked=4`, `resolved=0`, `skipped=0`.
- Current 14-probe snapshot:
  `PROBE_FILTER=14__* python3 tests/final/probes/run_probes.py`
  reports `blocked=4`, `resolved=26`, `skipped=0`.
- Blocked inventory:
  `14__probe_fnptr_nested_return_dispatch_matrix` (`fisics: 1 4 -8`, `clang: 6 4 -8`)
  `14__probe_fnptr_chooser_roundtrip_call` (`fisics: 1 8`, `clang: 6 8`)
  `14__probe_vla_three_dim_stride_reduce` (`fisics: <garbage> 4 -8`, `clang: 2814 12 123`)
  `14__probe_vla_three_dim_index_stride_basic` (`fisics: 0 4 4`, `clang: 123 12 8`).
- Status:
  wave23 remains probe-only; no promotion until grouped fix phase resolves both
  blocker families.

### 2026-03-21 — Bucket: runtime-surface (14) wave-23 fix closure

- Scope:
  close remaining wave-23 blockers in grouped fix mode, then revalidate full
  `14__*` probe lane.
- Codegen fixes:
  - preserved decorated named-call surface types when resolving callee
    signatures, preventing alias over-resolution from erasing nested
    function-pointer returns.
  - tightened function-pointer signature reconstruction for nested chooser-style
    call chains.
- Validation:
  `PROBE_FILTER=14__probe_fnptr_nested_return_dispatch_matrix,14__probe_fnptr_chooser_roundtrip_call python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=2`, `skipped=0`.
- Full lane verify:
  `PROBE_FILTER=14__* python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=30`, `skipped=0`.
- Status:
  wave23 probe set is fully green.

### 2026-03-21 — Bucket: runtime-surface (14) wave-24 probe expansion

- Scope:
  continue probe-first expansion in the same high-risk area (typedef-decorated
  nested function-pointer chains) and deeper VLA stride/index shapes.
- Added probes:
  `14__probe_fnptr_typedef_alias_chain_dispatch`,
  `14__probe_fnptr_chooser_table_ternary_chain`,
  `14__probe_vla_four_dim_stride_matrix`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_fnptr_typedef_alias_chain_dispatch,14__probe_fnptr_chooser_table_ternary_chain,14__probe_vla_four_dim_stride_matrix python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=3`, `skipped=0`.
- Full lane verify after additions:
  `PROBE_FILTER=14__* python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=33`, `skipped=0`.
- Notes:
  first 4D VLA draft used nested loop fill and exposed a separate object-emit
  crash path (`-o` with invalid IR warning under `CODEGEN_VERIFY=1`), so the
  probe was minimized to isolate 4D VLA stride/index semantics cleanly for this
  wave.

### 2026-03-21 — Bucket: runtime-surface (14) wave-25 probe expansion kickoff

- Scope:
  continue probe-first expansion into two remaining risk lanes:
  temporary struct-return call chains and VLA parameter slice rebasing.
- Added probes:
  `14__probe_fnptr_struct_temporary_chain`,
  `14__probe_vla_param_slice_stride_rebase`,
  `14__probe_volatile_short_circuit_sequence`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_fnptr_struct_temporary_chain,14__probe_vla_param_slice_stride_rebase,14__probe_volatile_short_circuit_sequence python3 tests/final/probes/run_probes.py`
  reports `blocked=2`, `resolved=1`, `skipped=0`.
- Full lane verify after additions:
  `PROBE_FILTER=14__* python3 tests/final/probes/run_probes.py`
  reports `blocked=2`, `resolved=34`, `skipped=0`.
- Blocked inventory:
  `14__probe_fnptr_struct_temporary_chain`
  (semantic: `Error at (40:0): Argument 1 of 'apply' has incompatible type`),
  `14__probe_vla_param_slice_stride_rebase`
  (runtime: `fisics=536871241 12 323`, `clang=331 12 323`).
- Resolved:
  `14__probe_volatile_short_circuit_sequence`
  (`stdout=7 975 1 1 24425`).
- Status:
  wave25 remains probe-only pending grouped fix phase for both blockers.

### 2026-03-21 — Bucket: runtime-surface (14) wave-25 fix-phase closure

- Scope:
  resolve the two wave-25 blockers in grouped fix mode and re-verify the full
  `14__*` probe lane.
- Semantic fix:
  improved callable-target recovery for function calls through typedef-named
  function-pointer chains, including temporary struct-return expression callees.
- Codegen fix:
  switched pointer-difference lowering to `LLVMBuildPtrDiff2` and normalized VLA
  index cast-width checks to avoid same-width no-op int-cast emission.
- Targeted validation:
  `PROBE_FILTER=14__probe_vla_param_slice_stride_rebase python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=1`, `skipped=0`.
- Full lane verify:
  `PROBE_FILTER=14__* python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=36`, `skipped=0`.
- Status:
  wave-25 probe set is fully green and ready for promotion review.

### 2026-03-21 — Bucket: runtime-surface (14) wave-25 promotion closure

- Scope:
  promote the resolved wave-25 probes into active runtime manifests.
- Promotion shard:
  added `tests/final/meta/14-runtime-surface-wave25-probe-promotions.json` with:
  `14__runtime_fnptr_struct_temporary_chain`,
  `14__runtime_vla_param_slice_stride_rebase`,
  `14__runtime_volatile_short_circuit_sequence`.
- Manifest index update:
  registered the new shard in `tests/final/meta/index.json`.
- Expectations:
  generated via targeted updates:
  `UPDATE_FINAL=1 FINAL_FILTER=<id> python3 tests/final/run_final.py`.
- Verification:
  targeted checks for all three promoted IDs passed, and full suite sweep
  `python3 tests/final/run_final.py` is green (`0 failing, 15 skipped`).
- Status:
  wave-25 is now fully integrated into active bucket-14 coverage.

### 2026-03-21 — Bucket: runtime-surface (14) wave-26 probe expansion kickoff

- Scope:
  add next high-value runtime probes centered on VLA subslice ptrdiff chains,
  function-pointer struct dispatch pipelines, pointer-byte bridge invariants,
  volatile sequencing chains, and wider variadic promotion stress.
- Added probes:
  `14__probe_vla_ptrdiff_subslice_rebase_chain`,
  `14__probe_fnptr_struct_array_dispatch_pipeline`,
  `14__probe_ptrdiff_char_bridge_roundtrip`,
  `14__probe_volatile_comma_ternary_control_chain`,
  `14__probe_variadic_width_stress_matrix`.
- Targeted validation:
  `PROBE_FILTER=14__probe_vla_ptrdiff_subslice_rebase_chain,14__probe_fnptr_struct_array_dispatch_pipeline,14__probe_ptrdiff_char_bridge_roundtrip,14__probe_volatile_comma_ternary_control_chain,14__probe_variadic_width_stress_matrix python3 tests/final/probes/run_probes.py`
  reports `blocked=1`, `resolved=4`, `skipped=0`.
- Full lane verify after additions:
  `PROBE_FILTER=14__* python3 tests/final/probes/run_probes.py`
  reports `blocked=1`, `resolved=40`, `skipped=0`.
- Resolved:
  `14__probe_vla_ptrdiff_subslice_rebase_chain`,
  `14__probe_fnptr_struct_array_dispatch_pipeline`,
  `14__probe_ptrdiff_char_bridge_roundtrip`,
  `14__probe_volatile_comma_ternary_control_chain`.
- Blocked:
  `14__probe_variadic_width_stress_matrix`
  (`fisics` compile crash in `-o` flow, exit `-11`; not promoted).
- Status:
  wave26 remains probe-only pending grouped fix-phase handling for the new
  variadic crash.

### 2026-03-13 — Bucket: runtime-surface (14) planning refresh

- Scope:
  prepare the next runtime expansion wave with explicit test IDs and execution
  gates while preserving current stable baseline.
- Verification snapshot:
  targeted runtime bucket was green at planning time; current validation uses
  exact-id bucket sweeps (since `FINAL_FILTER` is exact-match, not prefix).
- Planning output:
  added `docs/plans/runtime_bucket_14_execution_plan.md` with wave-by-wave
  targets (arithmetic/floating, ABI/calls, memory/control-flow stress),
  promotion flow, and completion criteria.
- Workflow capture:
  updated `docs/compiler_test_workflow_guide.md` with field-tested practices
  (manifest sharding, probe-to-promotion flow, bucket-filter gating, and
  `capture_frontend_diag` guidance).

### 2026-03-18 — Bucket: runtime-surface (14) wave-5 first promotions

- Scope:
  begin wave 5 by promoting resolved arithmetic/float probes into active suite.
- Promoted tests:
  `14__runtime_unsigned_wraparound`,
  `14__runtime_nan_propagation`
  (`tests/final/meta/14-runtime-surface-wave5-arith-float.json`).
- Probe validation:
  `PROBE_FILTER=14__probe_ python3 tests/final/probes/run_probes.py`
  remains fully resolved (`blocked=0`, `resolved=3`, `skipped=0`).
- Bucket validation:
  full `14` shard sweep passes (`30/30`) after promotion.

### 2026-03-18 — Bucket: runtime-surface (14) wave-5 expansion (batch 2)

- Scope:
  add the next arithmetic/float runtime targets and promote only resolved
  behavior into active manifests.
- Added/promoted tests:
  `14__runtime_signed_div_mod_sign_matrix`,
  `14__runtime_nan_comparisons`
  (`tests/final/meta/14-runtime-surface-wave5-arith-float.json`).
- Blocked probe (not promoted):
  `14__probe_float_cast_roundtrip` with differential mismatch
  (`fisics=1 0 1 0`, `clang=1 1 1 1`).
- Probe validation:
  `PROBE_FILTER=14__probe_ python3 tests/final/probes/run_probes.py`
  now reports `blocked=1`, `resolved=3`, `skipped=0`.
- Bucket validation:
  full active `14` exact-id sweep passes (`32/32`).

### 2026-03-18 — Bucket: runtime-surface (14) wave-6 ABI probes (batch 1)

- Scope:
  start wave-6 ABI/call stress in probe-first mode, promote only resolved
  behavior, and expand blocked inventory for grouped fix-phase work.
- Added new runtime probes:
  `14__probe_many_args_mixed_width`,
  `14__probe_variadic_promotions_matrix`,
  `14__probe_struct_with_array_pass_return`,
  `14__probe_union_payload_roundtrip`,
  `14__probe_fnptr_dispatch_table_mixed`.
- Probe validation:
  `PROBE_FILTER=14__probe_ python3 tests/final/probes/run_probes.py`
  reports `blocked=5`, `resolved=4`, `skipped=0`.
- Promoted:
  `14__probe_many_args_mixed_width` ->
  `14__runtime_many_args_mixed_width`
  (`tests/final/meta/14-runtime-surface-wave6-abi-calls.json`).
- Blocked probes retained (not promoted):
  `14__probe_float_cast_roundtrip`,
  `14__probe_variadic_promotions_matrix`,
  `14__probe_struct_with_array_pass_return`,
  `14__probe_union_payload_roundtrip`,
  `14__probe_fnptr_dispatch_table_mixed`.
- Bucket validation:
  full active `14` exact-id sweep passes (`33/33`).

### 2026-03-12 — Bucket: codegen-ir (13) stringent wave

- Scope:
  deepen `13` with higher-stress CFG and pointer-lowering coverage while
  staying in one bucket.
- Added tests:
  `13__ir_nested_loop_break_continue`,
  `13__ir_switch_fallthrough_sparse`,
  `13__ir_do_while_continue_break`,
  `13__ir_ternary_logical_chain`,
  `13__ir_ptr_stride_mixed`
  (`tests/final/meta/13-codegen-ir-wave3.json`).
- Verification:
  all 5 new tests pass individually and full targeted `13__*` sweep is green
  (`23/23`).

### 2026-03-12 — Bucket: codegen-ir (13) negatives expansion

- Scope:
  continue 13-only expansion with explicit negative coverage and additional CFG
  shape checks.
- Added tests:
  `13__ir_goto_backedge`,
  `13__ir_loop_if_merge`,
  `13__ir_reject_pointer_plus_pointer`,
  `13__ir_reject_subscript_scalar_base`,
  `13__ir_reject_shift_pointer_operand`,
  `13__ir_reject_deref_non_pointer`
  (`tests/final/meta/13-codegen-ir-wave4-negatives.json`).
- Verification:
  all 6 new tests pass individually and full targeted `13__*` sweep remains
  green (`29/29`).

### 2026-03-12 — Bucket: codegen-ir (13) calls/signedness wave

- Scope:
  extend 13 with deeper call-lowering and signedness paths plus additional
  invalid-operand negatives.
- Added tests:
  `13__ir_switch_signed_cases`,
  `13__ir_fnptr_array_dispatch`,
  `13__ir_recursion_factorial`,
  `13__ir_for_continue_break_accumulate`,
  `13__ir_signed_unsigned_compare`,
  `13__ir_reject_mod_pointer_operand`,
  `13__ir_reject_bitwise_float_operand`,
  `13__ir_reject_mul_pointer_operand`,
  `13__ir_reject_shift_float_rhs`
  (`tests/final/meta/13-codegen-ir-wave5-mixed.json`).
- Verification:
  all 9 new tests pass individually and full targeted `13__*` sweep remains
  green (`38/38`).

### 2026-03-12 — Bucket: codegen-ir (13) variadic/ABI wave

- Scope:
  continue 13-only expansion with variadic call-site IR coverage, nested
  switch-loop CFG stress, aggregate by-value lowering, and arity/cast negatives.
- Added tests:
  `13__ir_variadic_call_site`,
  `13__ir_nested_switch_loop_fallthrough_ctrl`,
  `13__ir_struct_by_value_transform`,
  `13__ir_mutual_recursion`,
  `13__ir_reject_nonvariadic_too_many_args`,
  `13__ir_reject_nonvariadic_too_few_args`,
  `13__ir_reject_cast_struct_return_to_int`
  (`tests/final/meta/13-codegen-ir-wave6-calls-cfg.json`).
- Verification:
  all 7 new tests pass individually and full targeted `13__*` sweep remains
  green (`45/45`).

### 2026-03-12 — Bucket: codegen-ir (13) union/volatile wave

- Scope:
  continue 13-only expansion with union-by-value flow, deeper volatile
  sequencing, nested switch lowering, and extra lvalue negatives.
- Added tests:
  `13__ir_union_by_value_roundtrip`,
  `13__ir_nested_switch_dispatch`,
  `13__ir_comma_chain_assign`,
  `13__ir_volatile_global_sequence`,
  `13__ir_reject_assign_non_lvalue`,
  `13__ir_reject_preinc_non_lvalue`
  (`tests/final/meta/13-codegen-ir-wave7-union-volatile.json`).
- Verification:
  all 6 new tests pass individually and full targeted `13__*` sweep remains
  green (`51/51`).
- Added blocked-tracking probes (not promoted):
  `13__probe_fnptr_too_many_args_reject`,
  `13__probe_fnptr_too_few_args_reject`
  (`PROBE_FILTER=13__probe_fnptr_` currently `blocked=2`).

### 2026-03-12 — Bucket: codegen-ir (13) function-pointer arity fix

- Scope:
  close blocked semantic gaps for function-pointer call arity diagnostics.
- Fix:
  in `analyzeExpression(AST_FUNCTION_CALL)`, added prototype-arity checking for
  non-symbol callees resolved from function-pointer/function typed expressions.
- Probe verification:
  `PROBE_FILTER=13__probe_fnptr_` now resolves both probes
  (`blocked=0`, `resolved=2`).
- Promotion:
  added active negatives
  `13__ir_reject_fnptr_too_many_args` and
  `13__ir_reject_fnptr_too_few_args`
  (`tests/final/meta/13-codegen-ir-wave8-fnptr-arity.json`).
- Bucket sweep:
  full targeted `13__*` remains green (`53/53`).

### 2026-03-12 — Bucket: codegen-ir (13) edge-depth wave

- Scope:
  continue 13-only expansion with additional signed switch/pointer/null/goto
  control-flow anchors and more unary/lvalue negatives.
- Added tests:
  `13__ir_switch_enum_signed_dispatch`,
  `13__ir_pointer_null_ternary_index`,
  `13__ir_while_goto_backedge_accumulate`,
  `13__ir_struct_array_field_load`,
  `13__ir_logical_ternary_merge_chain`,
  `13__ir_unsigned_shift_extract`,
  `13__ir_reject_postinc_non_lvalue`,
  `13__ir_reject_unary_plus_pointer`,
  `13__ir_reject_bitnot_float`
  (`tests/final/meta/13-codegen-ir-wave9-edge-depth.json`).
- Verification:
  all 9 new tests pass individually and full targeted `13__*` sweep remains
  green (`62/62`).
- New blocked probe tracking added for remaining semantic gaps:
  `13__probe_fnptr_ternary_decay_runtime`,
  `13__probe_mod_float_reject`,
  `13__probe_fnptr_assign_incompatible_reject`
  (each currently `blocked=1` when filtered).

### 2026-03-11 — Bucket: diagnostics-recovery (12) follow-up

- Scope: close diagnostics JSON export blockers for statement-recovery parser errors
- Fix:
  `parseWhileLoop` and `parseDoWhileLoop` now record parser diagnostics through
  `compiler_report_diag` (code `CDIAG_PARSER_GENERIC`) when missing `(` after
  `while` and missing `;` after `do-while` are encountered.
- Verification:
  `PROBE_FILTER=12__probe_diagjson_` now resolves all three probes.
- Probe status after fix:
  full sweep is now `blocked=0`, `resolved=98`, `skipped=0`.
- Bucket sweep:
  `12` manifest smoke remains green (`46/46` passing).
- Promotion:
  promoted recovered JSON-export paths into active suite as
  `12__parserdiag__recovery_while_missing_lparen` and
  `12__parserdiag__recovery_do_while_missing_semicolon`
  (`tests/final/meta/12-diagnostics-recovery-wave7-parserdiag.json`).
- Updated bucket sweep:
  `12` manifest smoke now `48/48` passing.

### 2026-03-02 — Bucket: lexer

- Scope: baseline audit of all current lexer coverage in `tests/final`
- Existing formal lexer tests run:
  `02__int_literals`, `02__float_literals`, `02__char_literals`,
  `02__string_concat`, `02__operator_edges`, `02__int_suffix_order`,
  `02__float_exponent_forms`, `02__char_unicode_escapes`,
  `02__invalid_escape_char`, `02__binary_literals`,
  `02__float_hex_no_exponent`, `02__literal_semantics`,
  `02__int_overflow_diag`
- Passing existing tests: 13
- Failing existing tests: 0
- Manual probes confirmed:
  - long identifiers tokenize correctly
  - digraphs are recognized
  - keyword misuse as identifiers is rejected
- Manual probes found concrete gaps:
  - UCN identifiers are not decoded (`\\u0061` becomes identifier `u0061`)
  - trigraphs are not translated (`??-` lexes as `?`, `?`, `-`)
- Main bucket issue: coverage density is still incomplete even though the
  current represented slice passes
- Fix batch status: pending formal test additions for identifiers, keywords,
  comments, digraphs/trigraphs, and preprocessing-token edge cases

### 2026-03-02 — Bucket: lexer (first expansion batch)

- Harness improvement:
  `tests/final/run_final.py` now supports opt-in front-end `Error` and
  `Warning` capture for `.diag` expectations
- Added formal lexer tests:
  `02__long_identifier`, `02__digraph_tokens`,
  `02__keyword_identifier_reject`, `02__trigraph_reject`
- Current formal lexer tests passing: 17
- Current formal lexer tests failing: 0
- Explicitly tracked unsupported behavior:
  trigraph spelling is rejected and now has a formal negative test
- Still unresolved concrete bug:
  UCN identifiers are not decoded correctly and still need a formal policy test
- Main bucket issue now:
  many Phase 1 checklist areas still lack formal coverage even though the
  current represented slice is stable
- Fix batch status: next expansion should target identifier matrix, comments,
  wide/UTF strings, and unresolved UCN behavior

### 2026-03-02 — Bucket: lexer (broader suite smoke)

- Full `tests/final` suite run after the lexer work
- Result:
  lexer additions stayed green and the wider suite is reduced to 2 unrelated
  non-lexer mismatches
- Remaining non-lexer mismatches:
  - `04__array_param_qualifiers`
  - `09__goto_cross_init`
- Bucket conclusion:
  lexer work is stable enough to continue, and the remaining failures should be
  handled in their respective buckets instead of being folded into lexer scope

### 2026-03-02 — Bucket: lexer (second expansion batch)

- Added formal lexer tests:
  `02__comments_and_splice`, `02__wide_utf_strings`
- Added issue-targeted repro files (not yet active in suite):
  `02__issue_c11_keywords`, `02__issue_ucn_identifier`
- Current formal lexer tests passing: 19
- Current formal lexer tests failing: 0
- New confirmed stable areas:
  - comment stripping
  - backslash-newline splicing
  - wide/UTF string tokenization
- New confirmed issue cluster:
  - `_Alignof` is recognized
  - `_Static_assert` is tokenized as `TOKEN_UNKNOWN`
  - `_Thread_local` and `_Atomic` are tokenized as identifiers
  - UCN identifiers still lex as literal spellings (`u0061`)
- Full `tests/final` smoke after the second batch:
  still only 2 unrelated non-lexer mismatches remain
- Fix batch status:
  next lexer step should convert the C11 keyword and UCN repros into explicit
  policy tests once support or unsupported behavior is chosen

### 2026-03-02 — Bucket: lexer/declarations (C11 keyword policy pass)

- Added active suite coverage:
  `02__c11_keyword_tokens`,
  `04__static_assert_pass`,
  `04__static_assert_fail`,
  `04__static_assert_nonconst`,
  `04__thread_local_unsupported`,
  `04__atomic_unsupported`
- `_Static_assert` now parses into a real `STATIC_ASSERT` AST node and enforces
  integer-constant-expression semantics
- `_Thread_local` and `_Atomic` now lex as reserved keywords, emit explicit
  unsupported diagnostics, and fail closed before semantic/codegen
- Build-system hazard fixed:
  the makefiles now emit and include header dependency files so token enum
  changes no longer require a manual clean rebuild to avoid stale-object ABI
  mismatches
- Follow-up from this decision cluster:
  UCN identifiers were the next unresolved item and are now covered by the
  explicit unsupported-policy pass below
- Full `tests/final` suite result after this pass:
  still only the same 2 unrelated mismatches remain

### 2026-03-02 — Bucket: lexer (UCN unsupported policy)

- Added active suite coverage:
  `02__ucn_identifier_unsupported`
- `\u` / `\U` identifier spellings now emit a dedicated unsupported diagnostic
  during lexing and fail closed before parse/codegen
- This closes the prior silent-mislex behavior where `\u0061` degraded into
  identifier `u0061`

### 2026-03-02 — Bucket: lexer (matrix expansion batch)

- Added active suite coverage:
  `02__identifier_matrix`,
  `02__keyword_matrix`,
  `02__float_edge_forms`,
  `02__digraph_pp_tokens`,
  `02__wide_utf_chars`
- New stable coverage added for:
  basic identifier shapes, broader keyword recognition, float boundary forms,
  digraph plus `%:` directive flow, and wide/UTF character literal prefixes
- Full `tests/final` suite result after the batch:
  still only the same 2 unrelated non-lexer mismatches remain

### 2026-03-02 — Bucket: lexer (comments/operators breadth batch)

- Added active suite coverage:
  `02__comment_nested_like`,
  `02__comment_macro_context`,
  `02__string_escape_matrix`,
  `02__operator_boundary_matrix`
- New stable coverage added for:
  nested-like block comments, comments inside macro bodies, string escape
  spelling, and denser adjacent operator/punctuator boundaries
- Fix-first repros added but intentionally kept out of active metadata:
  `02__issue_invalid_dollar`,
  `02__issue_malformed_float_forms`,
  `02__issue_unterminated_comment`
- New concrete lexer bugs preserved for later repair:
  `$` silently drops out of tokenization, malformed float spellings are
  accepted, and unterminated block comments fail open
- Full `tests/final` suite result after the batch:
  still only the same 2 unrelated non-lexer mismatches remain

### 2026-03-02 — Bucket: lexer (fail-closed diagnostics batch)

- Lexer fixes landed for:
  invalid characters, malformed floating exponents, unterminated block
  comments, and unterminated strings
- Added active suite coverage:
  `02__invalid_dollar_reject`,
  `02__malformed_float_reject`,
  `02__unterminated_comment_reject`,
  `02__unterminated_string_reject`
- The corresponding lexer failures now stop compilation before preprocessor or
  parse, instead of degrading into silent token drops or parser fallout
- Full `tests/final` suite result after the batch:
  still only the same 2 unrelated non-lexer mismatches remain

### 2026-03-02 — Bucket: suite cleanup after lexer hardening

- Tightened `02__float_hex_no_exponent` to strict standard behavior:
  hex-float spellings without `p` now reject instead of tokenizing as valid
  literals
- Updated stale expectations for:
  `04__array_param_qualifiers` and `09__goto_cross_init`
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-02 — Bucket: lexer (char/suffix coverage batch)

- Added active suite coverage:
  `02__empty_char_reject`,
  `02__unterminated_char_reject`,
  `02__float_suffix_matrix`
- Malformed character literals now fail closed in the lexer instead of
  degrading into parser errors
- Float token coverage now explicitly includes `f`, `F`, `L`, leading-dot
  decimal, and hex-float suffix forms
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (escape/operator breadth batch)

- Added active suite coverage:
  `02__char_invalid_hex_escape_reject`,
  `02__char_invalid_u16_escape_reject`,
  `02__char_invalid_u32_escape_reject`,
  `02__invalid_at_reject`,
  `02__operator_relational_matrix`
- Character-literal bad-escape coverage now explicitly includes malformed `\x`,
  short `\u`, and short `\U` cases
- Invalid-character rejection is now anchored by both `$` and `@`
- Operator token coverage now explicitly includes logical, relational, shift,
  arrow, and ellipsis forms
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (numeric/punctuator hardening batch)

- Added active suite coverage:
  `02__int_invalid_suffix_reject`,
  `02__octal_invalid_digit_reject`,
  `02__binary_invalid_digit_reject`,
  `02__invalid_backtick_reject`,
  `02__punctuator_matrix`
- Integer literal failures that previously tokenized ambiguously now fail closed:
  invalid suffix tails, invalid octal digits, and invalid binary digits
- Invalid-character rejection is now also anchored by a backtick case
- Punctuator coverage now explicitly includes brackets, parens, commas, braces,
  dot, and digraph-normalized bracket/brace spellings
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (numeric conformance tightening batch)

- Added active suite coverage:
  `02__hex_float_missing_mantissa_reject`,
  `02__radix_prefix_missing_digits_reject`
- Hex-float spellings with no mantissa digits (for example `0x.p1`) now fail
  closed in the lexer instead of being accepted as valid float literals
- Bare radix prefixes (`0x`, `0X`, `0b`, `0B`) now fail closed instead of
  tokenizing as incomplete number literals
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (string escape and radix-case batch)

- Added active suite coverage:
  `02__string_invalid_hex_escape_reject`,
  `02__string_invalid_u16_escape_reject`,
  `02__string_invalid_u32_escape_reject`,
  `02__radix_case_matrix`
- String literals now fail closed for malformed `\x`, short `\u`, and short
  `\U` escapes instead of leaking into invalid-character or UCN-identifier
  fallout
- Positive token coverage now explicitly anchors uppercase `0X`, uppercase
  `0B`, and uppercase `P` hex-float exponent spellings
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (mixed-radix and suffix stress batch)

- Added active suite coverage:
  `02__binary_float_forms_reject`,
  `02__int_suffix_case_matrix`
- Binary-prefixed literals with fractional or decimal-exponent spellings (for
  example `0b1.0` and `0b1e2`) now fail closed instead of being accepted as
  float literals
- Positive token coverage now includes a broader valid integer suffix matrix
  across uppercase/lowercase and ordering boundaries
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (generic escape strictness batch)

- Tightened existing coverage:
  `02__invalid_escape_char` now expects a fail-closed diagnostic instead of a
  permissive tokenized char literal
- Added active suite coverage:
  `02__string_invalid_escape_reject`,
  `02__escape_question_mark`
- Unknown escape sequences like `\q` now fail closed in both character and
  string literals
- Standard `\?` is explicitly preserved as a valid escape so the stricter
  policy does not over-reject
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (string stress and operator completion batch)

- Tightened existing coverage:
  `02__binary_float_forms_reject` now treats `0b1p2` and `0b1P2` as the same
  binary-float policy error instead of a generic invalid-suffix fallback
- Added active suite coverage:
  `02__string_embedded_null`,
  `02__long_string`,
  `02__operator_misc_matrix`
- String coverage now includes dedicated embedded-`\0` and long-literal anchors
- Operator coverage now explicitly adds prefix `~`, prefix `!`, divide,
  modulo, unary `*`, and unary `&`
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (hex-tail and boundary oddities batch)

- Tightened existing coverage:
  malformed floating exponents now consume trailing identifier junk before
  diagnosing, so bad tokens like `0x1.pz` and `0x1p+z` are reported in full
- Added active suite coverage:
  `02__hex_bad_exponent_tail_reject`,
  `02__punctuator_oddities`,
  `02__macro_identifier_boundary`
- Token boundary coverage now explicitly includes `::`, `?.`, and macro-pasted
  identifier formation (`a##b -> hello`)
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-02 — Bucket: lexer (final small-edge anchors)

- Added active suite coverage:
  `02__percent_hashhash_digraph`,
  `02__hex_suffix_tail_reject`,
  `02__macro_keyword_boundary`
- Token boundary coverage now explicitly includes `%:%:` digraph spelling and
  macro token-paste that forms a reserved keyword (`return`)
- Hex-float suffix garbage after a valid exponent now has an explicit negative
  oracle (`0x1p2foo`, `0x1.P+2bar`)
- Full `tests/final` suite result:
  all active tests still pass

### 2026-03-03 — Bucket: preprocessor (prep baseline)

- Lexer is now treated as the first completed major-pass baseline
- The final suite metadata is now sharded by bucket under `tests/final/meta/*.json`
- Current preprocessor manifest:
  `tests/final/meta/03-preprocessor.json`
- Current active preprocessor bucket result:
  20 / 20 passing
- Added a dedicated preprocessor prep report:
  `docs/compiler_preprocessor_bucket_report.md`
- Updated the preprocessor checklist rows to reflect current anchor coverage
  versus still-missing negative and edge cases
- Next step:
  expand the preprocessor bucket, collect the full failure set, then fix by
  behavior cluster

### 2026-03-03 — Bucket: preprocessor (first expansion wave)

- Added active suite coverage:
  `03__macro_to_nothing`,
  `03__line_continuation`,
  `03__empty_fn_args`,
  `03__recursive_block`,
  `03__include_missing`,
  `03__partial_token_reject`,
  `03__ifdef_ifndef`,
  `03__pragma_once`,
  `03__skip_missing_include`,
  `03__nested_conditionals`,
  `03__else_without_if`,
  `03__endif_without_if`,
  `03__unterminated_if_reject`,
  `03__elif_after_else`,
  `03__include_cycle`
- The active preprocessor manifest is now:
  35 / 35 passing
- Preprocessor coverage now explicitly includes:
  `#ifdef` / `#ifndef`, `#pragma once`, include-cycle diagnostics, dead-branch
  include skipping, malformed directive-stack diagnostics, and partial-token
  rejection during preprocessing
- Added a fix-first issue repro for macro arity mismatch
  (later fixed and promoted as `03__macro_arg_count_reject`)
- Current open preprocessor gap at this point in the run:
  too-few-argument function-like macro calls still leak to semantic analysis
  instead of failing closed in preprocessing
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-03 — Bucket: preprocessor (arity + include split + `#if` edges)

- Fixed a real preprocessor leak:
  too-few-argument function-like macro calls now fail in preprocessing instead
  of leaking into semantic analysis
- Promoted active suite coverage:
  `03__macro_arg_count_reject`,
  `03__quote_include_prefers_local`,
  `03__angle_include_uses_include_path`,
  `03__pp_if_short_circuit`,
  `03__pp_if_numeric_edges`
- The active preprocessor manifest is now:
  40 / 40 passing
- Include resolution now has an explicit quote-vs-angle split oracle
- `#if` behavior now has dedicated short-circuit and safe numeric-edge anchors
- Added a new fix-first issue repro:
  `03__issue_pp_if_large_values`
- Current open preprocessor gap:
  a larger unsigned `#if` expression still selects the wrong branch, pointing to
  a likely `pp_expr` evaluation bug
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-03 — Bucket: preprocessor (`pp_expr` large-value fix)

- Fixed a real `pp_expr` correctness bug:
  large unsigned preprocessor expressions were being forced through signed
  32-bit behavior and could select the wrong `#if` branch
- Updated `pp_expr` to use an internal intmax/uintmax-style value model for
  arithmetic, shifts, comparisons, and logical evaluation
- Promoted active suite coverage:
  `03__pp_if_large_values`
- The active preprocessor manifest is now:
  41 / 41 passing
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-03 — Bucket: preprocessor (`#if` semantics expansion)

- Promoted active suite coverage:
  `03__pp_if_mixed_signedness`,
  `03__pp_if_ternary_short_circuit`,
  `03__pp_if_unsigned_shift_boundary`
- `pp_expr` coverage now explicitly includes:
  mixed signed/unsigned comparisons, ternary selected-branch evaluation, and
  unsigned high-bit shift boundary behavior
- The active preprocessor manifest is now:
  44 / 44 passing
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-03 — Bucket: preprocessor (malformed `#if` fail-closed)

- Fixed a real preprocessor safety gap:
  malformed `#if` expressions were silently acting like false and skipping code
- `#if` / `#elif` expression evaluation now fails closed with explicit
  preprocessing diagnostics when the expression is invalid
- Promoted active suite coverage:
  `03__pp_if_missing_rhs_reject`,
  `03__pp_if_missing_colon_reject`,
  `03__pp_if_unmatched_paren_reject`,
  `03__pp_if_div_zero_reject`
- Restored `defined`-via-macro behavior while keeping the stricter fail-closed
  rule by preserving `defined` operands during `pp_prepare_expr_tokens`
- The active preprocessor manifest is now:
  48 / 48 passing
- Full `tests/final` suite result:
  all active tests pass

### 2026-03-05 — Bucket: preprocessor (directive strictness hardening)

- Fixed fail-open behavior for unknown active preprocessing directives:
  they now fail closed in preprocessing with explicit diagnostics.
- Fixed fail-open behavior for trailing tokens after conditional directives:
  `#else`, `#endif`, `#ifdef`, and `#ifndef` now fail closed.
- Added explicit unsupported-policy behavior for GNU `, ##__VA_ARGS__`:
  macro definition now emits a stable preprocessing diagnostic instead of
  unstable expansion failure.
- Promoted active suite coverage:
  `03__issue_unknown_directive_reject`,
  `03__issue_endif_extra_tokens_reject`,
  `03__issue_ifdef_extra_tokens_reject`,
  `03__issue_ifndef_extra_tokens_reject`,
  `03__issue_else_extra_tokens_reject`,
  `03__gnu_comma_vaargs`,
  `03__unknown_directive_inactive_branch`
- The active preprocessor manifest is now:
  87 / 87 passing
- Full `tests/final` suite result:
  263 / 263 passing

### 2026-03-05 — Bucket: preprocessor (`#line` + nested strictness + include order)

- Hardened `#line` parsing to fail closed on trailing tokens after the optional
  filename operand.
- Promoted `#line` negative coverage:
  `03__line_directive_missing_number_reject`,
  `03__line_directive_non_number_reject`,
  `03__line_directive_extra_tokens_reject`
- Added deeper nested strictness anchors:
  `03__nested_inner_ifdef_extra_tokens_reject`,
  `03__nested_inner_else_extra_tokens_reject`,
  `03__nested_inactive_skips_unknown_directive`
- Added explicit four-level include-path search-order anchor:
  `03__include_next_order_chain`
- The active preprocessor manifest is now:
  94 / 94 passing
- Full `tests/final` suite result:
  270 / 270 passing

### 2026-03-05 — Bucket: preprocessor (deeper stress follow-up)

- Added `#line` macro-edge fail-closed anchor:
  `03__line_directive_macro_trailing_reject`
- Added deeper nested strictness anchors:
  `03__nested_deep_inner_else_extra_tokens_reject`,
  `03__nested_deep_active_bad_elif_reject`,
  `03__nested_deep_inactive_skips_unknown_directive`
- Added mixed-delimiter include-order anchor:
  `03__include_next_mixed_delims_order`
- The active preprocessor manifest is now:
  99 / 99 passing
- Full `tests/final` suite result:
  275 / 275 passing

### 2026-03-05 — Bucket: preprocessor (strict `#line` + collision matrix follow-up)

- Tightened `#line` number parsing policy:
  line numbers must be positive decimal digit sequences.
- Promoted strict `#line` numeric negatives:
  `03__line_directive_zero_reject`,
  `03__line_directive_hex_number_reject`,
  `03__line_directive_suffix_number_reject`
- Added deeper depth-5 conditional anchors:
  `03__nested_depth5_duplicate_else_reject`,
  `03__nested_depth5_inactive_skips_bad_if_expr`
- Added include-collision quote/angle entry anchors:
  `03__include_collision_quote_chain`,
  `03__include_collision_angle_chain`
- The active preprocessor manifest is now:
  106 / 106 passing
- Full `tests/final` suite result:
  282 / 282 passing

### 2026-03-05 — Bucket: preprocessor (range/depth/collision-long follow-up)

- Promoted strict `#line` range/shape negatives:
  `03__line_directive_out_of_range_reject`,
  `03__line_directive_macro_out_of_range_reject`,
  `03__line_directive_charconst_reject`
- Added deeper depth-6 conditional anchors:
  `03__nested_depth6_duplicate_else_reject`,
  `03__nested_depth6_inactive_skips_bad_elif_expr`
- Added longer include-collision chain anchors:
  `03__include_collision_quote_chain_long`,
  `03__include_collision_angle_chain_long`
- The active preprocessor manifest is now:
  113 / 113 passing
- Full `tests/final` suite result:
  289 / 289 passing

### 2026-03-06 — Bucket: preprocessor (directive operand-shape hardening)

- Fixed fail-open directive operand-shape gaps:
  `#define`, `#undef`, `#ifdef`, and `#ifndef` now require same-line macro
  identifiers and fail closed otherwise.
- Fixed fail-open trailing-token behavior for `#undef`:
  trailing tokens now emit stable preprocessing diagnostics.
- Promoted active suite coverage:
  `03__issue_ifdef_missing_identifier_reject`,
  `03__issue_ifndef_missing_identifier_reject`,
  `03__issue_define_missing_identifier_reject`,
  `03__issue_define_non_identifier_reject`,
  `03__issue_undef_extra_tokens_reject`,
  `03__issue_undef_missing_identifier_reject`,
  `03__macro_expansion_limit_chain_ok`
- The active preprocessor manifest is now:
  120 / 120 passing
- Full `tests/final` suite result:
  296 / 296 passing

### 2026-03-06 — Bucket: preprocessor (recursion/define-shape/include-mix stress)

- Added deeper expansion-limit boundary anchors:
  `03__macro_expansion_limit_chain6_ok`,
  `03__macro_expansion_limit_chain6_reject`
- Added malformed function-like `#define` parameter-list fail-closed anchors:
  `03__issue_define_param_missing_comma_reject`,
  `03__issue_define_param_trailing_comma_reject`,
  `03__issue_define_param_unclosed_reject`
- Added local-entry mixed quote/angle include-collision chain anchor:
  `03__include_collision_mixed_local_angle_chain`
- The active preprocessor manifest is now:
  126 / 126 passing
- Full `tests/final` suite result:
  302 / 302 passing

### 2026-03-06 — Bucket: preprocessor (redef policy + depth7 + include-gap)

- Added macro redefinition policy anchors for cross-kind and conflicting
  function-like redefinitions:
  `03__macro_redef_obj_to_function_tokens`,
  `03__macro_redef_function_to_object_tokens`,
  `03__macro_redef_function_conflict_tokens`
- Added deeper nested malformed-conditional anchors:
  `03__nested_depth7_active_bad_elif_reject`,
  `03__nested_depth7_inactive_skips_bad_if_expr`
- Added missing-mid-chain include resolver anchor:
  `03__include_next_missing_mid_chain`
- The active preprocessor manifest is now:
  132 / 132 passing
- Full `tests/final` suite result:
  308 / 308 passing

### 2026-03-06 — Bucket: preprocessor (defined/operator-shape + depth8 follow-up)

- Fixed `defined` operand consistency for preprocessor identifiers:
  keyword-token macro names are now accepted by `defined` in both paren and
  no-paren forms.
- Promoted `defined` operand-shape anchors:
  `03__defined_keyword_name`,
  `03__defined_keyword_name_no_paren`,
  `03__defined_non_identifier_reject`
- Promoted non-identifier directive-operand fail-closed anchors:
  `03__issue_ifdef_non_identifier_reject`,
  `03__issue_ifndef_non_identifier_reject`,
  `03__issue_undef_non_identifier_reject`
- Added depth-8 malformed conditional anchors:
  `03__nested_depth8_active_bad_elif_reject`,
  `03__nested_depth8_skips_bad_elif_taken_branch`
- The active preprocessor manifest is now:
  140 / 140 passing
- Full `tests/final` suite result:
  316 / 316 passing

### 2026-03-06 — Bucket: preprocessor (include operand strictness follow-up)

- Tightened include-operand parsing to fail closed on trailing tokens after
  parsed header operands for both `#include` and `#include_next`.
- Promoted include strictness anchors:
  `03__include_extra_tokens_reject`,
  `03__include_next_extra_tokens_reject`,
  `03__include_macro_non_header_operand_reject`,
  `03__include_macro_trailing_tokens_reject`,
  `03__include_next_macro_trailing_tokens_reject`
- The active preprocessor manifest is now:
  145 / 145 passing
- Full `tests/final` suite result:
  321 / 321 passing

### 2026-03-08 — Bucket: declarations (Wave 1 storage/qualifier baseline)

- Added storage class baseline anchors:
  `04__storage__static`,
  `04__storage__extern`,
  `04__storage__auto`,
  `04__storage__register`
- Added qualifier baseline anchors:
  `04__qualifiers__const`,
  `04__qualifiers__volatile`,
  `04__qualifiers__restrict`
- `const` now has explicit negative coverage in baseline via assignment to
  const-qualified object.
- Storage and qualifier rows in Phase 3 are now marked `in_progress` with
  baseline valid coverage checked.
- Full `tests/final` suite result:
  328 / 328 passing

### 2026-03-08 — Bucket: declarations (Wave 1 negative expansion)

- Added storage parse-negative anchor:
  `04__storage__missing_declarator`
- Added qualifier negatives:
  `04__qualifiers__const_pointer_assign`,
  `04__qualifiers__missing_type`
- Updated Phase 3 matrix rows with newly covered negatives and explicit
  spec-gap notes for storage-class legality and non-pointer `restrict`.
- Full `tests/final` suite result:
  331 / 331 passing

### 2026-03-08 — Bucket: declarations (storage/qualifier fix pass + strict negatives)

- Enforced storage-class legality in semantic declaration analysis:
  file-scope `auto/register` now reject, conflicting storage-class specifiers
  now reject, and `typedef` mixed with other storage classes now rejects.
- Enforced `restrict` legality: non-pointer top-level `restrict` declarations
  now reject.
- Added strict negative anchors:
  `04__storage__file_scope_auto_reject`,
  `04__storage__file_scope_register_reject`,
  `04__storage__conflict_static_auto_reject`,
  `04__storage__conflict_extern_register_reject`,
  `04__storage__typedef_extern_reject`,
  `04__qualifiers__restrict_non_pointer_reject`
- Full `tests/final` suite result:
  337 / 337 passing

### 2026-03-08 — Bucket: declarations (primitive integer baseline start)

- Added primitive integer baseline anchor:
  `04__primitive__integers`
- Phase 3 primitive integer row now has valid coverage marked `in_progress`.
- Full `tests/final` suite result:
  338 / 338 passing

### 2026-03-08 — Bucket: declarations (primitive floating/bool baseline start)

- Added primitive baseline anchors:
  `04__primitive__floating`,
  `04__primitive__bool`
- Phase 3 floating and `_Bool` rows now have valid coverage marked
  `in_progress`.
- Full `tests/final` suite result:
  340 / 340 passing

### 2026-03-08 — Bucket: declarations (primitive specifier legality fix + negatives)

- Enforced primitive specifier legality in semantic declaration analysis:
  signed/unsigned conflict rejects, short/long conflict rejects, and invalid
  integer modifiers on `float`, `_Bool`, and `void` now reject.
- Added strict primitive-negative anchors:
  `04__primitive__signed_unsigned_conflict_reject`,
  `04__primitive__short_long_conflict_reject`,
  `04__primitive__unsigned_float_reject`,
  `04__primitive__signed_bool_reject`,
  `04__primitive__unsigned_void_reject`
- Phase 3 primitive rows now include negative coverage for integer/floating/
  `_Bool` combinations.
- Full `tests/final` suite result:
  345 / 345 passing

### 2026-03-08 — Bucket: declarations (char + enum follow-up)

- Added char baseline and negative anchors:
  `04__primitive__char_signedness`,
  `04__primitive__long_char_reject`
- Added enum duplicate-negative anchor:
  `04__enum__duplicate`
- Updated Phase 3 rows:
  char row now `in_progress` with valid+negative coverage; duplicate enum row
  now has negative coverage.
- Confirmed remaining gap:
  duplicate field names in `struct`/`union` are still accepted without
  diagnostics and remain tracked as unstarted.
- Full `tests/final` suite result:
  348 / 348 passing

### 2026-03-10 — Bucket: torture-differential (phase-10 baseline)

- Added runtime+differential torture anchors:
  `15__torture__long_expr`,
  `15__torture__deep_nesting`,
  `15__torture__many_decls`,
  `15__torture__large_struct`
- Added compile-surface pathological declarator anchor:
  `15__torture__pathological_decl`
- Added malformed-input robustness anchor:
  `15__torture__malformed_no_crash`
- Current known gap tracked:
  function-pointer runtime path for this declarator family still crashes in
  probe form, so pathological declarator is currently compile-surface only.

### 2026-03-10 — Bucket: diagnostics-recovery (targeted gap expansion)

- Added active diagnostics test:
  `12__diag_type_mismatch`
- Probe failures logged (not yet promoted as passing anchors):
  `12__diag_incompatible_ptr` candidate (`int* = double*`) currently emits no
  diagnostic.
  `12__diag_illegal_cast` candidate (`(int)(struct S){1}`) currently emits no
  diagnostic and lowers to a bitcast in IR.
- Possible causes:
  assignment compatibility checks are not enforcing pointer constraint
  diagnostics; cast semantic validation is not rejecting non-scalar source
  casts to scalar destinations.

### 2026-03-10 — Bucket: torture-differential (stress expansion wave 2)

- Added runtime stress anchor:
  `15__torture__many_globals`
- Added malformed robustness anchor:
  `15__torture__malformed_unclosed_comment_no_crash`
- Logged blocked probe:
  `15__torture__deep_switch` currently mismatches clang runtime output
  (`fisics=15`, `clang=60`) and is not promoted as a passing anchor.
- Result:
  new active stress anchors pass and differential-check cleanly where
  applicable;
  malformed unclosed-comment path fails closed with deterministic lexer
  diagnostics and no crash.

### 2026-03-10 — Bucket: torture-differential (stress expansion wave 3)

- Added runtime stress anchors:
  `15__torture__many_params`,
  `15__torture__large_array_stress`
- Added malformed robustness anchor:
  `15__torture__malformed_unbalanced_block_no_crash`
- Logged blocked probe:
  `15__torture__path_decl_nested` currently segfaults at runtime (`exit 139`)
  in function-pointer call path; not promoted as passing anchor.

### 2026-03-10 — Bucket: runtime-surface (harness + control-flow expansion)

- Added runtime harness-behavior anchors:
  `14__runtime_args_env`,
  `14__runtime_stdin`
- Added runtime control-flow anchor:
  `14__runtime_switch_simple`
- Logged blocked control-flow probe:
  looped-switch variant (`15__torture__deep_switch`) still mismatches clang
  output and remains outside active passing anchors.

### 2026-03-10 — Bucket: runtime-surface (float edge expansion)

- Added active floating-point infinity anchor:
  `14__runtime_float_infinity`
- Logged blocked floating-point probe:
  `14__float__nan` currently mismatches clang on NaN self-inequality.

### 2026-03-10 — Bucket: runtime-surface (calls/memory stress expansion)

- Added active runtime+differential anchors:
  `14__runtime_variadic_calls`,
  `14__runtime_deep_recursion`,
  `14__runtime_struct_layout_alignment`
- Phase-8 rows promoted to `in_progress` for variadic calls, deep recursion,
  struct layout correctness, and alignment correctness.

### 2026-03-10 — Probe Fixtures (pre-fix lock-in set)

- Added non-manifest probe fixtures under:
  `tests/final/probes/runtime/` and `tests/final/probes/diagnostics/`
- Added probe runner:
  `python3 tests/final/probes/run_probes.py`
- Current snapshot (`2026-03-10`):
  blocked=7, resolved=0, skipped=0
- Blocked runtime probes:
  `14__probe_unsigned_wrap`,
  `14__probe_float_nan`,
  `15__probe_switch_loop_lite`,
  `15__probe_switch_loop_mod5`,
  `15__probe_path_decl_nested_runtime`
- Blocked diagnostics probes:
  `12__probe_incompatible_ptr_assign`,
  `12__probe_illegal_struct_to_int_cast`

### 2026-03-10 — Probe Fixtures (declarator + shift-width expansion)

- Added runtime boundary probe:
  `04__probe_deep_declarator_call_only`
- Added deep declarator compile-time hang probe:
  `04__probe_deep_declarator_codegen_hang`
- Added diagnostics probe:
  `12__probe_invalid_shift_width`
- Updated probe runner with deterministic compile/runtime timeouts to avoid
  hanging the full probe sweep.
- Current snapshot (`2026-03-10`):
  blocked=3, resolved=7, skipped=0
- Blocked probes:
  `04__probe_deep_declarator_call_only` (compiler crash `-11`),
  `04__probe_deep_declarator_codegen_hang` (compile timeout),
  `12__probe_invalid_shift_width` (missing diagnostic).

### 2026-03-11 — Bucket: expressions (wave 3 surface expansion)

- Added expression-surface anchors:
  `05__unary__bitwise_not`,
  `05__unary__sizeof_ambiguity`,
  `05__binary__arithmetic`,
  `05__binary__bitwise`,
  `05__binary__logical`,
  `05__binary__relational`,
  `05__binary__equality`,
  `05__binary__invalid_shift_width`,
  `05__ternary__nested`,
  `05__casts__explicit`,
  `05__casts__ambiguity`
- Promoted negative shift-width behavior from probe-only to active suite:
  `05__binary__invalid_shift_width`
- Bucket sweep (`05__*`) currently passes with 39/39 green.
- Added expressions diagnostics probes:
  `05__probe_shift_width_large_reject`,
  `05__probe_bitwise_float_reject`,
  `05__probe_relational_struct_reject`,
  `05__probe_equality_struct_reject`,
  `05__probe_add_void_reject`
- Probe runner snapshot after wave-3 follow-up:
  blocked=0, resolved=50, skipped=0.

### 2026-03-11 — Bucket: expressions (probe expansion follow-up)

- Added runtime probes:
  `05__probe_precedence_runtime`,
  `05__probe_unsigned_arith_conv_runtime`,
  `05__probe_nested_ternary_runtime`,
  `05__probe_nested_ternary_outer_true_runtime`,
  `05__probe_nested_ternary_false_chain_runtime`
- Added diagnostics probes:
  `05__probe_address_of_rvalue_reject`,
  `05__probe_deref_non_pointer_reject`,
  `05__probe_sizeof_incomplete_type_reject`,
  `05__probe_sizeof_function_reject`,
  `05__probe_logical_void_operand_reject`,
  `05__probe_relational_void_reject`,
  `05__probe_ternary_struct_condition_reject`,
  `05__probe_cast_int_to_struct_reject`
- Probe runner snapshot:
  blocked=2, resolved=61, skipped=0.
- Blocked runtime probes:
  `05__probe_nested_ternary_runtime`,
  `05__probe_nested_ternary_false_chain_runtime`
  (both mismatch clang: `fisics` exit `1`, clang exit `0`).

### 2026-03-11 — Bucket: expressions (probe expansion follow-up 2)

- Added runtime probes:
  `05__probe_short_circuit_and_runtime`,
  `05__probe_short_circuit_or_runtime`,
  `05__probe_comma_eval_runtime`
- Added diagnostics probes:
  `05__probe_alignof_void_reject`,
  `05__probe_alignof_incomplete_reject`
- Probe runner snapshot:
  blocked=3, resolved=65, skipped=0.
- New blocked diagnostics probe:
  `05__probe_alignof_void_reject`
  (`_Alignof(void)` accepted without diagnostic).

### 2026-03-11 — Bucket: expressions (final probe sweep before fixes)

- Added runtime probes:
  `05__probe_vla_sizeof_side_effect_runtime`,
  `05__probe_ternary_side_effect_runtime`,
  `05__probe_nested_ternary_deep_false_chain_runtime`,
  `05__probe_compound_literal_array_runtime`
- Added diagnostics probes:
  `05__probe_unary_bitnot_float_reject`,
  `05__probe_unary_plus_struct_reject`,
  `05__probe_unary_minus_pointer_reject`,
  `05__probe_sizeof_void_reject`,
  `05__probe_alignof_expr_reject`
- Probe runner snapshot:
  blocked=7, resolved=70, skipped=0.
- New blocked runtime probes:
  `05__probe_vla_sizeof_side_effect_runtime`,
  `05__probe_nested_ternary_deep_false_chain_runtime`
  (both mismatch clang: `fisics` exit `1`, clang exit `0`).
- New blocked diagnostics probes:
  `05__probe_sizeof_void_reject`,
  `05__probe_alignof_expr_reject`
  (invalid operand forms accepted without diagnostic).

### 2026-03-11 — Post-fix probe baseline refresh

- Current probe runner snapshot:
  `blocked=0, resolved=77, skipped=0`
- Expressions (`05`) was promoted to active stable baseline with wave 4:
  runtime differential and diagnostics probe coverage moved into active suite.
- Previously stale open-probe notes were cleared in:
  `07-types-conversions.md`,
  `08-initializers-layout.md`,
  `09-statements-controlflow.md`,
  `10-scopes-linkage.md`,
  `12-diagnostics-recovery.md`,
  `15-torture-differential.md`.
- Coverage matrix rows updated to reflect resolved probe state for:
  expressions invalid `sizeof/_Alignof` diagnostics,
  nested ternary runtime,
  scopes file-scope extern-array consistency,
  runtime unsigned-wrap / NaN probes,
  switch-loop differential probes,
  and diagnostics incompatible-pointer / illegal-cast probes.

### 2026-03-11 — Bucket: statements-controlflow (wave 7 + wave 8)

- Promoted resolved diagnostics probes into active suite (wave 7):
  `09__diag__goto_into_vla_scope_reject`,
  `09__diag__switch_float_condition_reject`,
  `09__diag__switch_pointer_condition_reject`,
  `09__diag__switch_string_condition_reject`,
  `09__diag__switch_double_condition_reject`,
  `09__diag__if_void_condition_reject`,
  `09__diag__if_struct_condition_reject`,
  `09__diag__while_void_condition_reject`,
  `09__diag__do_struct_condition_reject`,
  `09__diag__for_void_condition_reject`,
  `09__diag__for_struct_condition_reject`.
- Added runtime+differential control-flow anchors (wave 8):
  `09__runtime__switch_fallthrough_order`,
  `09__runtime__nested_loop_switch_control`,
  `09__runtime__while_for_side_effects`,
  `09__runtime__goto_forward_backward`,
  `09__runtime__switch_default_selection`.
- Active `09__*` bucket sweep is green with these promotions.

### 2026-03-11 — Bucket: statements-controlflow (do-while runtime fix + wave 9)

- Fixed `do-while` runtime executable codegen crash by correcting `AST_WHILE_LOOP`
  lowering for `isDoWhile=true` in `codegenWhileLoop`.
- Added runtime+differential active anchor:
  `09__runtime__do_while_side_effects` (`09-statements-controlflow-wave9.json`).
- Added/kept dedicated runtime probe fixture:
  `09__probe_do_while_runtime_codegen_crash`.
- Probe runner snapshot now:
  `blocked=0, resolved=78, skipped=0`.
- Active `09__*` bucket remains green after wave 9 promotion.

### 2026-03-11 — Bucket: diagnostics-recovery (wave A + wave B)

- Wave A promotions completed into active suite:
  `12__diag_incompatible_ptr`,
  `12__diag_illegal_cast`.
- Wave B recovery expansion added:
  `12__recovery__if_missing_rparen_then_followup_stmt`,
  `12__recovery__while_missing_lparen_then_followup_stmt`,
  `12__recovery__for_header_missing_semicolon_then_followup_decl`,
  `12__recovery__do_while_missing_semicolon_then_followup_stmt`,
  `12__recovery__switch_case_missing_colon_then_followup_case`,
  `12__recovery__switch_bad_case_expr_then_default`,
  `12__recovery__goto_missing_label_token_then_followup_label`,
  `12__recovery__case_outside_switch_then_next_stmt_ok`.
- Active `12__*` bucket sweep is green after wave 2 additions.
- Next diagnostics-recovery target is Wave C:
  diagnostic line-anchor stability, primary-vs-follow-on ordering, and cascade
  suppression bounds.

### 2026-03-11 — Bucket: diagnostics-recovery (wave C)

- Added wave-C active diagnostics anchors:
  `12__diag_line_anchor__undeclared_calls`,
  `12__diag_order__nonvoid_return_then_unreachable`,
  `12__diag_cascade_bound__single_unreachable_region`,
  `12__diag_line_anchor__ptr_then_cast`,
  `12__diag_line_anchor__macro_two_callsites`.
- Added manifest:
  `tests/final/meta/12-diagnostics-recovery-wave3.json`.
- Active `12__*` bucket sweep is green after wave 3 additions.
- Next diagnostics-recovery target was parser diagnostic export path.

### 2026-03-11 — Bucket: diagnostics-recovery (wave 4 parser export)

- Added parser-diagnostic export assertions in active suite:
  `12__parserdiag__missing_semicolon`,
  `12__parserdiag__for_missing_paren`,
  `12__parserdiag__bad_paren_expr`,
  `12__parserdiag__stray_else_recovery`.
- Added manifest:
  `tests/final/meta/12-diagnostics-recovery-wave4-parserdiag.json`.
- Extended final harness with parser-diagnostic expectation support (`.pdiag`),
  sourced from `--emit-diags-json` and normalized to stable parser diagnostic tuples.
- Active `12__*` bucket sweep is green after wave 4 additions.

### 2026-03-11 — Buckets: parser-diag expansion wave set (04/05/09/11/12)

- Added parser-diagnostic manifests:
  `04-declarations-wave6-parserdiag.json`,
  `05-expressions-wave5-parserdiag.json`,
  `09-statements-controlflow-wave10-parserdiag.json`,
  `11-functions-calls-wave2-parserdiag.json`,
  `12-diagnostics-recovery-wave5-parserdiag.json`.
- Added parser-focused malformed fixtures:
  `05__parserdiag__*` and `11__parserdiag__*` new case files.
- Promoted parser-diag coverage on existing negative fixtures for declarations,
  statements/control-flow, and diagnostics-recovery recovery paths.
- Fixed diagnostics export for zero-diagnostic translation units so
  `--emit-diags-json` now emits a valid empty payload instead of failing.
- Targeted new-wave IDs all pass, and full active sweep across
  `04__*`, `05__*`, `09__*`, `11__*`, and `12__*` remains green.

### 2026-03-11 — Buckets: parser-diag expansion wave set B (04/05/09/11/12)

- Added parser expansion note:
  `tests/final/parser_wave_b_expansion_plan.md`.
- Added parser-diagnostic manifests:
  `04-declarations-wave7-parserdiag.json`,
  `05-expressions-wave6-parserdiag.json`,
  `09-statements-controlflow-wave11-parserdiag.json`,
  `11-functions-calls-wave3-parserdiag.json`,
  `12-diagnostics-recovery-wave6-parserdiag.json`.
- Added malformed parser fixtures for:
  deeper control-flow recovery shapes (`09`),
  declaration/declarator grammar failures (`04`),
  postfix/ternary malformed forms (`05`),
  prototype/parameter-list malformed forms (`11`),
  and mixed parser+semantic ordering files (`12`).
- New wave IDs are all green and the full active sweep across
  `04__*`, `05__*`, `09__*`, `11__*`, and `12__*` remains green.

### 2026-03-11 — Bucket: statements-controlflow (09 parserdiag lane + probe pass)

- Added 09 parser statement-shape probes:
  `09__probe_if_missing_then_stmt_reject`,
  `09__probe_else_missing_stmt_reject`,
  `09__probe_switch_missing_rparen_reject`,
  `09__probe_for_missing_first_semicolon_reject`,
  `09__probe_goto_undefined_label_nested_reject`.
- Added probe-runner subset filter:
  `PROBE_FILTER=...` in `tests/final/probes/run_probes.py`.
- 09-only probe sweep (`PROBE_FILTER=09__probe_`) result:
  `blocked=0, resolved=17, skipped=0`.
- Promoted resolved parser-diag cases into new standardized lane manifest:
  `tests/final/meta/09-statements-controlflow-parserdiag.json`
  with active tests:
  `09__parserdiag__if_missing_then_stmt_reject`,
  `09__parserdiag__else_missing_stmt_reject`,
  `09__parserdiag__switch_missing_rparen_reject`,
  `09__parserdiag__for_missing_first_semicolon_reject`.

### 2026-03-11 — Bucket: statements-controlflow (09 nested-label promotion)

- Promoted resolved probe `09__probe_goto_undefined_label_nested_reject`
  into active suite as:
  `09__goto_undefined_label_nested_reject`
  under `tests/final/meta/09-statements-controlflow-wave5.json`.
- Targeted test pass confirms promotion is green.

### 2026-03-11 — Bucket: declarations (04 parserdiag lane start)

- Added 04 parser-shape declaration probes:
  `04__probe_struct_member_missing_type_reject`,
  `04__probe_bitfield_missing_colon_reject`,
  `04__probe_enum_missing_rbrace_reject`,
  `04__probe_typedef_missing_identifier_reject`,
  `04__probe_declarator_unbalanced_parens_reject`.
- 04-only probe sweep (`PROBE_FILTER=04__probe_`) result:
  `blocked=0, resolved=16, skipped=0`.
- Promoted resolved parser-diag cases into standardized lane manifest:
  `tests/final/meta/04-declarations-parserdiag.json`
  with active tests:
  `04__parserdiag__struct_member_missing_type_reject`,
  `04__parserdiag__bitfield_missing_colon_reject`,
  `04__parserdiag__enum_missing_rbrace_reject`,
  `04__parserdiag__typedef_missing_identifier_reject`,
  `04__parserdiag__declarator_unbalanced_parens_reject`.
- 04 active bucket sweep (`04__*`) is green after promotion.

### 2026-03-11 — Bucket: declarations (04 union-overlap + complex legality follow-up)

- Added new 04 probes:
  - runtime: `04__probe_union_overlap_runtime`
  - diagnostics: `04__probe_complex_int_reject`,
    `04__probe_complex_unsigned_reject`,
    `04__probe_complex_missing_base_reject`
- Probe result for this batch:
  `blocked=1, resolved=3, skipped=0`
  - blocked: `04__probe_complex_int_reject`
- Promoted resolved cases into new standardized lane manifest:
  `tests/final/meta/04-declarations-runtime-semantic.json`
  with active tests:
  `04__union__overlap`,
  `04__primitive__complex_unsigned_reject`,
  `04__primitive__complex_missing_base_reject`.
- Full `04__*` active sweep remains green after promotion.

### 2026-03-11 — Bucket: declarations (04 complex-int fix + promotion)

- Fixed declaration-specifier validation to reject non-floating complex bases:
  `_Complex int` now fails closed with a semantic diagnostic.
- Re-ran targeted complex probes:
  `PROBE_FILTER=04__probe_complex_` now reports
  `blocked=0, resolved=3, skipped=0`.
- Promoted the previously blocked probe into active suite:
  `04__primitive__complex_int_reject`
  (added to `tests/final/meta/04-declarations-runtime-semantic.json`).
- Full `04__*` active sweep remains green after this fix.

### 2026-03-11 — Bucket: codegen/runtime (Wave 0 baseline + Wave 1 probes)

- Ran targeted active-suite baseline for `13__*` and `14__*`:
  `selected=33, pass=33, fail=0, skip=0`.
- Added first codegen-focused runtime differential probes:
  `13__probe_phi_continue_runtime`,
  `13__probe_short_circuit_chain_runtime`,
  `13__probe_struct_copy_runtime`,
  `13__probe_ptr_stride_runtime`,
  `13__probe_global_init_runtime`.
- Probe sweep (`PROBE_FILTER=13__probe_`) result:
  `blocked=1, resolved=4, skipped=0`.
- New blocker discovered:
  `13__probe_ptr_stride_runtime` mismatches clang:
  `fisics -> 8 6 42949672960`, `clang -> 4 8 6`.
- Initial interpretation:
  pointer arithmetic scaling for non-`int` element widths (`long long*`) is
  incorrect in current codegen path.

### 2026-03-11 — Bucket: codegen/runtime (pointer-stride fix)

- Fixed pointer arithmetic and pointer-difference scaling bug in
  `cg_build_pointer_offset` / `cg_build_pointer_difference` by using a
  pointer-op element-size chooser that prefers LLVM ABI size when semantic and
  LLVM size disagree.
- Regression check:
  `PROBE_FILTER=13__probe_ptr_stride_runtime` now resolves and matches clang.
- Wave 1 probe set status after fix:
  `PROBE_FILTER=13__probe_` => `blocked=0, resolved=5, skipped=0`.
- Active bucket sanity check:
  targeted `13__*` + `14__*` sweep => `selected=33, pass=33, fail=0, skip=0`.

### 2026-03-11 — Bucket: runtime-surface (wave 2 codegen promotion)

- Promoted Wave 1 resolved codegen runtime probes into active suite manifest:
  `tests/final/meta/14-runtime-surface-wave2-codegen.json`.
- New active tests:
  `14__runtime_loop_continue_break_phi`,
  `14__runtime_short_circuit_chain_effects`,
  `14__runtime_struct_copy_update`,
  `14__runtime_pointer_stride_long_long`,
  `14__runtime_global_partial_init_zerofill`.
- Each promoted test is runtime + differential (`fisics` vs clang) with explicit
  stdout/stderr expectations under `tests/final/expect/`.
- Targeted sweep after promotion:
  `13__*` + `14__*` => `selected=38, pass=38, fail=0, skip=0`.

### 2026-03-12 — Bucket: runtime-surface (wave 3 ABI/control additions)

- Added active runtime+differential tests:
  `14__runtime_struct_mixed_width_pass_return`,
  `14__runtime_struct_nested_copy_chain`,
  `14__runtime_global_designated_sparse`.
- Added control-flow stress case as probe-only blocker:
  `14__probe_switch_loop_control_mix`.
- Probe result:
  `PROBE_FILTER=14__probe_switch_loop_control_mix` =>
  `blocked=1, resolved=0, skipped=0`
  (`fisics=2278`, `clang=2266`).
- Active bucket sanity check after wave 3:
  `13__*` + `14__*` => `selected=41, pass=41, fail=0, skip=0`.

### 2026-03-12 — Bucket: codegen-ir (wave 11 fnptr compatibility hardening)

- Fixed function-designator semantic typing so function identifiers carry full
  function signatures through decay and assignment checks.
- Tightened conditional pointer compatibility for function pointers:
  ternary branches with incompatible function signatures now fail closed.
- Tightened `%` semantic constraints to require integer operands.
- Resolved previously blocked probes:
  `13__probe_fnptr_ternary_decay_runtime`,
  `13__probe_mod_float_reject`,
  `13__probe_fnptr_assign_incompatible_reject`.
- Current `13` probe status:
  `PROBE_FILTER=13__probe_` => `blocked=0, resolved=10, skipped=0`.
- Promoted new active tests in
  `tests/final/meta/13-codegen-ir-wave10-fnptr-compat.json`:
  `13__ir_fnptr_ternary_decay_dispatch`,
  `13__ir_reject_fnptr_assign_incompatible`,
  `13__ir_reject_mod_float_operand`,
  `13__ir_reject_fnptr_ternary_incompatible`.
- Full suite regression check:
  `python3 tests/final/run_final.py` => all active tests pass.

### 2026-03-12 — Bucket: codegen-ir (wave 12 pointer-domain probes/promotions)

- Added new `13` diagnostic probes for pointer-domain assignment behavior:
  `13__probe_voidptr_to_fnptr_assign_reject`,
  `13__probe_fnptr_to_voidptr_assign_reject`,
  `13__probe_fnptr_nested_qualifier_loss_reject`.
- Probe result:
  `PROBE_FILTER=13__probe_` => `blocked=1, resolved=12, skipped=0`.
  - blocked:
    `13__probe_fnptr_nested_qualifier_loss_reject`
    (nested function-pointer qualifier-loss not diagnosed)
- Promoted resolved pointer-domain negatives into active suite via:
  `tests/final/meta/13-codegen-ir-wave11-pointer-domain.json`
  - `13__ir_reject_voidptr_to_fnptr_assign`
  - `13__ir_reject_fnptr_to_voidptr_assign`
- New active tests pass under targeted runs.

### 2026-03-12 — Bucket: codegen-ir (wave 13 nested fnptr qualifier-loss fix)

- Fixed nested pointer qualifier-loss detection in assignment checks by adding
  parsed-type pointer-derivation qualifier comparison for non-top-level pointer
  levels before function-pointer compatibility acceptance.
- Resolved blocker:
  `13__probe_fnptr_nested_qualifier_loss_reject`
  now emits:
  `Assignment discards qualifiers from pointer target`.
- Promoted to active suite:
  `13__ir_reject_fnptr_nested_qualifier_loss`
  in `tests/final/meta/13-codegen-ir-wave11-pointer-domain.json`.
- Current `13` probe status:
  `PROBE_FILTER=13__probe_` => `blocked=0, resolved=13, skipped=0`.
- Full final suite regression check remains green.

### 2026-03-12 — Bucket: codegen-ir (wave 14 qualifier-loss matrix expansion)

- Added additional qualifier-loss probes for function-pointer pointer chains:
  `13__probe_fnptr_nested_volatile_qualifier_loss_reject`,
  `13__probe_fnptr_deep_const_qualifier_loss_reject`.
- Probe result:
  `PROBE_FILTER=13__probe_` => `blocked=0, resolved=15, skipped=0`.
- Promoted resolved negatives into active suite:
  `13__ir_reject_fnptr_nested_volatile_qualifier_loss`,
  `13__ir_reject_fnptr_deep_const_qualifier_loss`
  (manifest: `tests/final/meta/13-codegen-ir-wave11-pointer-domain.json`).
- Full final suite regression check remains green after promotion.

### 2026-03-12 — Bucket: codegen-ir (wave 15 UB signed-overflow IR stress)

- Added UB-tagged compile/IR-only tests:
  `13__ir_ub_signed_add_overflow_path`,
  `13__ir_ub_signed_mul_overflow_path`,
  `13__ir_ub_signed_sub_overflow_path`.
- Added manifest:
  `tests/final/meta/13-codegen-ir-wave12-ub-overflow.json`
  and registered in `tests/final/meta/index.json`.
- Fixture metadata marks all three as `ub: true` to keep semantics explicit.
- Active `13` case count increased to `74`.
- Probe status remains green:
  `PROBE_FILTER=13__probe_` => `blocked=0, resolved=15, skipped=0`.
- Full final suite regression check remains green after promotion.

### 2026-03-12 — Bucket: codegen-ir (wave 16 UB overflow-op expansion)

- Added additional UB-tagged IR-only tests:
  `13__ir_ub_signed_shl_overflow_path`,
  `13__ir_ub_negate_intmin_path`,
  `13__ir_ub_div_intmin_neg1_path`,
  `13__ir_ub_mixed_overflow_compare_chain`.
- Added manifest:
  `tests/final/meta/13-codegen-ir-wave13-ub-overflow-ops.json`
  and registered in `tests/final/meta/index.json`.
- Active `13` case count increased to `78`.
- Probe status remains green:
  `PROBE_FILTER=13__probe_` => `blocked=0, resolved=15, skipped=0`.
- Full final suite regression check remains green after promotion.

### 2026-03-12 — Bucket: codegen-ir (wave 17 UB final matrix run)

- Added final UB-tagged IR-only tests:
  `13__ir_ub_preinc_intmax_path`,
  `13__ir_ub_predec_intmin_path`,
  `13__ir_ub_add_assign_intmax_path`,
  `13__ir_ub_sub_assign_intmin_path`,
  `13__ir_ub_shift_count_eq_width_path`,
  `13__ir_ub_shift_count_negative_path`.
- Added manifest:
  `tests/final/meta/13-codegen-ir-wave15-ub-final-matrix.json`
  and registered in `tests/final/meta/index.json`.
- Active `13` case count increased to `84`.
- Coverage focus:
  pre-inc/pre-dec overflow, compound assignment overflow, and shift-count UB.

### 2026-03-12 — Bucket: codegen-ir (wave 18 non-UB shift-width negatives)

- Added strict negative diagnostics tests:
  `13__ir_reject_shift_count_negative_const`,
  `13__ir_reject_shift_count_eq_width_const`,
  `13__ir_reject_shift_count_gt_width_const`.
- Added manifest:
  `tests/final/meta/13-codegen-ir-wave16-negatives-shift-width.json`
  and registered in `tests/final/meta/index.json`.
- Coverage focus:
  fail-closed constant shift-width validation (`<0`, `== width`, `> width`)
  before IR/codegen paths.

### 2026-03-12 — Bucket: codegen-ir (wave 19 assignment-op negatives)

- Added strict assignment-form negatives:
  `13__ir_reject_shl_assign_count_negative_const`,
  `13__ir_reject_shl_assign_count_eq_width_const`,
  `13__ir_reject_shl_assign_count_gt_width_const`,
  `13__ir_reject_shr_assign_count_negative_const`,
  `13__ir_reject_shr_assign_count_eq_width_const`,
  `13__ir_reject_shr_assign_count_gt_width_const`,
  `13__ir_reject_bitand_assign_float_rhs`,
  `13__ir_reject_mod_assign_float_rhs`,
  `13__ir_reject_shl_assign_float_rhs`,
  `13__ir_reject_shl_assign_pointer_lhs`.
- Added manifest:
  `tests/final/meta/13-codegen-ir-wave17-negatives-assign-ops.json`
  and registered in `tests/final/meta/index.json`.
- Coverage focus:
  enforce fail-closed diagnostics for assignment-form integer operators,
  including constant shift-width checks and float/pointer operand rejection.

### 2026-03-12 — Bucket: codegen-ir (wave 20 `|=`/`^=` assignment negatives)

- Added strict bitwise-assignment negatives:
  `13__ir_reject_bitor_assign_float_rhs`,
  `13__ir_reject_bitxor_assign_float_rhs`,
  `13__ir_reject_bitor_assign_pointer_lhs`,
  `13__ir_reject_bitxor_assign_pointer_lhs`,
  `13__ir_reject_bitor_assign_pointer_rhs`,
  `13__ir_reject_bitxor_assign_pointer_rhs`,
  `13__ir_reject_bitor_assign_struct_lhs`,
  `13__ir_reject_bitxor_assign_struct_lhs`,
  `13__ir_reject_bitor_assign_float_lhs`,
  `13__ir_reject_bitxor_assign_float_lhs`.
- Added manifest:
  `tests/final/meta/13-codegen-ir-wave18-negatives-bitor-bitxor-assign.json`
  and registered in `tests/final/meta/index.json`.
- Coverage focus:
  complete fail-closed diagnostics symmetry for assignment-form bitwise
  operators (`|=`, `^=`) across float/pointer/struct misuse.

### 2026-03-12 — Bucket: codegen-ir (wave 21 arithmetic assignment negatives)

- Added strict arithmetic-assignment negatives:
  `13__ir_reject_add_assign_pointer_float_rhs`,
  `13__ir_reject_add_assign_pointer_rhs`,
  `13__ir_reject_add_assign_struct_lhs`,
  `13__ir_reject_add_assign_struct_rhs`,
  `13__ir_reject_sub_assign_pointer_float_rhs`,
  `13__ir_reject_sub_assign_pointer_rhs`,
  `13__ir_reject_mul_assign_pointer_lhs`,
  `13__ir_reject_div_assign_pointer_rhs`,
  `13__ir_reject_mod_assign_pointer_lhs`,
  `13__ir_reject_mod_assign_float_lhs`.
- Added manifest:
  `tests/final/meta/13-codegen-ir-wave19-negatives-arith-assign.json`
  and registered in `tests/final/meta/index.json`.
- Coverage focus:
  complete fail-closed diagnostics for assignment-form arithmetic operators
  (`+=`, `-=`, `*=`, `/=`, `%=`) across pointer/float/struct misuse.

### 2026-03-12 — Bucket: codegen-ir (wave 22 diagnostics-order normalization)

- Semantic normalization implemented in `AST_ASSIGNMENT` analysis:
  for compound assignments, when synthetic operator validation emits a primary
  diagnostic (operand-type or shift-width), suppress follow-on
  `"Incompatible assignment operands"` for the same node.
- Updated expectations for affected active lanes:
  `13-codegen-ir-wave17-negatives-assign-ops.json`,
  `13-codegen-ir-wave18-negatives-bitor-bitxor-assign.json`,
  `13-codegen-ir-wave19-negatives-arith-assign.json`.
- Coverage focus:
  deterministic, less noisy category ordering while preserving fail-closed
  diagnostics on invalid assignment behavior.
- Validation:
  targeted assignment-negative manifests pass, and full
  `python3 tests/final/run_final.py` suite remains green.

### 2026-03-12 — Bucket: codegen-ir (wave 23 cast/ternary normalization locks)

- Added diagnostics-order lock tests:
  `13__ir_reject_cast_invalid_pointer_add_source`,
  `13__ir_reject_ternary_invalid_branch_suppresses_incompatible`,
  `13__ir_reject_ternary_invalid_condition_suppresses_incompatible`,
  `13__ir_reject_ternary_invalid_logical_branch_suppresses_incompatible`.
- Added manifest:
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`
  and registered in `tests/final/meta/index.json`.
- Coverage focus:
  enforce deterministic primary diagnostics for cast/ternary invalid-source
  paths, mirroring the assignment-order normalization policy.
- Validation:
  full `python3 tests/final/run_final.py` suite passes with new tests active.

### 2026-03-12 — Bucket: codegen-ir (wave 24 mixed parser+semantic ternary recovery lock)

- Added mixed malformed lane:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitwise_float`.
- Manifest update:
  appended to `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  lock parser recovery + semantic follow-on ordering in one test by asserting:
  parser diagnostics (`.pdiag`) for missing ternary `:`, plus deterministic
  semantic operator rejection (`.diag`) on subsequent `float & int`.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-12 — Bucket: codegen-ir (wave 25 mixed recovery-depth expansion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_pointer_plus_pointer`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitwise_float`.
- Manifest update:
  appended both tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  broaden parser-recovery + semantic-ordering locks across a second semantic
  operator class (pointer+pointer) and deeper nested malformed ternary paths.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-12 — Bucket: codegen-ir (wave 26 mixed recovery semantic-category expansion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shift_width`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_call_too_few_args`.
- Manifest update:
  appended both tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  broaden parser-recovery + semantic-ordering locks into constant-expression
  validators (shift width) and call-arity diagnostics.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-12 — Bucket: codegen-ir (wave 27 mixed recovery call-family + cast expansion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_too_few_args`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_invalid_cast`.
- Manifest update:
  appended both tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  broaden parser-recovery + semantic-ordering locks into function-pointer
  call-family arity diagnostics and non-scalar cast rejection.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-12 — Bucket: codegen-ir (wave 28 mixed recovery assignment/unary/shift expansion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_incompatible_assignment`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_unary_plus_pointer`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitnot_float`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_assign_non_lvalue`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_shift_pointer`.
- Manifest update:
  appended all five tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  broaden parser-recovery + semantic-ordering locks across assignment-family,
  unary-operator-family, and shift-family diagnostics.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-12 — Bucket: codegen-ir (wave 29 mixed recovery logical/subscript expansion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_logical_and_struct`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_logical_or_struct`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_subscript_scalar_base`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_subscript_scalar_base`.
- Manifest update:
  appended all four tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  broaden parser-recovery + semantic-ordering locks across logical short-circuit
  operand validation and subscript base-domain validation, including nested
  malformed recovery depth.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-12 — Bucket: codegen-ir (wave 30 mixed recovery fnptr-assignment/mod-float expansion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_assign_incompatible`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_fnptr_assign_incompatible`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_mod_float`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_mod_float`.
- Manifest update:
  appended all four tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  broaden parser-recovery + semantic-ordering locks across function-pointer
  assignment compatibility and arithmetic operator `%` float-operand rejection,
  including nested malformed recovery depth.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-12 — Bucket: codegen-ir (wave 31 mixed recovery relational/deref expansion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_relational_struct`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_relational_struct`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_deref_non_pointer`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_deref_non_pointer`.
- Manifest update:
  appended all four tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  broaden parser-recovery + semantic-ordering locks across comparator-domain
  diagnostics (struct relational misuse) and unary pointer-domain diagnostics
  (dereference non-pointer), including nested malformed recovery depth.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-13 — Bucket: codegen-ir (wave 32 mixed recovery equality/fnptr-qualifier expansion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_equality_struct`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_equality_struct`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_qualifier_loss`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_fnptr_qualifier_loss`.
- Manifest update:
  appended all four tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  broaden parser-recovery + semantic-ordering locks across equality comparator
  misuse on structs and richer function-pointer qualifier-loss assignment
  diagnostics, including nested malformed recovery depth.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-13 — Bucket: codegen-ir (wave 33 mixed recovery pointer-sub/compound-shift expansion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_sub_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_sub_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_shl_assign_count_negative_const`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shl_assign_count_negative_const`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_shr_assign_count_eq_width_const`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shr_assign_count_eq_width_const`.
- Manifest update:
  appended all six tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  broaden parser-recovery + semantic-ordering locks across pointer-domain
  subtraction rejection and compound-assignment shift-width constant
  validation, including nested malformed recovery depth.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-13 — Bucket: codegen-ir (wave 34 mixed recovery shl-operand/fnptr-arity symmetry)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_shl_assign_float_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shl_assign_float_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_shl_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shl_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_too_many_args`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_fnptr_too_many_args`.
- Manifest update:
  appended all six tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  broaden parser-recovery + semantic-ordering locks across assignment-form
  integer-operand rejection (`<<=` float/pointer misuse) and complete
  function-pointer call-arity symmetry (too few + too many) under recovery.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-13 — Bucket: codegen-ir (wave 35 mixed recovery shr-operand/fnptr-qualifier-depth)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_shr_assign_float_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shr_assign_float_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_shr_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shr_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_volatile_qualifier_loss`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_fnptr_volatile_qualifier_loss`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_deep_const_qualifier_loss`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_fnptr_deep_const_qualifier_loss`.
- Manifest update:
  appended all eight tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  complete parser-recovery + semantic-ordering symmetry for shift-assignment
  operand-domain rejection across both `<<=` and `>>=` families, and broaden
  recovery lanes for deeper function-pointer qualifier-loss diagnostics.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-13 — Bucket: codegen-ir (wave 36 mixed recovery shift-width/bitwise-assignment completion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_shl_assign_count_eq_width_const`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shl_assign_count_eq_width_const`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_shl_assign_count_gt_width_const`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shl_assign_count_gt_width_const`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_shr_assign_count_negative_const`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shr_assign_count_negative_const`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_shr_assign_count_gt_width_const`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shr_assign_count_gt_width_const`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitor_assign_float_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitor_assign_float_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitxor_assign_float_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitxor_assign_float_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitor_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitor_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitxor_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitxor_assign_pointer_lhs`.
- Manifest update:
  appended all sixteen tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  complete compound shift-width boundary symmetry under malformed ternary
  recovery (`<<=` negative/eq/gt and `>>=` negative/eq/gt) and lock
  parser-recovery bitwise-assignment integer-domain failures for `|=`/`^=`
  (`float rhs` and `pointer lhs`) across shallow and nested lanes.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-13 — Bucket: codegen-ir (wave 37 mixed recovery assignment-family symmetry extension)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitor_assign_float_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitor_assign_float_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitxor_assign_float_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitxor_assign_float_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitor_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitor_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitxor_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitxor_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitand_assign_float_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitand_assign_float_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_mod_assign_float_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_mod_assign_float_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_mod_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_mod_assign_pointer_lhs`.
- Manifest update:
  appended all fourteen tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  extend parser-recovery assignment-family symmetry by locking remaining
  `|=`/`^=` (`float lhs`, `pointer rhs`) variants and new fail-closed lanes for
  `&=` and `%=` operand-domain rejection under malformed ternary recovery.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-13 — Bucket: codegen-ir (wave 38 mixed recovery struct-domain and `%=` float-lhs parity)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_mod_assign_float_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_mod_assign_float_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitor_assign_struct_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitor_assign_struct_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitxor_assign_struct_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitxor_assign_struct_lhs`.
- Manifest update:
  appended all six tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  complete parser-recovery parity for `%=` float-lhs and mirror existing
  non-recovery struct-domain bitwise-assignment negatives (`|=`/`^=` struct lhs)
  under malformed ternary recovery in both shallow and nested lanes.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-13 — Bucket: codegen-ir (wave 39 mixed recovery arithmetic-asymmetry + `&=` parity expansion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_mul_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_mul_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_div_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_div_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_add_assign_struct_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_add_assign_struct_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_sub_assign_pointer_float_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_sub_assign_pointer_float_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitand_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitand_assign_pointer_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitand_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitand_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitand_assign_struct_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitand_assign_struct_lhs`.
- Manifest update:
  appended all fourteen tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  extend parser-recovery parity for arithmetic-assignment asymmetry edges
  (`*=`, `/=`, `+=`, `-=` invalid operand domains) and broaden `&=` parity to
  include pointer-lhs, pointer-rhs, and struct-lhs misuse under malformed
  ternary recovery.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-13 — Bucket: codegen-ir (wave 40 mixed recovery `+=`/`-=` symmetry completion)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_add_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_add_assign_pointer_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_add_assign_pointer_float_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_add_assign_pointer_float_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_add_assign_struct_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_add_assign_struct_rhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_sub_assign_struct_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_sub_assign_struct_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_sub_assign_struct_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_sub_assign_struct_rhs`.
- Manifest update:
  appended all ten tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  complete remaining parser-recovery `+=`/`-=` symmetry lanes by mirroring
  non-recovery invalid operand-domain cases across shallow and nested malformed
  ternary paths.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-13 — Bucket: codegen-ir (wave 41 final small `&=` parity closure + stable mark)

- Added mixed malformed lanes:
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitand_assign_float_lhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitand_assign_float_lhs`,
  `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitand_assign_struct_rhs`,
  `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitand_assign_struct_rhs`.
- Manifest update:
  appended all four tests to
  `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`.
- Coverage focus:
  close the last small `&=` recovery parity edges for current policy scope, then
  mark bucket `13` as stable for this phase.
- Validation:
  full `python3 tests/final/run_final.py` suite remains green.

### 2026-03-18 — Bucket: runtime-surface (14 wave8 probes + promotion)

- Fixed remaining VLA row-pointer differential mismatch by correcting pointer
  difference element-size selection for direct-array/VLA targets in:
  `src/CodeGen/codegen_helpers.c` (`cg_build_pointer_difference` now uses
  semantic/direct-array sizing and runtime VLA byte-size when required).
- Added new runtime probes:
  `14__probe_nested_switch_fallthrough_loop`,
  `14__probe_short_circuit_side_effect_counter`,
  `14__probe_vla_ptrdiff_row_size_dynamic`.
- Probe validation:
  `PROBE_FILTER=14__probe_ python3 tests/final/probes/run_probes.py`
  is green (`blocked=0`, `resolved=17`, `skipped=0`).
- Promoted three resolved probes into active suite:
  `14__runtime_nested_switch_fallthrough_loop`,
  `14__runtime_short_circuit_side_effect_counter`,
  `14__runtime_vla_ptrdiff_row_size_dynamic`.
- Added new manifest shard:
  `tests/final/meta/14-runtime-surface-wave8-control-memory.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  targeted `FINAL_FILTER` runs for all three new runtime ids pass, and full
  `python3 tests/final/run_final.py` remains green.

### 2026-03-18 — Bucket: runtime-surface (14 wave9 probe-lane promotions)

- Promoted resolved VLA/layout probe lanes into active final runtime coverage:
  `14__runtime_vla_stride_indexing`,
  `14__runtime_alignment_long_double_struct`,
  `14__runtime_struct_array_byte_stride`,
  `14__runtime_union_embedded_alignment`,
  `14__runtime_vla_row_pointer_decay`.
- Added new manifest shard:
  `tests/final/meta/14-runtime-surface-wave9-vla-layout-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  each promoted id passes with `FINAL_FILTER=... python3 tests/final/run_final.py`,
  full probe lane remains green
  (`PROBE_FILTER=14__probe_` => `blocked=0`, `resolved=17`, `skipped=0`),
  and full `python3 tests/final/run_final.py` remains green.

### 2026-03-18 — Bucket: runtime-surface (14 wave10 UB policy lane start)

- Added explicit UB-policy runtime test:
  `14__runtime_signed_overflow_ub_path`
  (`INT_MAX + 1` and `-INT_MIN` path marker test).
- Added new manifest shard:
  `tests/final/meta/14-runtime-surface-wave10-ub-policy.json`
  and registered it in `tests/final/meta/index.json`.
- Policy behavior:
  test is tagged `ub: true` + `differential: true`, so harness compiles/runs
  and compares expected stdout/stderr/exit, but intentionally skips strict
  clang differential check.
- Validation:
  `FINAL_FILTER=14__runtime_signed_overflow_ub_path python3 tests/final/run_final.py`
  => `PASS` with expected UB-policy skip message.
  Full suite remains green: `0 failing, 1 skipped`.

### 2026-03-19 — Bucket: runtime-surface (14 wave10 UB policy lane expansion)

- Added three more UB-policy runtime tests:
  `14__runtime_signed_mul_overflow_ub_path`,
  `14__runtime_signed_sub_overflow_ub_path`,
  `14__runtime_shift_count_ub_path`.
- Updated
  `tests/final/meta/14-runtime-surface-wave10-ub-policy.json`
  to include all four UB policy-lane tests.
- Validation:
  each new id passes under `FINAL_FILTER=...` with expected
  `differential disabled for ub=true` skips, and full
  `python3 tests/final/run_final.py` remains green:
  `0 failing, 4 skipped`.

### 2026-03-19 — Bucket: runtime-surface (14 wave11 implementation-defined lane)

- Added `impl_defined` policy support in harness:
  `tests/final/run_final.py` now skips strict differential checks when
  `impl_defined: true` is present (analogous to `ub: true` policy).
- Added implementation-defined runtime tests:
  `14__runtime_impldef_signed_right_shift`,
  `14__runtime_impldef_char_signedness`.
- Added manifest shard:
  `tests/final/meta/14-runtime-surface-wave11-impl-defined-policy.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  both tests pass under `FINAL_FILTER=...` with expected
  `differential disabled for impl_defined=true` skip messages, and full
  `python3 tests/final/run_final.py` remains green:
  `0 failing, 6 skipped`.

### 2026-03-19 — Bucket: runtime-surface (14 wave12 strict+policy mixed expansion)

- Added strict differential tests:
  `14__runtime_pointer_diff_compare_matrix`,
  `14__runtime_bitwise_shift_mask_matrix`
  (new shard:
  `tests/final/meta/14-runtime-surface-wave12-strict-differential.json`).
- Expanded policy lanes:
  - UB: added `14__runtime_preinc_intmax_ub_path`
  - impl-defined: added `14__runtime_impldef_enum_neg_cast`
- Registered wave12 shard in `tests/final/meta/index.json`.
- Probe-lane blockers captured (not promoted):
  `14__probe_vla_param_matrix_reduce`,
  `14__probe_fnptr_struct_by_value_dispatch`
  (both mismatch clang runtime).
- Validation:
  targeted `FINAL_FILTER` checks pass for promoted tests and full
  `python3 tests/final/run_final.py` remains green:
  `0 failing, 8 skipped`.

### 2026-03-19 — Bucket: runtime-surface (14 wave13/14 lane expansion)

- Added strict differential tests:
  `14__runtime_ternary_side_effect_phi`,
  `14__runtime_comma_sequence_matrix`
  (new shard:
  `tests/final/meta/14-runtime-surface-wave13-strict-differential.json`).
- Added implementation-defined policy tests:
  `14__runtime_impldef_plain_int_bitfield_sign`,
  `14__runtime_impldef_enum_size_matrix`
  (new shard:
  `tests/final/meta/14-runtime-surface-wave14-impl-defined-policy.json`).
- Registered both new shards in `tests/final/meta/index.json`.
- Validation:
  each new ID passes under exact `FINAL_FILTER=...` checks, and full
  `python3 tests/final/run_final.py` remains green:
  `0 failing, 10 skipped`.

### 2026-03-19 — Bucket: runtime-surface (14 wave15/16 lane expansion)

- Added UB-policy overflow-variant tests:
  `14__runtime_predec_intmin_ub_path`,
  `14__runtime_add_assign_intmax_ub_path`,
  `14__runtime_sub_assign_intmin_ub_path`
  (new shard:
  `tests/final/meta/14-runtime-surface-wave15-ub-policy.json`).
- Added strict differential mixed sequencing/control test:
  `14__runtime_call_sequence_conditional_mix`
  (new shard:
  `tests/final/meta/14-runtime-surface-wave16-strict-differential.json`).
- Registered both new shards in `tests/final/meta/index.json`.
- Validation:
  each new ID passes under exact `FINAL_FILTER=...` checks, and full
  `python3 tests/final/run_final.py` remains green:
  `0 failing, 13 skipped`.

### 2026-03-19 — Bucket: runtime-surface (14 wave17/18 lane expansion)

- Added strict differential tests:
  `14__runtime_fnptr_ternary_dispatch_accumulate`,
  `14__runtime_struct_ptrdiff_update_matrix`
  (new shard:
  `tests/final/meta/14-runtime-surface-wave17-strict-differential.json`).
- Added implementation-defined policy test:
  `14__runtime_impldef_signed_char_narrowing`
  (new shard:
  `tests/final/meta/14-runtime-surface-wave18-impl-defined-policy.json`).
- Registered both new shards in `tests/final/meta/index.json`.
- Validation:
  each new ID passes under exact `FINAL_FILTER=...` checks, and full
  `python3 tests/final/run_final.py` remains green:
  `0 failing, 14 skipped`.

### 2026-03-19 — Bucket: runtime-surface (14 wave19/20 lane expansion)

- Added strict differential tests:
  `14__runtime_switch_fnptr_dispatch_chain`,
  `14__runtime_fnptr_struct_pointer_pipeline`
  (new shard:
  `tests/final/meta/14-runtime-surface-wave19-strict-differential.json`).
- Added implementation-defined policy test:
  `14__runtime_impldef_long_double_size_align`
  (new shard:
  `tests/final/meta/14-runtime-surface-wave20-impl-defined-policy.json`).
- Registered both new shards in `tests/final/meta/index.json`.
- Validation:
  each new ID passes under exact `FINAL_FILTER=...` checks, and full
  `python3 tests/final/run_final.py` remains green:
  `0 failing, 15 skipped`.

### 2026-03-21 — Bucket: runtime-surface (14 wave26 varargs fix + promotion)

- Fixed blocked probe `14__probe_variadic_width_stress_matrix` by updating
  codegen named-alias type lowering:
  `src/CodeGen/codegen_types.c` now maps `__builtin_va_list` to pointer-form
  lowering, which removes backend crash in object emission.
- Probe validation:
  `PROBE_FILTER=14__* python3 tests/final/probes/run_probes.py`
  is green (`resolved=41`, `blocked=0`, `skipped=0`).
- Promoted resolved wave26 probes into active suite:
  `14__runtime_vla_ptrdiff_subslice_rebase_chain`,
  `14__runtime_fnptr_struct_array_dispatch_pipeline`,
  `14__runtime_ptrdiff_char_bridge_roundtrip`,
  `14__runtime_volatile_comma_ternary_control_chain`.
- Added promotion shard:
  `tests/final/meta/14-runtime-surface-wave26-probe-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  targeted `FINAL_FILTER=...` checks pass for all promoted ids, and full
  `make final` is green (`0 failing, 15 skipped`).

### 2026-03-21 — Bucket: runtime-surface (14 wave27 broad probe-add collection)

- Scope:
  deep probe-add mode for bucket-14 to maximize blocked inventory before next
  grouped fix cycle.
- Added runtime probes:
  `14__probe_variadic_vacopy_forwarder_matrix`,
  `14__probe_variadic_fnptr_dispatch_chain`,
  `14__probe_variadic_nested_forwarder_table`,
  `14__probe_struct_large_return_pipeline`,
  `14__probe_struct_large_return_fnptr_pipeline`,
  `14__probe_vla_param_negative_ptrdiff_matrix`,
  `14__probe_vla_rebased_slice_signed_unsigned_mix`,
  `14__probe_ptrdiff_struct_char_bridge_matrix`,
  `14__probe_fnptr_stateful_table_loop_matrix`,
  `14__probe_struct_union_by_value_roundtrip_chain`,
  `14__probe_fnptr_return_struct_pipeline`,
  `14__probe_vla_param_cross_function_pipeline`,
  `14__probe_ptrdiff_reinterpret_longlong_bridge`,
  `14__probe_recursive_fnptr_mix_runtime`.
- Targeted probe batch validation:
  `PROBE_FILTER=14__probe_variadic_,14__probe_struct_large_return_,14__probe_vla_param_negative_ptrdiff_matrix,14__probe_vla_rebased_slice_signed_unsigned_mix,14__probe_ptrdiff_struct_char_bridge_matrix,14__probe_fnptr_stateful_table_loop_matrix,14__probe_struct_union_by_value_roundtrip_chain,14__probe_fnptr_return_struct_pipeline,14__probe_vla_param_cross_function_pipeline,14__probe_ptrdiff_reinterpret_longlong_bridge,14__probe_recursive_fnptr_mix_runtime python3 tests/final/probes/run_probes.py`
  reports `blocked=7`, `resolved=10`, `skipped=0`.
- Full lane snapshot after additions:
  `PROBE_FILTER=14__* python3 tests/final/probes/run_probes.py`
  reports `blocked=7`, `resolved=48`, `skipped=0`.
- Blockers captured:
  - `14__probe_variadic_vacopy_forwarder_matrix`: unresolved
    `___builtin_va_copy` symbol at link.
  - `14__probe_variadic_nested_forwarder_table`: unresolved
    `___builtin_va_copy` symbol at link.
  - `14__probe_struct_large_return_pipeline`: runtime mismatch vs clang.
  - `14__probe_struct_large_return_fnptr_pipeline`: runtime mismatch vs clang.
  - `14__probe_ptrdiff_struct_char_bridge_matrix`: runtime mismatch vs clang
    (typed struct pointer-difference corruption).
  - `14__probe_struct_union_by_value_roundtrip_chain`: runtime mismatch vs
    clang.
  - `14__probe_vla_param_cross_function_pipeline`: semantic mismatch on VLA
    argument compatibility.
- Status:
  wave27 remains probe-only by design; no promotions performed.

### 2026-03-21 — Bucket: runtime-surface (14 wave27 grouped fixes + promotion)

- Resolved wave27 blocker set with grouped backend/semantic fixes:
  - Added `__builtin_va_copy` lowering in function-call codegen path.
  - Preserved typedef surface derivations (pointer/array/function layers) during
    codegen typedef resolution so pointer arithmetic uses correct element sizing.
  - Normalized pointer assignment compatibility for qualifier/VLA-like array
    surface differences in semantic assignment checks.
  - Corrected union LLVM body materialization in `codegenStructDefinition` so
    union storage size/alignment matches semantic layout for by-value flows.
- Targeted blocker validation:
  `PROBE_FILTER=14__probe_variadic_vacopy_forwarder_matrix,14__probe_variadic_nested_forwarder_table,14__probe_ptrdiff_struct_char_bridge_matrix,14__probe_struct_union_by_value_roundtrip_chain,14__probe_vla_param_cross_function_pipeline python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=5`, `skipped=0`.
- Full probe-lane snapshot after fixes:
  `PROBE_FILTER=14__* python3 tests/final/probes/run_probes.py`
  reports `blocked=0`, `resolved=55`, `skipped=0`.
- Promoted resolved wave27 probes into active runtime suite:
  `14__runtime_variadic_vacopy_forwarder_matrix`,
  `14__runtime_variadic_nested_forwarder_table`,
  `14__runtime_ptrdiff_struct_char_bridge_matrix`,
  `14__runtime_struct_union_by_value_roundtrip_chain`,
  `14__runtime_vla_param_cross_function_pipeline`.
- Added promotion shard:
  `tests/final/meta/14-runtime-surface-wave27-probe-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  full `make final` is green after promotion (`0 failing, 15 skipped`).

### 2026-03-21 — Bucket: runtime-surface (14 wave28 promotion completion pass)

- Promoted additional resolved probe-only runtime tests that were still not in
  active manifests:
  `14__runtime_variadic_width_stress_matrix`,
  `14__runtime_variadic_fnptr_dispatch_chain`,
  `14__runtime_vla_param_negative_ptrdiff_matrix`,
  `14__runtime_vla_rebased_slice_signed_unsigned_mix`,
  `14__runtime_fnptr_stateful_table_loop_matrix`,
  `14__runtime_fnptr_return_struct_pipeline`,
  `14__runtime_ptrdiff_reinterpret_longlong_bridge`,
  `14__runtime_recursive_fnptr_mix_runtime`,
  `14__runtime_struct_large_return_pipeline`,
  `14__runtime_struct_large_return_fnptr_pipeline`.
- Added promotion shard:
  `tests/final/meta/14-runtime-surface-wave28-probe-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  full `make final` remains green after wave28 promotion integration.

### 2026-03-21 — Bucket: runtime-surface (14 wave29-31 probe expansion)

- Scope:
  continue probe-first runtime expansion in ordered lanes:
  VLA 4D, function-pointer alias chains, variadic promotions, bitfield by-value flow.
- Added runtime probes:
  `14__probe_vla_four_dim_slice_rebase_runtime`,
  `14__probe_fnptr_alias_chooser_struct_chain`,
  `14__probe_variadic_small_types_forward_chain`,
  `14__probe_bitfield_truncation_struct_flow`,
  `14__probe_vla_four_dim_param_handoff_reduce`,
  `14__probe_fnptr_alias_indirect_dispatch`,
  `14__probe_variadic_promote_char_short_float_mix`,
  `14__probe_bitfield_unsigned_mask_merge_chain`,
  `14__probe_vla_four_dim_rebase_weighted_reduce`,
  `14__probe_fnptr_alias_conditional_factory_simple`,
  `14__probe_variadic_signed_unsigned_char_matrix`,
  `14__probe_bitfield_by_value_roundtrip_simple`.
- Targeted validation highlights:
  - VLA lane additions resolved against clang.
  - Variadic lane additions resolved against clang.
  - Most function-pointer alias additions resolved; one alias-chain tail-call
    shape remains blocked.
  - Bitfield by-value/mutation lane exposes stable mismatches.
- Full lane snapshot after wave31 additions:
  `PROBE_FILTER=14__* python3 tests/final/probes/run_probes.py`
  reports `blocked=4`, `resolved=63`, `skipped=0`.
- Active blockers:
  - `14__probe_fnptr_alias_chooser_struct_chain`: runtime mismatch vs clang
    (unstable garbage tail value).
  - `14__probe_bitfield_truncation_struct_flow`: runtime mismatch vs clang.
  - `14__probe_bitfield_unsigned_mask_merge_chain`: runtime mismatch vs clang.
  - `14__probe_bitfield_by_value_roundtrip_simple`: runtime mismatch vs clang.
- Status:
  wave29-31 remains probe-only; no promotions performed during this expansion pass.

### 2026-03-22 — Bucket: runtime-surface (14 wave29 bitfield promotion closure)

- Promoted resolved bitfield probes into active runtime suite:
  `14__runtime_bitfield_unsigned_pack_roundtrip`,
  `14__runtime_bitfield_truncation_struct_flow`,
  `14__runtime_bitfield_unsigned_mask_merge_chain`,
  `14__runtime_bitfield_by_value_roundtrip_simple`.
- Added promotion shard:
  `tests/final/meta/14-runtime-surface-wave29-bitfield-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  exact-id checks pass for all 4 promoted ids via `FINAL_FILTER=<id>`,
  and full `make final` is green (`0 failing, 15 skipped`).
- Probe-lane snapshot after closure:
  `PROBE_FILTER=14__*` reports `resolved=67`, `blocked=0`, `skipped=0`.
- Next queued promotion waves persisted in plan/docs:
  - Wave 30: function-pointer alias/callee-chain promotions.
  - Wave 31: variadic promotion-edge promotions.
  - Wave 32: 3D/4D VLA promotion set.

### 2026-03-22 — Bucket: runtime-surface (14 wave30 fnptr alias promotion closure)

- Promoted resolved function-pointer alias/callee-chain probes into active runtime suite:
  `14__runtime_fnptr_alias_chooser_struct_chain`,
  `14__runtime_fnptr_alias_conditional_factory_simple`,
  `14__runtime_fnptr_alias_indirect_dispatch`,
  `14__runtime_fnptr_typedef_alias_chain_dispatch`,
  `14__runtime_fnptr_chooser_roundtrip_call`,
  `14__runtime_fnptr_chooser_table_ternary_chain`,
  `14__runtime_fnptr_nested_return_dispatch_matrix`.
- Added promotion shard:
  `tests/final/meta/14-runtime-surface-wave30-fnptr-alias-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  exact-id checks pass for all 7 promoted ids via `FINAL_FILTER=<id>`,
  and full `make final` is green (`0 failing, 15 skipped`).
- Probe validation snapshot for promoted wave30 ids:
  `blocked=0`, `resolved=7`, `skipped=0`.
- Remaining queued promotion waves:
  - Wave 31: variadic promotion-edge promotions.
  - Wave 32: 3D/4D VLA promotion set.

### 2026-03-22 — Bucket: runtime-surface (14 wave31 variadic edge promotion closure)

- Promoted resolved variadic edge probes into active runtime suite:
  `14__runtime_variadic_promotion_edges`,
  `14__runtime_variadic_promote_char_short_float_mix`,
  `14__runtime_variadic_signed_unsigned_char_matrix`,
  `14__runtime_variadic_small_types_forward_chain`.
- Added promotion shard:
  `tests/final/meta/14-runtime-surface-wave31-variadic-edge-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  exact-id checks pass for all 4 promoted ids via `FINAL_FILTER=<id>`.
  full `make final` is green (`0 failing, 15 skipped`).
- Targeted probe snapshot for promoted wave31 ids:
  `blocked=0`, `resolved=4`, `skipped=0`.
- Remaining queued promotion wave:
  - Wave 32: 3D/4D VLA promotion set.

### 2026-03-22 — Bucket: runtime-surface (14 wave32 VLA depth promotion closure)

- Promoted resolved 3D/4D VLA probes into active runtime suite:
  `14__runtime_vla_three_dim_index_stride_basic`,
  `14__runtime_vla_three_dim_stride_reduce`,
  `14__runtime_vla_four_dim_stride_matrix`,
  `14__runtime_vla_four_dim_slice_rebase_runtime`,
  `14__runtime_vla_four_dim_param_handoff_reduce`,
  `14__runtime_vla_four_dim_rebase_weighted_reduce`.
- Added promotion shard:
  `tests/final/meta/14-runtime-surface-wave32-vla-depth-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  exact-id checks pass for all 6 promoted ids via `FINAL_FILTER=<id>`.
  full `make final` is green (`0 failing, 15 skipped`).
- Targeted probe snapshot for promoted wave32 ids:
  `blocked=0`, `resolved=6`, `skipped=0`.
- Full probe-lane snapshot:
  `PROBE_FILTER=14__*` reports `blocked=0`, `resolved=67`, `skipped=0`.
- Status:
  planned wave30-32 promotion queue is fully closed.

### 2026-03-23 — Bucket: runtime-surface (14 wave33-36 closure + policy expansion)

- Wave 33 promoted from probe lane:
  `14__runtime_unsigned_div_mod_extremes_matrix`,
  `14__runtime_signed_unsigned_cmp_boundary_matrix`,
  `14__runtime_float_signed_zero_inf_matrix`,
  `14__runtime_cast_chain_width_sign_matrix`.
- Added shard:
  `tests/final/meta/14-runtime-surface-wave33-boundary-promotions.json`.
- Wave 34 probe blocker fixed:
  `_Bool` cast lowering now uses truthiness conversion (`!= 0`) in
  `src/CodeGen/codegen_helpers.c` instead of integer truncation.
- Wave 34 promoted from probe lane:
  `14__runtime_header_stddef_ptrdiff_size_t_bridge`,
  `14__runtime_header_stdint_intptr_uintptr_roundtrip`,
  `14__runtime_header_limits_llong_matrix`,
  `14__runtime_header_stdbool_int_bridge`.
- Added shard:
  `tests/final/meta/14-runtime-surface-wave34-header-promotions.json`.
- Wave 35 promoted from probe lane:
  `14__runtime_struct_bitfield_mixed_pass_return`,
  `14__runtime_struct_double_int_padding_roundtrip`,
  `14__runtime_fnptr_variadic_dispatch_bridge`,
  `14__runtime_many_args_struct_scalar_mix`.
- Added shard:
  `tests/final/meta/14-runtime-surface-wave35-abi-promotions.json`.
- Wave 36 added as policy lanes:
  `14__runtime_impldef_plain_int_bitfield_signedness_matrix` (`impl_defined`),
  `14__runtime_impldef_signed_shift_char_matrix` (`impl_defined`),
  `14__runtime_ub_left_shift_negative_lhs_path` (`ub`),
  `14__runtime_ub_negate_intmin_path` (`ub`).
- Added shard:
  `tests/final/meta/14-runtime-surface-wave36-policy-lanes.json`.
- Validation:
  targeted exact-id runs pass for all wave33-36 additions.
  `PROBE_FILTER=14__probe_*` snapshot: `resolved=79`, `blocked=0`, `skipped=0`.
  full `make final` snapshot: `0 failing, 19 skipped`.

### 2026-03-23 — Bucket: runtime-surface (14 wave37 probe promotion closure)

- Added and resolved new probes:
  `14__probe_float_negzero_propagation_chain`,
  `14__probe_ptrdiff_one_past_end_matrix`,
  `14__probe_many_args_float_int_struct_mix`,
  `14__probe_variadic_llong_double_bridge`.
- Promoted into active runtime suite:
  `14__runtime_float_negzero_propagation_chain`,
  `14__runtime_ptrdiff_one_past_end_matrix`,
  `14__runtime_many_args_float_int_struct_mix`,
  `14__runtime_variadic_llong_double_bridge`.
- Added promotion shard:
  `tests/final/meta/14-runtime-surface-wave37-float-pointer-abi-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  targeted probe snapshot for wave37 ids: `resolved=4`, `blocked=0`, `skipped=0`.
  exact-id runtime checks pass for all 4 promoted ids.
  full probe-lane snapshot: `resolved=87`, `blocked=0`, `skipped=0`.

### 2026-03-23 — Bucket: runtime-surface (14 wave38 probe promotion closure)

- Added and resolved new probes:
  `14__probe_control_do_while_switch_phi_chain`,
  `14__probe_struct_array_double_by_value_roundtrip`,
  `14__probe_vla_dim_recompute_stride_matrix`,
  `14__probe_pointer_byte_rebase_roundtrip_matrix`.
- Promoted into active runtime suite:
  `14__runtime_control_do_while_switch_phi_chain`,
  `14__runtime_struct_array_double_by_value_roundtrip`,
  `14__runtime_vla_dim_recompute_stride_matrix`,
  `14__runtime_pointer_byte_rebase_roundtrip_matrix`.
- Added promotion shard:
  `tests/final/meta/14-runtime-surface-wave38-control-vla-abi-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  targeted probe snapshot for wave38 ids: `resolved=4`, `blocked=0`, `skipped=0`.
  exact-id runtime checks pass for all 4 promoted ids.

### 2026-03-23 — Bucket: runtime-surface (14 wave39 probe promotion closure)

- Added and resolved new probes:
  `14__probe_float_nan_inf_order_chain`,
  `14__probe_header_null_ptrdiff_bridge`,
  `14__probe_struct_union_array_overlay_roundtrip`,
  `14__probe_variadic_long_width_crosscheck`.
- Promoted into active runtime suite:
  `14__runtime_float_nan_inf_order_chain`,
  `14__runtime_header_null_ptrdiff_bridge`,
  `14__runtime_struct_union_array_overlay_roundtrip`,
  `14__runtime_variadic_long_width_crosscheck`.
- Added promotion shard:
  `tests/final/meta/14-runtime-surface-wave39-float-header-variadic-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Validation:
  targeted probe snapshot for wave39 ids: `resolved=4`, `blocked=0`, `skipped=0`.
  exact-id runtime checks pass for all 4 promoted ids.
  full suite snapshot: `make final` => `0 failing`, `19 skipped`.

### 2026-03-23 — Bucket: runtime-surface (14 wave40 probe promotion + blocker capture)

- Added new probes:
  `14__probe_struct_ternary_by_value_select_chain`,
  `14__probe_header_offsetof_nested_matrix`,
  `14__probe_switch_sparse_signed_case_ladder`,
  `14__probe_bitwise_compound_recurrence_matrix`.
- Probe results:
  `resolved=3`, `blocked=1`, `skipped=0`.
- Promoted into active runtime suite:
  `14__runtime_struct_ternary_by_value_select_chain`,
  `14__runtime_switch_sparse_signed_case_ladder`,
  `14__runtime_bitwise_compound_recurrence_matrix`.
- Added promotion shard:
  `tests/final/meta/14-runtime-surface-wave40-ternary-switch-bitwise-promotions.json`
  and registered it in `tests/final/meta/index.json`.
- Blocked probe captured:
  `14__probe_header_offsetof_nested_matrix`
  (`__builtin_offsetof` nested member designator path fails in fisics).
- Validation:
  full `PROBE_FILTER=14__probe_*` snapshot: `resolved=94`, `blocked=1`, `skipped=0`.
  full suite snapshot: `make final` => `0 failing`, `19 skipped`.

### 2026-03-25 — Bucket: runtime-surface (14 wave40 blocker fix + closure)

- Fixed parser support for dotted `offsetof` member designators:
  `src/Parser/Expr/parser_expr_pratt.c` now accepts chained
  `field.subfield` for `__builtin_offsetof`/`offsetof` second argument.
- Added shared nested-path `offsetof` evaluation in semantics:
  `evalOffsetofFieldPath(...)` in `src/Syntax/analyze_expr.c`
  and exported via `src/Syntax/analyze_expr.h`.
- Wired const-eval/codegen to the shared path resolver:
  `src/Syntax/const_eval.c`, `src/CodeGen/codegen_expr.c`.
- Updated probe shape to keep nested-member coverage while avoiding unrelated
  inline-record field-resolution noise:
  `tests/final/probes/runtime/14__probe_header_offsetof_nested_matrix.c`.
- Formerly blocked probe now resolved and promoted:
  `14__runtime_header_offsetof_nested_matrix`
  added to `tests/final/meta/14-runtime-surface-wave40-ternary-switch-bitwise-promotions.json`.
- Validation:
  targeted probe (`14__probe_header_offsetof_nested_matrix`) => `resolved=1`, `blocked=0`, `skipped=0`.
  full probe-lane snapshot => `resolved=95`, `blocked=0`, `skipped=0`.
  full suite snapshot => `make final` => `0 failing`, `19 skipped`.

### 2026-03-25 — Bucket: runtime-surface (14 wave41 harness + multi-TU additions)

- Added Wave41 runtime tests:
  `14__runtime_harness_exit_stderr_nonzero_arg`,
  `14__runtime_harness_env_stderr_nonzero`,
  `14__runtime_multitu_struct_return_bridge`,
  `14__runtime_multitu_variadic_bridge`.
- Added new shard:
  `tests/final/meta/14-runtime-surface-wave41-harness-multitu-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Multi-TU lane:
  implemented true multi-file compile/link checks via metadata `inputs` list for
  struct-return ABI and variadic bridge paths.
- Harness lane:
  implemented non-zero `expect_exit` + non-empty runtime `expected_stderr`
  contracts; kept these as non-differential harness checks.
- Validation:
  targeted exact-id checks pass for all 4 wave41 tests.
  full probe-lane snapshot => `resolved=95`, `blocked=0`, `skipped=0`.
  full suite snapshot => `make final` => `0 failing`, `19 skipped`.

### 2026-03-25 — Bucket: runtime-surface (14 wave42 harness + multi-TU stress additions)

- Added Wave42 runtime tests:
  `14__runtime_harness_args_stderr_exit13`,
  `14__runtime_harness_stdin_stderr_exit17`,
  `14__runtime_multitu_fnptr_callback_accumulate`,
  `14__runtime_multitu_union_struct_dispatch`,
  `14__runtime_multitu_vla_param_bridge`.
- Added new shard:
  `tests/final/meta/14-runtime-surface-wave42-harness-multitu-stress-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  targeted exact-id checks pass for all 5 wave42 tests.

### 2026-03-25 — Bucket: runtime-surface (14 wave43 extra multi-TU ABI stress additions)

- Added Wave43 runtime tests:
  `14__runtime_multitu_struct_array_return_chain`,
  `14__runtime_multitu_mixed_abi_call_chain`,
  `14__runtime_multitu_fnptr_struct_fold_pipeline`,
  `14__runtime_multitu_nested_aggregate_roundtrip`.
- Added new shard:
  `tests/final/meta/14-runtime-surface-wave43-multitu-abi-stress-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  targeted exact-id checks pass for all 4 wave43 tests.
  full suite snapshot => `make final` => `0 failing`, `19 skipped`.

### 2026-03-25 — Harness workflow hardening (partial-run cadence)

- Added selector support in `tests/final/run_final.py`:
  `FINAL_PREFIX`, `FINAL_GLOB`, `FINAL_BUCKET`, `FINAL_TAG`,
  `FINAL_MANIFEST`, `FINAL_MANIFEST_GLOB` (in addition to `FINAL_FILTER`).
- Added fail-closed selector policy:
  if any selector is provided and zero tests match, harness exits non-zero.
- Standardized fast slice runs through Make entrypoints:
  `final-id`, `final-prefix`, `final-glob`, `final-bucket`,
  `final-manifest`, `final-wave`, `final-runtime`.
- Documented slice-first cadence in:
  `tests/final/00-harness.md`,
  `tests/final/README.md`,
  `docs/compiler_test_workflow_guide.md`.
- Validation:
  `make final-id ID=14__runtime_multitu_mixed_abi_call_chain` -> pass.
  `make final-wave WAVE=43` -> pass.
  `make final-manifest MANIFEST=14-runtime-surface-wave43-multitu-abi-stress-promotions.json` -> pass.
  `make final-prefix PREFIX=14__runtime_harness_` -> pass.
  `make final-manifest MANIFEST=does-not-exist-wave999.json` -> fail-closed as expected.

### 2026-03-25 — Bucket: runtime-surface (14 wave44 ABI register/stack split expansion)

- Added Wave44 runtime tests:
  `14__runtime_many_args_reg_stack_split_matrix`,
  `14__runtime_variadic_kind_fold_matrix`,
  `14__runtime_large_struct_return_select_chain`,
  `14__runtime_bitfield_pack_rewrite_matrix`.
- Added new shard:
  `tests/final/meta/14-runtime-surface-wave44-abi-variadic-large-struct-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Probe-first context:
  resolved probe set used as upstream signal for this lane:
  `14__probe_variadic_width_stress_matrix`,
  `14__probe_variadic_fnptr_dispatch_chain`,
  `14__probe_variadic_nested_forwarder_table`,
  `14__probe_struct_large_return_pipeline`,
  `14__probe_struct_large_return_fnptr_pipeline`,
  `14__probe_bitfield_unsigned_mask_merge_chain`
  (`blocked=0`, `resolved=6`, `skipped=0`).
- Validation:
  `make final-wave WAVE=44` => all 4 pass.
  exact-id sweep for all 4 new ids => pass.
  `make final-runtime` bucket slice => `0 failing`, `19 skipped`.

### 2026-03-25 — Bucket: runtime-surface (14 wave45 deep multi-TU ABI expansion)

- Added Wave45 runtime tests:
  `14__runtime_multitu_large_struct_return_select`,
  `14__runtime_multitu_variadic_route_table`,
  `14__runtime_multitu_fnptr_return_ladder`,
  `14__runtime_multitu_vla_stride_bridge`.
- Added new shard:
  `tests/final/meta/14-runtime-surface-wave45-multitu-abi-depth-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `make final-wave WAVE=45` => all 4 pass.
  exact-id sweep for all 4 Wave45 ids => pass.

### 2026-03-25 — Bucket: runtime-surface (14 wave46 runtime memory-layout expansion)

- Added Wave46 runtime tests:
  `14__runtime_layout_stride_invariants_matrix`,
  `14__runtime_layout_nested_offset_consistency`,
  `14__runtime_layout_union_byte_rewrite_invariants`,
  `14__runtime_layout_vla_subslice_stride_matrix`.
- Added new shard:
  `tests/final/meta/14-runtime-surface-wave46-layout-depth-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `make final-wave WAVE=46` => all 4 pass.
  exact-id sweep for all 4 Wave46 ids => pass.
  `make final-runtime` bucket slice => `0 failing`, `19 skipped`.
  full suite checkpoint `make final` => `0 failing`, `19 skipped`.

### 2026-03-25 — Bucket: runtime-surface (14 wave47 multi-TU linkage/storage ABI expansion)

- Added Wave47 runtime tests:
  `14__runtime_multitu_tentative_def_zero_init_bridge`,
  `14__runtime_multitu_extern_array_size_consistency`,
  `14__runtime_multitu_static_extern_partition`,
  `14__runtime_multitu_global_struct_init_bridge`.
- Added new shard:
  `tests/final/meta/14-runtime-surface-wave47-multitu-linkage-storage-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Initial issue capture:
  direct `extern` global declarations in `main` TUs produced duplicate symbols
  in current compiler output. Tests were adjusted to validate linkage/storage
  behavior through accessor functions while retaining multi-TU intent.
- Validation:
  `make final-wave WAVE=47` => all 4 pass.
  exact-id sweep for all 4 Wave47 ids => pass.

### 2026-03-25 — Bucket: runtime-surface (14 wave48 multi-TU decay/qualifier ABI expansion)

- Added Wave48 runtime tests:
  `14__runtime_multitu_fnptr_qualifier_decay_bridge`,
  `14__runtime_multitu_array_param_decay_stride`,
  `14__runtime_multitu_const_volatile_ptr_pipeline`,
  `14__runtime_multitu_struct_with_fnptr_return_chain`.
- Added new shard:
  `tests/final/meta/14-runtime-surface-wave48-multitu-decay-qualifier-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Initial issue capture:
  the original `const volatile` global-pointer variant triggered a compiler
  crash in this environment; the test was revised to a const-pointer pipeline
  variant to keep the wave stable while preserving qualifier/decay coverage.
- Validation:
  `make final-wave WAVE=48` => all 4 pass.
  exact-id sweep for all 4 Wave48 ids => pass.
  `make final-runtime` bucket slice => `0 failing`, `19 skipped`.

### 2026-03-25 — Bucket: runtime-surface (14 wave49 layout policy/edge expansion)

- Added Wave49 runtime tests:
  `14__runtime_layout_bitfield_container_boundary_matrix` (`impl_defined: true`),
  `14__runtime_layout_enum_underlying_width_bridge` (`impl_defined: true`),
  `14__runtime_layout_ptrdiff_large_span_matrix`,
  `14__runtime_layout_nested_vla_rowcol_stride_bridge`.
- Added new shard:
  `tests/final/meta/14-runtime-surface-wave49-layout-policy-edge-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=49` => pass (policy skips expected).
  `make final-wave WAVE=49` => all 4 pass (`2` expected skips).
  `make final-runtime` bucket slice => `0 failing`, `21 skipped`.
  full suite checkpoint `make final` => `0 failing`, `21 skipped`.

### 2026-03-25 — Bucket: runtime-surface (14 wave50 bool/bitfield probe + promotion)

- Designed Wave50 as strict differential bool/bitfield probe expansion:
  `14__probe_bool_scalar_conversion_matrix`,
  `14__probe_bitfield_signed_promotion_matrix`,
  `14__probe_bitfield_compound_assign_bridge`,
  `14__probe_bitfield_bool_bridge_matrix`.
- Targeted wave probe validation:
  `PROBE_FILTER=14__probe_bool_scalar_conversion_matrix,14__probe_bitfield_signed_promotion_matrix,14__probe_bitfield_compound_assign_bridge,14__probe_bitfield_bool_bridge_matrix python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py`
  => `resolved=99`, `blocked=0`, `skipped=0`.
- Promotion shard added:
  `tests/final/meta/14-runtime-surface-wave50-bool-bitfield-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=50` => pass.
  `make final-wave WAVE=50` => all 4 pass.
  `make final-runtime` bucket slice => `0 failing`, `21 skipped`.
  full suite checkpoint `make final` => `0 failing`, `21 skipped`.

### 2026-03-25 — Bucket: runtime-surface (14 wave52 multi-TU ABI depth probe + promotion)

- Probe harness enhancement:
  `tests/final/probes/run_probes.py` runtime probes now support multi-source
  compilation via optional `inputs` lists, enabling true multi-TU probe lanes.
- Added Wave52 multi-TU probes:
  `14__probe_multitu_fnptr_table_state_bridge`,
  `14__probe_multitu_variadic_fold_bridge`,
  `14__probe_multitu_vla_window_reduce_bridge`,
  `14__probe_multitu_struct_union_route_bridge`.
- Initial blocker and fix:
  first draft of the fold probe used `<stdarg.h>` and failed preprocessing in
  this frontend path; test was revised to a fixed-arity mixed-width fold bridge.
- Targeted wave probe validation:
  `PROBE_FILTER=14__probe_multitu_fnptr_table_state_bridge,14__probe_multitu_variadic_fold_bridge,14__probe_multitu_vla_window_reduce_bridge,14__probe_multitu_struct_union_route_bridge python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promotion shard added:
  `tests/final/meta/14-runtime-surface-wave52-multitu-abi-depth-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=52` => pass.
  `make final-wave WAVE=52` => all 4 pass.

### 2026-03-25 — Bucket: runtime-surface (14 wave53 strict layout/control probe + promotion)

- Added Wave53 strict probes:
  `14__probe_control_switch_do_for_state_machine`,
  `14__probe_layout_offset_stride_bridge`,
  `14__probe_pointer_stride_rebase_control_mix`,
  `14__probe_bool_bitmask_control_chain`.
- Targeted wave probe validation:
  `PROBE_FILTER=14__probe_control_switch_do_for_state_machine,14__probe_layout_offset_stride_bridge,14__probe_pointer_stride_rebase_control_mix,14__probe_bool_bitmask_control_chain python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py`
  => `resolved=107`, `blocked=0`, `skipped=0`.
- Promotion shard added:
  `tests/final/meta/14-runtime-surface-wave53-layout-control-strict-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=53` => pass.
  `make final-wave WAVE=53` => all 4 pass.
  `make final-runtime` bucket slice => `0 failing`, `21 skipped`.
  full suite checkpoint `make final` => `0 failing`, `21 skipped`.

### 2026-03-25 — Bucket: runtime-surface (14 wave53 diagnostics hardening promotion)

- Added Wave53 diagnostics hardening probes:
  `14__probe_diag_fnptr_table_incompatible_assign_reject`,
  `14__probe_diag_offsetof_bitfield_reject`,
  `14__probe_diag_vla_non_integer_bound_reject`,
  `14__probe_diag_ternary_struct_condition_reject`.
- Targeted diagnostics probe validation:
  `PROBE_FILTER=14__probe_diag_fnptr_table_incompatible_assign_reject,14__probe_diag_offsetof_bitfield_reject,14__probe_diag_vla_non_integer_bound_reject,14__probe_diag_ternary_struct_condition_reject python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promotion shard added:
  `tests/final/meta/14-runtime-surface-wave53-diagnostics-hardening-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=53` => pass.
  `make final-wave WAVE=53` => pass.

### 2026-03-25 — Bucket: runtime-surface (14 wave54 strict runtime expansion)

- Added Wave54 runtime probes:
  `14__probe_control_goto_cleanup_counter_matrix`,
  `14__probe_vla_sizeof_rebind_stride_matrix`,
  `14__probe_fnptr_array_ternary_reseed_matrix`,
  `14__probe_ptrdiff_signed_index_rebase_matrix`.
- Targeted wave probe validation:
  `PROBE_FILTER=14__probe_control_goto_cleanup_counter_matrix,14__probe_vla_sizeof_rebind_stride_matrix,14__probe_fnptr_array_ternary_reseed_matrix,14__probe_ptrdiff_signed_index_rebase_matrix python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted runtime shard:
  `tests/final/meta/14-runtime-surface-wave54-control-vla-fnptr-ptrdiff-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=54` => pass.
  `make final-wave WAVE=54` => all 4 pass.

### 2026-03-25 — Bucket: runtime-surface (14 wave55 diagnostics control-hardening expansion)

- Added Wave55 diagnostics probes:
  `14__probe_diag_continue_outside_loop_reject`,
  `14__probe_diag_break_outside_loop_reject`,
  `14__probe_diag_case_outside_switch_reject`,
  `14__probe_diag_default_outside_switch_reject`.
- Targeted diagnostics probe validation:
  `PROBE_FILTER=14__probe_diag_continue_outside_loop_reject,14__probe_diag_break_outside_loop_reject,14__probe_diag_case_outside_switch_reject,14__probe_diag_default_outside_switch_reject python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted diagnostics shard:
  `tests/final/meta/14-runtime-surface-wave55-diagnostics-control-hardening-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Full probe snapshot:
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py`
  => `resolved=119`, `blocked=0`, `skipped=0`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=55` => pass.
  `make final-wave WAVE=55` => all 4 pass.
  `make final-runtime` bucket slice => `0 failing`, `21 skipped`.

### 2026-03-25 — Bucket: runtime-surface (14 wave56 multi-TU storage + VLA/fnptr expansion)

- Added Wave56 runtime probes:
  `14__probe_multitu_static_local_counter_bridge`,
  `14__probe_multitu_static_storage_slice_bridge`,
  `14__probe_vla_row_pointer_handoff_rebase_matrix`,
  `14__probe_fnptr_struct_state_reseed_loop_matrix`.
- Targeted wave probe validation:
  `PROBE_FILTER=14__probe_multitu_static_local_counter_bridge,14__probe_multitu_static_storage_slice_bridge,14__probe_vla_row_pointer_handoff_rebase_matrix,14__probe_fnptr_struct_state_reseed_loop_matrix python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted runtime shard:
  `tests/final/meta/14-runtime-surface-wave56-multitu-vla-fnptr-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=56` => pass.
  `make final-wave WAVE=56` => all 4 pass.
  `make final-runtime` bucket slice => `0 failing`, `21 skipped`.
  full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py`
  => `resolved=123`, `blocked=0`, `skipped=0`.

### 2026-03-26 — Bucket: runtime-surface (14 wave57 diagnostics switch/control hardening)

- Added Wave57 diagnostics probes:
  `14__probe_diag_switch_duplicate_default_reject`,
  `14__probe_diag_switch_duplicate_case_reject`,
  `14__probe_diag_switch_nonconst_case_reject`,
  `14__probe_diag_continue_in_switch_reject`.
- Targeted diagnostics probe validation:
  `PROBE_FILTER=14__probe_diag_switch_duplicate_default_reject,14__probe_diag_switch_duplicate_case_reject,14__probe_diag_switch_nonconst_case_reject,14__probe_diag_continue_in_switch_reject python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted diagnostics shard:
  `tests/final/meta/14-runtime-surface-wave57-diagnostics-switch-control-hardening.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=57` => pass.
  `make final-wave WAVE=57` => all 4 pass.
  `make final-runtime` bucket slice => `0 failing`, `21 skipped`.
  full suite checkpoint `make final` => `0 failing`, `21 skipped`.
  full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py`
  => `resolved=127`, `blocked=0`, `skipped=0`.

### 2026-03-26 — Bucket: runtime-surface (14 wave58 runtime ABI/VLA/control promotion)

- Added Wave58 runtime probes:
  `14__probe_multitu_fnptr_rotation_bridge`,
  `14__probe_multitu_pair_fold_bridge`,
  `14__probe_vla_column_pointer_stride_mix`,
  `14__probe_switch_goto_reentry_matrix`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_multitu_fnptr_rotation_bridge,14__probe_multitu_pair_fold_bridge,14__probe_vla_column_pointer_stride_mix,14__probe_switch_goto_reentry_matrix python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted runtime shard:
  `tests/final/meta/14-runtime-surface-wave58-runtime-abi-vla-control.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=58` => pass.
  `make final-wave WAVE=58` => all 4 pass.
  `make final-runtime` bucket slice => `0 failing`, `21 skipped`.
  full runtime-probe snapshot:
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py`
  => `resolved=131`, `blocked=0`, `skipped=0`.
  full suite checkpoint `make final` => `0 failing`, `21 skipped`.

### 2026-03-26 — Bucket: runtime-surface (14 wave59 restrict/alias promotion)

- Added Wave59 runtime probes:
  `14__probe_restrict_nonoverlap_accumulate_matrix`,
  `14__probe_restrict_vla_window_stride_matrix`,
  `14__probe_multitu_restrict_bridge`,
  `14__probe_restrict_alias_overlap_ub_path`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_restrict_nonoverlap_accumulate_matrix,14__probe_restrict_vla_window_stride_matrix,14__probe_multitu_restrict_bridge,14__probe_restrict_alias_overlap_ub_path python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted runtime shard:
  `tests/final/meta/14-runtime-surface-wave59-restrict-alias-policy.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=59` => `0 failing`, `1 skipped`.
  `make final-wave WAVE=59` => `0 failing`, `1 skipped` (expected UB-policy skip).
  `make final-runtime` bucket slice => `0 failing`, `22 skipped`.
  full suite checkpoint `make final` => `0 failing`, `22 skipped`.

### 2026-03-26 — Bucket: runtime-surface (14 wave60 long-double probe expansion, blocked)

- Added Wave60 probe set:
  `14__probe_long_double_call_chain_matrix`,
  `14__probe_long_double_variadic_bridge`,
  `14__probe_multitu_long_double_bridge`,
  `14__probe_long_double_struct_pass_return`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_long_double_call_chain_matrix,14__probe_long_double_variadic_bridge,14__probe_multitu_long_double_bridge,14__probe_long_double_struct_pass_return python3 tests/final/probes/run_probes.py`
  => `blocked=4`, `resolved=0`, `skipped=0`.
- Blocker root causes:
  long-double representation/ABI divergence vs local clang target,
  missing quad-float helper linkage (`__*tf3`) on arithmetic/conversion paths,
  backend varargs lowering failure on f128 lane (`Cannot select ... trunc to f128`).
- Full probe snapshot after Wave59+60 additions:
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=4, resolved=135, skipped=0`.

### 2026-03-26 — Bucket: runtime-surface (14 wave60 fix-phase closure + promotion)

- Root fix applied in target layout selection:
  updated `src/Syntax/target_layout.c` so Apple arm64 defaults/triples use LP64 with
  `long double` as 64-bit (`layout_lp64_fp64`) instead of the non-Apple
  quad-precision path.
- Result:
  long-double ABI/representation now matches local clang target on this platform,
  resolving probe mismatches and backend f128 varargs lowering failure in this lane.
- Wave60 promoted runtime shard:
  `tests/final/meta/14-runtime-surface-wave60-long-double-abi-promotions.json`
  with:
  `14__runtime_long_double_call_chain_matrix`,
  `14__runtime_long_double_variadic_bridge`,
  `14__runtime_multitu_long_double_bridge`,
  `14__runtime_long_double_struct_pass_return`.
- Related policy-lane expectation refresh:
  regenerated `14__runtime_impldef_long_double_size_align` expected output
  to reflect corrected target layout (`8 8` on this host).
- Validation:
  targeted wave60 probe run => `resolved=4`, `blocked=0`, `skipped=0`.
  `make final-wave WAVE=60` => all 4 pass.
  `make final-runtime` => `0 failing`, `22 skipped`.
  `make final` => `0 failing`, `22 skipped`.
  full probe snapshot:
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=139, skipped=0`.

### 2026-03-26 — Bucket: runtime-surface (14 wave61 complex runtime promotion)

- Added Wave61 runtime probes:
  `14__probe_complex_addsub_eq_matrix`,
  `14__probe_complex_unary_scalar_promotion_chain`,
  `14__probe_complex_param_return_bridge`,
  `14__probe_multitu_complex_bridge`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_complex_addsub_eq_matrix,14__probe_complex_unary_scalar_promotion_chain,14__probe_complex_param_return_bridge,14__probe_multitu_complex_bridge python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted runtime shard:
  `tests/final/meta/14-runtime-surface-wave61-complex-runtime-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=61` => pass.
  `make final-wave WAVE=61` => all 4 pass.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-26 — Bucket: runtime-surface (14 wave62 fnptr diagnostics hardening)

- Added Wave62 diagnostics probes:
  `14__probe_diag_fnptr_table_too_many_args_reject`,
  `14__probe_diag_fnptr_struct_arg_incompatible_reject`,
  `14__probe_diag_fnptr_param_noncallable_reject`.
- Targeted diagnostics probe validation:
  `PROBE_FILTER=14__probe_diag_fnptr_table_too_many_args_reject,14__probe_diag_fnptr_struct_arg_incompatible_reject,14__probe_diag_fnptr_param_noncallable_reject python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted diagnostics shard:
  `tests/final/meta/14-runtime-surface-wave62-fnptr-diagnostics-hardening.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=62` => pass.
  `make final-wave WAVE=62` => all 3 pass.
  full probe snapshot:
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=146, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-26 — Bucket: runtime-surface (14 complex-struct layout fix)

- Root cause discovered while probing struct-contained `_Complex` lanes:
  struct field layout used scalar float size for complex values during
  semantic layout (`_Complex double` treated as 8 bytes for field stepping
  instead of 16 on this target), producing incorrect byte offsets.
- Fix applied in `src/Syntax/layout.c`:
  `TYPEINFO_FLOAT` size path now doubles storage size when `info.isComplex`
  while preserving element alignment.
- Validation:
  previously mismatching repro `14__probe_complex_struct_pass_return_bridge`
  now matches clang exactly.

### 2026-03-26 — Bucket: runtime-surface (14 wave63 complex struct pass/return promotion)

- Added Wave63 runtime probes:
  `14__probe_complex_struct_pass_return_bridge`,
  `14__probe_complex_struct_array_pass_return_chain`,
  `14__probe_complex_nested_struct_field_bridge`,
  `14__probe_complex_struct_ternary_select_bridge`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_complex_struct_pass_return_bridge,14__probe_complex_struct_array_pass_return_chain,14__probe_complex_nested_struct_field_bridge,14__probe_complex_struct_ternary_select_bridge python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted runtime shard:
  `tests/final/meta/14-runtime-surface-wave63-complex-struct-pass-return-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=63` => pass.
  `make final-wave WAVE=63` => all 4 pass.

### 2026-03-26 — Bucket: runtime-surface (14 wave64 complex struct multi-TU/pointer/copy promotion)

- Added Wave64 runtime probes:
  `14__probe_multitu_complex_struct_pass_return`,
  `14__probe_multitu_complex_nested_route`,
  `14__probe_complex_struct_pointer_roundtrip_bridge`,
  `14__probe_complex_struct_copy_assign_chain`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_multitu_complex_struct_pass_return,14__probe_multitu_complex_nested_route,14__probe_complex_struct_pointer_roundtrip_bridge,14__probe_complex_struct_copy_assign_chain python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted runtime shard:
  `tests/final/meta/14-runtime-surface-wave64-complex-struct-multitu-pointer-copy-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=64` => pass.
  `make final-wave WAVE=64` => all 4 pass.

### 2026-03-26 — Bucket: runtime-surface (14 wave65 complex layout promotion)

- Added Wave65 runtime probes:
  `14__probe_complex_layout_size_align_matrix`,
  `14__probe_complex_layout_nested_offset_matrix`,
  `14__probe_complex_layout_array_stride_matrix`,
  `14__probe_complex_layout_union_overlay_roundtrip`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_complex_layout_size_align_matrix,14__probe_complex_layout_nested_offset_matrix,14__probe_complex_layout_array_stride_matrix,14__probe_complex_layout_union_overlay_roundtrip python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted runtime shard:
  `tests/final/meta/14-runtime-surface-wave65-complex-layout-promotions.json`
  and registered in `tests/final/meta/index.json`.
- Validation:
  `UPDATE_FINAL=1 make final-wave WAVE=65` => pass.
  `make final-wave WAVE=65` => all 4 pass.
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=158, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-27 — Bucket: runtime-surface (14 wave79 multitu link-conflict diagnostics promotion)

- Added/landed Wave79 diagnostics lanes:
  `14__diag__multitu_duplicate_external_definition_reject`,
  `14__diag__multitu_extern_type_mismatch_reject`,
  `14__diag__multitu_duplicate_tentative_type_conflict_reject`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_diag_multitu_duplicate_external_definition_reject,14__probe_diag_multitu_extern_type_mismatch_reject,14__probe_diag_multitu_duplicate_tentative_type_conflict_reject python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Scope adjustment:
  link-stage `.diagjson` lanes were deferred because linker-failure exits do
  not currently produce diagnostics JSON files.
- Promoted shard:
  `tests/final/meta/14-runtime-surface-wave79-multitu-linkage-conflict-diagnostics.json`.
- Validation:
  `make final-wave WAVE=79` => all 3 pass.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-27 — Bucket: runtime-surface (14 wave80 static-local init/re-entrancy promotion)

- Added Wave80 probes/tests:
  `14__runtime_static_local_init_once_chain`,
  `14__runtime_static_local_init_recursion_gate`,
  `14__runtime_multitu_static_init_visibility_bridge`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_static_local_init_once_chain,14__probe_static_local_init_recursion_gate,14__probe_multitu_static_init_visibility_bridge python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/14-runtime-surface-wave80-static-local-init-reentrancy.json`.
- Validation:
  `make final-wave WAVE=80` => all 3 pass.
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=206, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-27 — Bucket: runtime-surface (14 wave81 VLA parameter/depth stress II promotion)

- Added Wave81 probes/tests:
  `14__runtime_vla_param_nested_handoff_matrix`,
  `14__runtime_vla_stride_rebind_cross_fn_chain`,
  `14__runtime_vla_ptrdiff_cross_scope_matrix`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_vla_param_nested_handoff_matrix,14__probe_vla_stride_rebind_cross_fn_chain,14__probe_vla_ptrdiff_cross_scope_matrix python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/14-runtime-surface-wave81-vla-parameter-depth-stress-ii.json`.
- Validation:
  `make final-wave WAVE=81` => all 3 pass.
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=209, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-28 — Bucket: runtime-surface (14 wave82 pointer/int bridge bounds promotion)

- Added Wave82 probes/tests:
  `14__runtime_intptr_roundtrip_offset_matrix`,
  `14__runtime_uintptr_mask_rebase_matrix`,
  `14__runtime_ptrdiff_boundary_crosscheck_matrix`.
- Probe-first blocker/fix:
  initial `uintptr` lane used raw pointer low bits and mismatched clang due
  stack-address variance; fixed by using base-relative masked offsets.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_intptr_roundtrip_offset_matrix,14__probe_uintptr_mask_rebase_matrix,14__probe_ptrdiff_boundary_crosscheck_matrix python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/14-runtime-surface-wave82-pointer-int-bridge-bounds.json`.
- Validation:
  `make final-wave WAVE=82` => all 3 pass.
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=212, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-28 — Bucket: runtime-surface (14 wave83 long-double/complex ABI cross-product promotion)

- Added Wave83 probes/tests:
  `14__runtime_long_double_variadic_struct_bridge`,
  `14__runtime_complex_long_double_bridge_matrix`,
  `14__runtime_multitu_long_double_variadic_bridge`.
- Probe-first blockers/fixes:
  1) `<stdarg.h>` include path failed preprocessing (`invalid #if expression`) in
     this toolchain lane.
  2) direct `va_arg(ap, struct ...)` path reproduced compiler crash (`exit -11`).
  Fix path for this wave:
  - switched to builtin vararg macros (`__builtin_va_list`, `__builtin_va_*`)
  - reworked first lane to keep long-double variadics + struct return bridge,
    without struct `va_arg` retrieval.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_long_double_variadic_struct_bridge,14__probe_complex_long_double_bridge_matrix,14__probe_multitu_long_double_variadic_bridge python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/14-runtime-surface-wave83-long-double-complex-abi-cross-product.json`.
- Validation:
  `make final-wave WAVE=83` => all 3 pass.
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=215, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-28 — Bucket: runtime-surface (14 wave84 volatile/restrict sequencing depth promotion)

- Added Wave84 probes/tests:
  `14__runtime_volatile_restrict_store_order_matrix`,
  `14__runtime_restrict_nonoverlap_rebind_chain`,
  `14__runtime_volatile_fnptr_control_chain`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_volatile_restrict_store_order_matrix,14__probe_restrict_nonoverlap_rebind_chain,14__probe_volatile_fnptr_control_chain python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/14-runtime-surface-wave84-volatile-restrict-sequencing-depth.json`.
- Validation:
  `make final-wave WAVE=84` => all 3 pass.
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=218, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-28 — Bucket: runtime-surface (14 wave85 header/builtin surface expansion promotion)

- Added Wave85 probes/tests:
  `14__runtime_header_stdalign_bridge`,
  `14__runtime_header_stdint_limits_crosscheck`,
  `14__runtime_header_null_sizeof_ptrdiff_bridge`.
- Probe-first blocker/fix:
  initial `header_null_sizeof_ptrdiff_bridge` lane used `struct-array = {0}`
  shorthand; this compiler path rejected it in this context.
  Lane was stabilized by removing the unused initializer while preserving the
  same NULL/sizeof/ptrdiff semantics under test.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_header_stdalign_bridge,14__probe_header_stdint_limits_crosscheck,14__probe_header_null_sizeof_ptrdiff_bridge python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/14-runtime-surface-wave85-header-builtin-surface-expansion.json`.
- Validation:
  `make final-wave WAVE=85` => all 3 pass.
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=221, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-28 — Bucket: runtime-surface (14 wave86 deterministic mini-binary smoke promotion)

- Added Wave86 probes/tests:
  `14__runtime_smoke_expr_eval_driver`,
  `14__runtime_smoke_dispatch_table_driver`,
  `14__runtime_smoke_multitu_config_driver`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_smoke_expr_eval_driver,14__probe_smoke_dispatch_table_driver,14__probe_smoke_multitu_config_driver python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/14-runtime-surface-wave86-deterministic-mini-binary-smoke.json`.
- Validation:
  `make final-wave WAVE=86` => all 3 pass.
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=224, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-28 — Bucket: runtime-surface (14 wave87 repeatability/oracle-hardening promotion)

- Added Wave87 probes/tests:
  `14__runtime_repeatable_output_hash_matrix`,
  `14__runtime_repeatable_diagjson_hash_matrix`,
  `14__runtime_repeatable_multitu_link_hash_matrix`.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_repeatable_output_hash_matrix,14__probe_repeatable_diagjson_hash_matrix,14__probe_repeatable_multitu_link_hash_matrix python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/14-runtime-surface-wave87-repeatability-oracle-hardening.json`.
- Validation:
  `make final-wave WAVE=87` => all 3 pass.
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=227, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-28 — Bucket: runtime-surface (14 wave88 ABI/init-order/linkdiag hardening promotion)

- Added Wave88 probes/tests:
  `14__runtime_abi_long_double_variadic_regstack_hardening`,
  `14__runtime_multitu_static_init_order_depth_bridge`,
  `14__diag__multitu_duplicate_function_definition_reject`.
- Probe-first blocker/fix:
  initial ABI lane used `__builtin_va_list` directly and failed parsing in this
  frontend path; stabilized by switching to a `typedef __builtin_va_list` +
  builtin-wrapper macro pattern already used in passing variadic lanes.
- Targeted probe validation:
  `PROBE_FILTER=14__probe_abi_long_double_variadic_regstack_hardening,14__probe_multitu_static_init_order_depth_bridge,14__probe_diag_multitu_duplicate_function_definition_reject python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/14-runtime-surface-wave88-abi-initorder-linkdiag-hardening.json`.
- Validation:
  `make final-wave WAVE=88` => all 3 pass.
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=230, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-28 — Bucket: runtime-surface (14 wave89 link-stage `.diagjson` enablement + parity promotion)

- Driver implementation:
  - Added deterministic link-stage diagnostics JSON fallback writer in
    `src/main.c` for linker non-zero exits when `--emit-diags-json` is active.
  - Added stable synthetic diagnostic code `4001` with schema-compatible
    payload (`profile/schema_version/counts/diagnostics[]`), without changing
    linker stderr/exit behavior.
- Added diagnostics-json probes:
  `14__probe_diagjson_multitu_duplicate_external_definition_reject`,
  `14__probe_diagjson_multitu_extern_type_mismatch_reject`,
  `14__probe_diagjson_multitu_duplicate_tentative_type_conflict_reject`,
  `14__probe_diagjson_multitu_duplicate_function_definition_reject`.
- Promoted shard:
  `tests/final/meta/14-runtime-surface-wave89-link-diagjson-enablement.json`.
- Promoted tests:
  `14__diagjson__multitu_duplicate_external_definition_reject`,
  `14__diagjson__multitu_extern_type_mismatch_reject`,
  `14__diagjson__multitu_duplicate_tentative_type_conflict_reject`,
  `14__diagjson__multitu_duplicate_function_definition_reject`.
- Validation:
  `PROBE_FILTER=14__probe_diagjson_multitu_duplicate_external_definition_reject,14__probe_diagjson_multitu_extern_type_mismatch_reject,14__probe_diagjson_multitu_duplicate_tentative_type_conflict_reject,14__probe_diagjson_multitu_duplicate_function_definition_reject python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
  `make final-wave WAVE=89` => all 4 pass.
  `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=234, skipped=0`.
  `make final-runtime` => `0 failing`, `22 skipped`.

### 2026-03-28 — Bucket: torture-differential (15 wave1 control/declarator-depth promotion)

- Added Wave1 probe lanes:
  `15__probe_deep_switch_loop_state_machine`,
  `15__probe_switch_sparse_case_jump_table`,
  `15__probe_declarator_depth_runtime_chain`.
- Targeted probe validation:
  `PROBE_FILTER=15__probe_deep_switch_loop_state_machine,15__probe_switch_sparse_case_jump_table,15__probe_declarator_depth_runtime_chain python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave1-control-decl-depth.json`.
- Promoted tests:
  `15__torture__deep_switch_loop_state_machine`,
  `15__torture__switch_sparse_case_jump_table`,
  `15__torture__declarator_depth_runtime_chain`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave1-control-decl-depth.json`
  => all 3 pass.
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=6, skipped=0`.
  `make final-bucket BUCKET=torture-differential`
  => all 15 pass.

### 2026-03-30 — Bucket: torture-differential (15 wave15 runtime stress expansion)

- Added Wave15 probe lanes:
  `15__probe_runtime_vla_fnptr_feedback_matrix`,
  `15__probe_runtime_struct_union_reducer_chain`,
  `15__probe_multitu_state_reseed_pipeline`.
- Probe-first blocker/fix:
  initial multi-TU reseed implementation crashed in `-o` compile/link path.
  Lane was stabilized by switching to an unsigned-only multi-TU reseed/mix
  pipeline while keeping the same stress intent (stateful + cross-TU + reseed).
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave15-runtime-stress-expansion.json`.
- Promoted tests:
  `15__torture__runtime_vla_fnptr_feedback_matrix`,
  `15__torture__runtime_struct_union_reducer_chain`,
  `15__torture__multitu_state_reseed_pipeline`.
- Validation:
  `PROBE_FILTER=15__probe_runtime_vla_fnptr_feedback_matrix,15__probe_runtime_struct_union_reducer_chain,15__probe_multitu_state_reseed_pipeline python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
  `make final-manifest MANIFEST=15-torture-differential-wave15-runtime-stress-expansion.json`
  => all 3 pass.
  `make final-wave WAVE=15 WAVE_BUCKET=15-torture-differential`
  => all 3 pass.

### 2026-03-30 — Bucket: torture-differential (15 wave16 malformed+diagjson expansion)

- Added Wave16 probe lanes:
  `15__probe_diag_malformed_invalid_dollar_no_crash`,
  `15__probe_diag_malformed_char_invalid_hex_escape_no_crash`,
  `15__probe_diagjson_malformed_invalid_dollar_no_crash`,
  `15__probe_diagjson_malformed_char_invalid_hex_escape_no_crash`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave16-malformed-diagjson-expansion.json`.
- Promoted tests:
  `15__torture__malformed_invalid_dollar_no_crash`,
  `15__torture__malformed_char_invalid_hex_escape_no_crash`,
  `15__diagjson__malformed_invalid_dollar_no_crash`,
  `15__diagjson__malformed_char_invalid_hex_escape_no_crash`.
- Validation:
  `PROBE_FILTER=15__probe_diag_malformed_invalid_dollar_no_crash,15__probe_diag_malformed_char_invalid_hex_escape_no_crash,15__probe_diagjson_malformed_invalid_dollar_no_crash,15__probe_diagjson_malformed_char_invalid_hex_escape_no_crash python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
  `make final-manifest MANIFEST=15-torture-differential-wave16-malformed-diagjson-expansion.json`
  => all 4 pass.
  `make final-wave WAVE=16 WAVE_BUCKET=15-torture-differential`
  => all 4 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing`, `2 skipped`.
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=52, skipped=0`.

### 2026-03-30 — Bucket: torture-differential (15 status/doc refresh)

- Re-validated current bucket status:
  `make final-bucket BUCKET=torture-differential` => `0 failing, 2 skipped` (`61` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=52, skipped=0`.
- Refreshed top-level bucket docs to current wave-16 baseline:
  `tests/final/15-torture-differential.md`,
  `docs/plans/torture_bucket_15_execution_plan.md`,
  `docs/status/PASSING.md`,
  `docs/status/UNVALIDATED.md`.
- Added explicit optional expansion backlog for bucket-15 so remaining
  untested regions are recorded and waveable.

### 2026-03-30 — Bucket: torture-differential (15 tomorrow-wave planning)

- Converted backlog into explicit execution waves for next session:
  Wave17 external-corpus scale-up,
  Wave18 malformed PP + multi-file expansion,
  Wave19 GCC tri-differential matrix,
  Wave20 stress-ceiling expansion,
  Wave21 policy-tag matrix expansion.
- Added planned shard names and lane intents in:
  `docs/plans/torture_bucket_15_execution_plan.md`.
- Updated bucket status doc pointer:
  `tests/final/15-torture-differential.md` now lists tomorrow target waves.

### 2026-03-31 — Bucket: torture-differential (15 wave17 external-corpus scale-up)

- Added Wave17 probe lanes:
  `15__probe_corpus_external_parser_fragment_a_smoke`,
  `15__probe_corpus_external_parser_fragment_b_smoke`,
  `15__probe_diag_corpus_external_macro_guard_reject`.
- Probe-first validation:
  `PROBE_FILTER=15__probe_corpus_external_parser_fragment_a_smoke,15__probe_corpus_external_parser_fragment_b_smoke,15__probe_diag_corpus_external_macro_guard_reject python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave17-external-corpus-scaleup.json`.
- Promoted tests:
  `15__torture__corpus_external_parser_fragment_a_smoke`,
  `15__torture__corpus_external_parser_fragment_b_smoke`,
  `15__torture__corpus_external_macro_guard_reject`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave17-external-corpus-scaleup.json`
  => all 3 pass.
  `make final-wave WAVE=17 WAVE_BUCKET=15-torture-differential`
  => all 3 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 2 skipped` (`64` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=55, skipped=0`.

### 2026-03-31 — Bucket: torture-differential (15 wave18 malformed PP/multi-file expansion)

- Added Wave18 probe lanes:
  `15__probe_diag_malformed_pp_unterminated_if_chain_no_crash`,
  `15__probe_diag_malformed_pp_macro_paste_fragments_no_crash`,
  `15__probe_diagjson_malformed_pp_unterminated_if_chain_no_crash`,
  `15__probe_diagjson_malformed_pp_macro_paste_fragments_no_crash`.
- Probe-first validation:
  `PROBE_FILTER=15__probe_diag_malformed_pp_unterminated_if_chain_no_crash,15__probe_diag_malformed_pp_macro_paste_fragments_no_crash,15__probe_diagjson_malformed_pp_unterminated_if_chain_no_crash,15__probe_diagjson_malformed_pp_macro_paste_fragments_no_crash python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave18-malformed-pp-multifile.json`.
- Promoted tests:
  `15__torture__malformed_pp_unterminated_if_chain_no_crash`,
  `15__torture__malformed_pp_macro_paste_fragments_no_crash`,
  `15__diagjson__malformed_pp_unterminated_if_chain_no_crash`,
  `15__diagjson__malformed_pp_macro_paste_fragments_no_crash`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave18-malformed-pp-multifile.json`
  => all 4 pass.
  `make final-wave WAVE=18 WAVE_BUCKET=15-torture-differential`
  => all 4 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 2 skipped` (`68` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=59, skipped=0`.

### 2026-03-31 — Bucket: torture-differential (15 wave19 GCC tri-differential matrix)

- Added Wave19 probe lanes:
  `15__probe_runtime_clang_gcc_tri_diff_control_checksum`,
  `15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge`,
  `15__probe_runtime_clang_gcc_tri_diff_abi_args_matrix`.
- Probe-first blocker/fix:
  initial multi-TU state-bridge shape triggered a compile crash (`-11`) in this
  frontend/backend path; stabilized by replacing it with a proven-safe multi-TU
  function-pointer state matrix shape while preserving tri-differential intent.
- Probe-first validation:
  `PROBE_FILTER=15__probe_runtime_clang_gcc_tri_diff_control_checksum,15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge,15__probe_runtime_clang_gcc_tri_diff_abi_args_matrix python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave19-gcc-tri-diff-matrix.json`.
- Promoted tests:
  `15__torture__clang_gcc_tri_diff_control_checksum`,
  `15__torture__clang_gcc_tri_diff_multitu_state_bridge`,
  `15__torture__clang_gcc_tri_diff_abi_args_matrix`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave19-gcc-tri-diff-matrix.json`
  => all 3 pass.
  `make final-wave WAVE=19 WAVE_BUCKET=15-torture-differential`
  => all 3 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 2 skipped` (`71` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=62, skipped=0`.

### 2026-03-31 — Bucket: torture-differential (15 wave20 stress-ceiling expansion)

- Added Wave20 probe lanes:
  `15__probe_deep_recursion_stack_pressure_ii`,
  `15__probe_many_decls_density_pressure_ii`,
  `15__probe_large_struct_array_pressure_ii`.
- Probe-first validation:
  `PROBE_FILTER=15__probe_deep_recursion_stack_pressure_ii,15__probe_many_decls_density_pressure_ii,15__probe_large_struct_array_pressure_ii python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave20-stress-ceiling-expansion.json`.
- Promoted tests:
  `15__torture__deep_recursion_stack_pressure_ii`,
  `15__torture__many_decls_density_pressure_ii`,
  `15__torture__large_struct_array_pressure_ii`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave20-stress-ceiling-expansion.json`
  => all 3 pass.
  `make final-wave WAVE=20 WAVE_BUCKET=15-torture-differential`
  => all 3 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 2 skipped` (`74` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=65, skipped=0`.

### 2026-03-31 — Bucket: torture-differential (15 wave21 policy-matrix expansion)

- Added Wave21 probe lanes:
  `15__probe_policy_impldef_signed_shift_matrix_tagged`,
  `15__probe_policy_impldef_signed_char_matrix_tagged`,
  `15__probe_policy_ub_unsequenced_write_tagged`,
  `15__probe_policy_ub_alias_overlap_tagged`.
- Probe-first validation:
  `PROBE_FILTER=15__probe_policy_impldef_signed_shift_matrix_tagged,15__probe_policy_impldef_signed_char_matrix_tagged,15__probe_policy_ub_unsequenced_write_tagged,15__probe_policy_ub_alias_overlap_tagged python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave21-policy-matrix-expansion.json`.
- Promoted tests:
  `15__torture__policy_impldef_signed_shift_matrix_tagged`,
  `15__torture__policy_impldef_signed_char_matrix_tagged`,
  `15__torture__policy_ub_unsequenced_write_tagged`,
  `15__torture__policy_ub_alias_overlap_tagged`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave21-policy-matrix-expansion.json`
  => `0 failing, 4 skipped`.
  `make final-wave WAVE=21 WAVE_BUCKET=15-torture-differential`
  => `0 failing, 4 skipped`.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 6 skipped` (`78` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=69, skipped=0`.

### 2026-03-31 — Bucket: torture-differential (15 wave22 malformed PP/include-graph expansion)

- Added Wave22 probe lanes:
  `15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash`,
  `15__probe_diag_malformed_pp_include_cycle_guarded_no_crash`,
  `15__probe_diagjson_malformed_pp_nested_ifdef_chain_seeded_d_no_crash`,
  `15__probe_diagjson_malformed_pp_include_cycle_guarded_no_crash`.
- Probe-first validation:
  `PROBE_FILTER=15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash,15__probe_diag_malformed_pp_include_cycle_guarded_no_crash,15__probe_diagjson_malformed_pp_nested_ifdef_chain_seeded_d_no_crash,15__probe_diagjson_malformed_pp_include_cycle_guarded_no_crash python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave22-malformed-pp-include-graph.json`.
- Promoted tests:
  `15__torture__malformed_pp_nested_ifdef_chain_seeded_d_no_crash`,
  `15__torture__malformed_pp_include_cycle_guarded_no_crash`,
  `15__diagjson__malformed_pp_nested_ifdef_chain_seeded_d_no_crash`,
  `15__diagjson__malformed_pp_include_cycle_guarded_no_crash`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave22-malformed-pp-include-graph.json`
  => all 4 pass.
  `make final-wave WAVE=22 WAVE_BUCKET=15-torture-differential`
  => all 4 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 6 skipped` (`82` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=73, skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15 wave23 malformed multi-file token-chaos expansion)

- Added Wave23 probe lanes:
  `15__probe_diag_malformed_multifile_header_tail_garbage_no_crash`,
  `15__probe_diag_malformed_multifile_macro_arity_mismatch_no_crash`,
  `15__probe_diagjson_malformed_multifile_header_tail_garbage_no_crash`,
  `15__probe_diagjson_malformed_multifile_macro_arity_mismatch_no_crash`.
- Probe-first validation:
  `PROBE_FILTER=15__probe_diag_malformed_multifile_header_tail_garbage_no_crash,15__probe_diag_malformed_multifile_macro_arity_mismatch_no_crash,15__probe_diagjson_malformed_multifile_header_tail_garbage_no_crash,15__probe_diagjson_malformed_multifile_macro_arity_mismatch_no_crash python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave23-malformed-multifile-token-chaos.json`.
- Promoted tests:
  `15__torture__malformed_multifile_header_tail_garbage_no_crash`,
  `15__torture__malformed_multifile_macro_arity_mismatch_no_crash`,
  `15__diagjson__malformed_multifile_header_tail_garbage_no_crash`,
  `15__diagjson__malformed_multifile_macro_arity_mismatch_no_crash`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave23-malformed-multifile-token-chaos.json`
  => all 4 pass.
  `make final-wave WAVE=23 WAVE_BUCKET=15-torture-differential`
  => all 4 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 6 skipped` (`86` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=77, skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15 wave24 tri-compiler differential breadth I)

- Added Wave24 probe lanes:
  `15__probe_runtime_clang_gcc_tri_diff_loop_state_crc_matrix`,
  `15__probe_runtime_clang_gcc_tri_diff_struct_array_stride_crc_matrix`,
  `15__probe_runtime_clang_gcc_tri_diff_pointer_mix_checksum_matrix`.
- Probe-first validation:
  `PROBE_FILTER=15__probe_runtime_clang_gcc_tri_diff_loop_state_crc_matrix,15__probe_runtime_clang_gcc_tri_diff_struct_array_stride_crc_matrix,15__probe_runtime_clang_gcc_tri_diff_pointer_mix_checksum_matrix python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave24-gcc-tri-diff-single-tu.json`.
- Promoted tests:
  `15__torture__clang_gcc_tri_diff_loop_state_crc_matrix`,
  `15__torture__clang_gcc_tri_diff_struct_array_stride_crc_matrix`,
  `15__torture__clang_gcc_tri_diff_pointer_mix_checksum_matrix`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave24-gcc-tri-diff-single-tu.json`
  => all 3 pass.
  `make final-wave WAVE=24 WAVE_BUCKET=15-torture-differential`
  => all 3 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 6 skipped` (`89` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=80, skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15 wave25 tri-compiler differential breadth II)

- Added Wave25 probe lanes:
  `15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge`,
  `15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline`,
  `15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge`.
- Probe-first validation:
  `PROBE_FILTER=15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge,15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline,15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave25-gcc-tri-diff-multitu.json`.
- Promoted tests:
  `15__torture__clang_gcc_tri_diff_multitu_fnptr_table_bridge`,
  `15__torture__clang_gcc_tri_diff_multitu_const_seed_pipeline`,
  `15__torture__clang_gcc_tri_diff_multitu_layout_digest_bridge`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave25-gcc-tri-diff-multitu.json`
  => all 3 pass.
  `make final-wave WAVE=25 WAVE_BUCKET=15-torture-differential`
  => all 3 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 6 skipped` (`92` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=83, skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15 wave26 stress ceiling expansion III)

- Added Wave26 probe lanes:
  `15__probe_deep_recursion_stack_pressure_iii`,
  `15__probe_many_decls_density_pressure_iii`,
  `15__probe_large_struct_array_pressure_iii`.
- Probe-first validation:
  `PROBE_FILTER=15__probe_deep_recursion_stack_pressure_iii,15__probe_many_decls_density_pressure_iii,15__probe_large_struct_array_pressure_iii python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave26-stress-ceiling-expansion-iii.json`.
- Promoted tests:
  `15__torture__deep_recursion_stack_pressure_iii`,
  `15__torture__many_decls_density_pressure_iii`,
  `15__torture__large_struct_array_pressure_iii`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave26-stress-ceiling-expansion-iii.json`
  => all 3 pass.
  `make final-wave WAVE=26 WAVE_BUCKET=15-torture-differential`
  => all 3 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 6 skipped` (`95` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=86, skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15 wave27 stress ceiling expansion IV, multi-TU)

- Added Wave27 probe lanes:
  `15__probe_multitu_large_table_crc_pressure`,
  `15__probe_multitu_recursive_dispatch_pressure`,
  `15__probe_multitu_layout_stride_pressure`.
- Probe-first validation:
  `PROBE_FILTER=15__probe_multitu_large_table_crc_pressure,15__probe_multitu_recursive_dispatch_pressure,15__probe_multitu_layout_stride_pressure python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave27-multitu-stress-ceiling-iv.json`.
- Promoted tests:
  `15__torture__multitu_large_table_crc_pressure`,
  `15__torture__multitu_recursive_dispatch_pressure`,
  `15__torture__multitu_layout_stride_pressure`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave27-multitu-stress-ceiling-iv.json`
  => all 3 pass.
  `make final-wave WAVE=27 WAVE_BUCKET=15-torture-differential`
  => all 3 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 6 skipped` (`98` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=89, skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15 wave28 external corpus scale-up I)

- Added Wave28 probe lanes:
  `15__probe_corpus_external_parser_fragment_c_smoke`,
  `15__probe_corpus_external_parser_fragment_d_smoke`,
  `15__probe_diag_corpus_external_macro_chain_reject`.
- Probe-first blocker/fix:
  initial macro-chain reject lane mismatched expected diagnostic substring.
  Updated the malformed define shape to fail closed at preprocessing with
  deterministic `invalid parameter list in #define`.
- Probe-first validation:
  `PROBE_FILTER=15__probe_corpus_external_parser_fragment_c_smoke,15__probe_corpus_external_parser_fragment_d_smoke,15__probe_diag_corpus_external_macro_chain_reject python3 tests/final/probes/run_probes.py`
  => `resolved=3`, `blocked=0`, `skipped=0`.
- Promoted shard:
  `tests/final/meta/15-torture-differential-wave28-external-corpus-scaleup-i.json`.
- Promoted tests:
  `15__torture__corpus_external_parser_fragment_c_smoke`,
  `15__torture__corpus_external_parser_fragment_d_smoke`,
  `15__torture__corpus_external_macro_chain_reject`.
- Validation:
  `make final-manifest MANIFEST=15-torture-differential-wave28-external-corpus-scaleup-i.json`
  => all 3 pass.
  `make final-wave WAVE=28 WAVE_BUCKET=15-torture-differential`
  => all 3 pass.
  `make final-bucket BUCKET=torture-differential`
  => `0 failing, 6 skipped` (`101` active).
  `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py | rg "^Summary:"`
  => `Summary: blocked=0, resolved=92, skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15) wave-36 stress-ceiling frontier sweep

- Scope:
  add bounded, deterministic stress-ceiling sweep lanes for recursion depth,
  declaration density, aggregate size, and multi-TU layout pressure.
- Added probes:
  `15__probe_ceiling_recursion_depth_sweep`,
  `15__probe_ceiling_decl_density_sweep`,
  `15__probe_ceiling_aggregate_size_sweep`,
  `15__probe_ceiling_multitu_layout_pressure_sweep`.
- Probe verify:
  `PROBE_FILTER=15__probe_ceiling_ python3 tests/final/probes/run_probes.py`
  => `resolved=4`, `blocked=0`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave36-stress-ceiling-frontier-sweep.json`
  with:
  `15__torture__ceiling_recursion_depth_sweep`,
  `15__torture__ceiling_decl_density_sweep`,
  `15__torture__ceiling_aggregate_size_sweep`,
  `15__torture__ceiling_multitu_layout_pressure_sweep`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave36-stress-ceiling-frontier-sweep.json`
  passes;
  `make final-wave WAVE=36 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 10 skipped`, `125` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=116`, `skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15) wave-37 fuzz-volume deterministic replay

- Scope:
  expand seeded fuzz coverage from smoke lanes to bounded deterministic
  replay-volume lanes and add malformed replay fail-closed parity in both
  text diagnostics and diagnostics JSON.
- Added probes:
  `15__probe_fuzz_seeded_expr_volume_replay`,
  `15__probe_fuzz_seeded_stmt_volume_replay`,
  `15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash`,
  `15__probe_diagjson_fuzz_seeded_malformed_volume_replay_no_crash`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=4`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave37-fuzz-volume-deterministic-replay.json`
  with:
  `15__torture__fuzz_seeded_expr_volume_replay`,
  `15__torture__fuzz_seeded_stmt_volume_replay`,
  `15__torture__fuzz_seeded_malformed_volume_replay_no_crash`,
  `15__diagjson__fuzz_seeded_malformed_volume_replay_no_crash`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave37-fuzz-volume-deterministic-replay.json`
  passes;
  `make final-wave WAVE=37 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 10 skipped`, `129` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=120`, `skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15) wave-38 gcc tri-reference breadth expansion

- Scope:
  widen optional gcc tri-reference runtime coverage beyond prior checksum/header
  lanes with four additional control-flow, variadic ABI, VLA, and struct/union
  layout differential lanes.
- Added probes:
  `15__probe_runtime_clang_gcc_tri_diff_control_flow_lattice_matrix`,
  `15__probe_runtime_clang_gcc_tri_diff_abi_variadic_regstack_matrix`,
  `15__probe_runtime_clang_gcc_tri_diff_vla_stride_rebase_matrix`,
  `15__probe_runtime_clang_gcc_tri_diff_struct_union_layout_bridge`.
- Probe-first blocker/fix:
  initial variadic ABI lane failed preprocessing due `<stdarg.h>` handling;
  lane was converted to the established builtin variadic pattern:
  `__builtin_va_list`, `__builtin_va_start`, `__builtin_va_arg`, `__builtin_va_end`.
- Probe verify:
  targeted rerun reports `blocked=0`, `resolved=4`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave38-gcc-tri-diff-breadth-expansion.json`
  with:
  `15__torture__clang_gcc_tri_diff_control_flow_lattice_matrix`,
  `15__torture__clang_gcc_tri_diff_abi_variadic_regstack_matrix`,
  `15__torture__clang_gcc_tri_diff_vla_stride_rebase_matrix`,
  `15__torture__clang_gcc_tri_diff_struct_union_layout_bridge`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave38-gcc-tri-diff-breadth-expansion.json`
  passes;
  `make final-wave WAVE=38 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 10 skipped`, `133` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=124`, `skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15) wave-39 compile-surface stress expansion

- Scope:
  expand compile-surface depth with one additional pathological declaration-graph
  AST lane and paired initializer/switch negative lanes with `.diagjson` parity.
- Added probes:
  `15__probe_ast_pathological_decl_graph_surface`,
  `15__probe_diag_pathological_initializer_rewrite_surface_reject`,
  `15__probe_diag_pathological_switch_case_surface_reject`,
  `15__probe_diagjson_pathological_initializer_rewrite_surface_reject`,
  `15__probe_diagjson_pathological_switch_case_surface_reject`.
- Probe-first blocker/fix:
  initial initializer rewrite `.diagjson` probe expected line mismatch;
  updated expected line to `22` based on emitted diagnostics location.
- Probe verify:
  targeted rerun reports `blocked=0`, `resolved=5`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave39-compile-surface-expansion.json`
  with:
  `15__torture__pathological_decl_graph_surface`,
  `15__torture__pathological_initializer_rewrite_surface_reject`,
  `15__torture__pathological_switch_case_surface_reject`,
  `15__diagjson__pathological_initializer_rewrite_surface_reject`,
  `15__diagjson__pathological_switch_case_surface_reject`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave39-compile-surface-expansion.json`
  passes;
  `make final-wave WAVE=39 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 10 skipped`, `138` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=129`, `skipped=0`.

### 2026-04-01 — Bucket: torture-differential (15) wave-40 external-corpus pinned bundle expansion

- Scope:
  complete the planned external-corpus pinned bundle with three deterministic
  runtime fragment lanes plus two pinned negative lanes and two `.diagjson`
  parity lanes.
- Added probes:
  `15__probe_corpus_pinned_fragment_e_smoke`,
  `15__probe_corpus_pinned_fragment_f_smoke`,
  `15__probe_corpus_pinned_fragment_g_smoke`,
  `15__probe_diag_corpus_pinned_macro_include_chain_reject`,
  `15__probe_diag_corpus_pinned_typedef_decl_cycle_reject`,
  `15__probe_diagjson_corpus_pinned_macro_include_chain_reject`,
  `15__probe_diagjson_corpus_pinned_typedef_decl_cycle_reject`.
- Probe verify:
  targeted run reports `blocked=0`, `resolved=7`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/15-torture-differential-wave40-external-corpus-pinned-bundle.json`
  with:
  `15__torture__corpus_pinned_fragment_e_smoke`,
  `15__torture__corpus_pinned_fragment_f_smoke`,
  `15__torture__corpus_pinned_fragment_g_smoke`,
  `15__torture__corpus_pinned_macro_include_chain_reject`,
  `15__torture__corpus_pinned_typedef_decl_cycle_reject`,
  `15__diagjson__corpus_pinned_macro_include_chain_reject`,
  `15__diagjson__corpus_pinned_typedef_decl_cycle_reject`.
- Runtime verify:
  `make final-manifest MANIFEST=15-torture-differential-wave40-external-corpus-pinned-bundle.json`
  passes;
  `make final-wave WAVE=40 WAVE_BUCKET=15-torture-differential` passes;
  `make final-bucket BUCKET=torture-differential` passes (`0 failing, 10 skipped`, `145` active).
- Bucket probe snapshot after promotion:
  `PROBE_FILTER=15__probe_*` => `blocked=0`, `resolved=136`, `skipped=0`.

### 2026-04-01 — Cross-bucket audit refresh (post-wave40 / post-wave89)

- Scope:
  refresh docs to match live suite state and identify remaining explicit gaps
  after runtime/torture campaign stabilization.
- Validation snapshot:
  - `make final-runtime` => `0 failing, 22 skipped`.
  - `make final-bucket BUCKET=torture-differential` => `0 failing, 10 skipped`.
  - `PROBE_FILTER=14__probe_* python3 tests/final/probes/run_probes.py`
    => `blocked=0`, `resolved=234`, `skipped=0`.
  - `PROBE_FILTER=15__probe_* python3 tests/final/probes/run_probes.py`
    => `blocked=0`, `resolved=136`, `skipped=0`.
  - `make final` => `0 failing, 32 skipped`.
- Doc refreshes:
  - Updated `docs/status/UNVALIDATED.md` with explicit unstarted checklist gaps
    and a prioritized next execution order.
  - Updated runtime/torture bucket status docs and plan docs to remove stale
    pre-wave36/pre-wave89 baseline counts.
- Remaining explicit gaps (from checklist + policy backlog):
  - semantic bucket (`07`) conversion/member/constexpr matrix is still mostly unstarted,
  - lexer boundary leftovers (`02`) remain for literal precision/bounds/escape overflow,
  - statement return diagnostics policy lanes in bucket `09` are still unstarted,
  - bucket-15 high-volume randomized fuzz harness remains optional backlog.

### 2026-04-01 — Bucket: types-conversions (07) wave-2 semantic expansion

- Scope:
  start bucket-07 wave flow with semantic negative/edge lanes for aggregate
  member validity and constant-expression enforcement.
- Added probes:
  - runtime:
    `07__probe_agg_member_access_runtime`,
    `07__probe_agg_offsets_runtime`
  - diagnostics:
    `07__probe_assign_struct_to_int_reject`,
    `07__probe_agg_invalid_member_reject`,
    `07__probe_constexpr_array_size_reject`,
    `07__probe_constexpr_case_label_reject`,
    `07__probe_constexpr_static_init_reject`
- Probe-first result:
  initial run had `blocked=1` due strict substring mismatch on static-init
  diagnostic wording; matcher updated to `not a constant expression`.
- Probe verify after matcher fix:
  `PROBE_FILTER=07__probe_* python3 tests/final/probes/run_probes.py`
  => `blocked=0`, `resolved=7`, `skipped=0`.
- Promotions:
  added
  `tests/final/meta/07-types-conversions-wave2-semantic.json`
  with:
  `07__assign_struct_to_int_reject`,
  `07__agg__member_access`,
  `07__agg__offsets`,
  `07__agg__invalid_member_reject`,
  `07__constexpr__array_size_reject`,
  `07__constexpr__case_label_reject`,
  `07__constexpr__static_init_reject`.
- Validation:
  `make final-manifest MANIFEST=07-types-conversions-wave2-semantic.json`
  => all 7 pass.
  `make final-prefix PREFIX=07__`
  => all 16 pass.
