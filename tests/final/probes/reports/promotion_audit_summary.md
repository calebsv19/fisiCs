# Probe Promotion Audit

## Summary

- Total resolved probe inventory audited: `3598`
- Promoted stable coverage: `3577`
- Intentional probe-only coverage: `21`
- Missing promotion candidates: `0`
- Stable final tests scanned: `4988`
- Critical integrity errors: `0`
- Ambiguous stem matches: `0`
- Multi-owner best-rank matches: `568`
- Duplicate stable semantic shapes: `12`
- Probe assets outside both lanes: `21`

## Integrity

- Duplicate probe IDs: `0`
- Duplicate final IDs: `0`
- Missing probe inputs: `0`
- Missing final inputs: `0`
- Missing explicit promotion owners: `0`
- Promoted non-ok matches: `0`
- Ambiguous stem ownership requiring review: `0`
- Multi-owner best-rank matches requiring explicit-owner migration: `568`
- Duplicate stable semantic shapes: `12`
- Probe assets `stable-only`: `13`
- Probe assets `inventory-only`: `621`
- Probe assets `unowned`: `21`

### Promoted Match Evidence

- `explicit-id`: `495`
- `id`: `1507`
- `path`: `1573`
- `stem`: `2`

### Explicit Probe-Only Reasons

- `2`: explicit probe-only disposition
- `6`: explicit probe-only family marker: corpus
- `3`: explicit probe-only family marker: pathological
- `4`: explicit probe-only family marker: policy
- `5`: explicit probe-only family marker: seeded
- `1`: explicit probe-only note prefix: axis3 wave19 reduced

### Bucket Breakdown

- Bucket `01`: promoted `64`, probe-only `0`, missing `0`
- Bucket `02`: promoted `94`, probe-only `0`, missing `0`
- Bucket `03`: promoted `100`, probe-only `0`, missing `0`
- Bucket `04`: promoted `191`, probe-only `0`, missing `0`
- Bucket `05`: promoted `141`, probe-only `0`, missing `0`
- Bucket `06`: promoted `120`, probe-only `0`, missing `0`
- Bucket `07`: promoted `302`, probe-only `0`, missing `0`
- Bucket `08`: promoted `243`, probe-only `0`, missing `0`
- Bucket `09`: promoted `301`, probe-only `0`, missing `0`
- Bucket `10`: promoted `133`, probe-only `0`, missing `0`
- Bucket `11`: promoted `242`, probe-only `0`, missing `0`
- Bucket `12`: promoted `247`, probe-only `0`, missing `0`
- Bucket `13`: promoted `184`, probe-only `0`, missing `0`
- Bucket `14`: promoted `587`, probe-only `1`, missing `0`
- Bucket `15`: promoted `628`, probe-only `20`, missing `0`

## Missing Promotion Candidates

- None.

## Intentional Probe-Only Coverage


### explicit probe-only note prefix: axis3 wave19 reduced

- `14__probe_axis3_wave19_multitu_abi_hfa_struct_return_variadic_shadow_replay_fold_edge_reduced` (`runtime`) from `probes/runtime/14__probe_axis3_wave19_multitu_abi_hfa_struct_return_variadic_shadow_replay_fold_edge_reduced_main.c` - axis3 wave19 reduced: smaller shadow/replay HFA variadic fold boundary remains clang-parity stable as the strict lane is now fixed and promoted

### explicit probe-only family marker: corpus

- `15__probe_diag_corpus_external_compile_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_external_compile_reject.c` - external-corpus fragment with malformed typedef tail should fail closed with parser diagnostic
- `15__probe_diag_corpus_external_include_guard_mismatch_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_external_include_guard_mismatch_reject.c` - external-corpus include-guard mismatch should fail closed via recursive-include detection
- `15__probe_diag_corpus_external_macro_chain_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_external_macro_chain_reject.c` - external-corpus malformed macro-chain fragment should fail closed in preprocessing
- `15__probe_diag_corpus_external_macro_guard_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_external_macro_guard_reject.c` - external-corpus malformed macro-guard fragment should fail closed in preprocessing
- `15__probe_diag_corpus_pinned_macro_include_chain_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_pinned_macro_include_chain_reject.c` - pinned corpus macro include-chain fragment should fail closed in preprocessing
- `15__probe_diag_corpus_pinned_typedef_decl_cycle_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_pinned_typedef_decl_cycle_reject.c` - pinned corpus typedef-decl cycle fragment should fail closed with parser diagnostic

### explicit probe-only family marker: seeded

- `15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash` (`diagnostic`) from `probes/diagnostics/15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash.c` - seeded malformed fuzz-volume replay lane should fail closed without crashing
- `15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash` (`diagnostic`) from `probes/diagnostics/15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash.c` - malformed nested #if/#elif chain should fail closed without crashing
- `15__probe_diag_malformed_token_stream_seeded_a_no_crash` (`diagnostic`) from `probes/diagnostics/15__probe_diag_malformed_token_stream_seeded_a_no_crash.c` - seeded malformed token-stream A should fail closed without crashing
- `15__probe_diag_malformed_token_stream_seeded_b_no_crash` (`diagnostic`) from `probes/diagnostics/15__probe_diag_malformed_token_stream_seeded_b_no_crash.c` - seeded malformed token-stream B should fail closed without crashing
- `15__probe_diag_malformed_token_stream_seeded_c_no_crash` (`diagnostic`) from `probes/diagnostics/15__probe_diag_malformed_token_stream_seeded_c_no_crash.c` - seeded malformed token-stream C should fail closed without crashing

### explicit probe-only family marker: pathological

- `15__probe_diag_pathological_initializer_rewrite_surface_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_pathological_initializer_rewrite_surface_reject.c` - pathological initializer rewrite surface lane should fail closed with deterministic diagnostics
- `15__probe_diag_pathological_initializer_shape_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_pathological_initializer_shape_reject.c` - pathological designated-initializer shape should fail closed with deterministic diagnostics
- `15__probe_diag_pathological_switch_case_surface_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_pathological_switch_case_surface_reject.c` - pathological switch case surface lane should reject non-constant case labels

### explicit probe-only family marker: policy

- `15__probe_os_post_edu19_edu23_parallelism_matrix` (`runtime`) from `probes/runtime/15__probe_os_post_edu19_edu23_parallelism_matrix.c` - Immutable EDU-23 generated-C policy mirror: bounded worker admission, grants, deterministic partition values, path evidence, terminal state, and fail-closed identity conflicts
- `15__probe_os_post_edu19_edu34_deadline_entry_matrix` (`runtime`) from `probes/runtime/15__probe_os_post_edu19_edu34_deadline_entry_matrix.c` - OS post-EDU-19 policy intake from immutable EDU-34: exact durable queue-entry validation for state-dependent deadline placement, timeout/budget/cancellation terminal evidence, preserved phase prefixes, bounds, unaligned input, and recomputed-checksum contradictions
- `15__probe_os_post_edu19_edu36_resume_entry_matrix` (`runtime`) from `probes/runtime/15__probe_os_post_edu19_edu36_resume_entry_matrix.c` - OS post-EDU-19 policy intake from immutable EDU-36: exact Entry-v8 and Trace-v3 checkpoint-resume validation for resumed running/complete/interrupted/timeout/cancel states, restored phase-three evidence, ordinary-path separation, unaligned input, fail-closed contradictions, and the frozen validator's current compact-trace flag admission
- `15__probe_os_post_edu19_result_v1_policy_matrix` (`runtime`) from `probes/runtime/15__probe_os_post_edu19_result_v1_matrix.c` - OS post-EDU-19 policy intake from immutable EDU-33: exact Result-v1 sector structure, identity bounds, nested and whole checksums, zero padding, unaligned input, opaque result bits, and fail-closed identity contradictions

### explicit probe-only disposition

- `15__probe_os_post_edu19_temporal_fault_sequence_matrix` (`runtime`) from `probes/runtime/15__probe_os_post_edu19_temporal_fault_sequence_matrix.c` - Frontier lane: hardware-blind temporal composition over the frozen EDU-26/35/37/39/40/41 contracts and durable-owner chain: pre-ACK interruption blocks generation reuse, post-checkpoint restart distinguishes an overwrite destination from restored record identity, mid-phase owner loss preserves the peer, and post-completion retirement blocks stale redispatch. This is compiler-probe evidence, not an EDU-44 OS implementation claim
- `15__probe_os_post_edu19_temporal_pair_fault_matrix` (`runtime`) from `probes/runtime/15__probe_os_post_edu19_temporal_pair_fault_matrix.c` - Frontier lane: paired temporal contradictions over the first EDU-44 compiler model: checkpoint interruption plus generation reuse, completion/retirement reordering, owner loss plus peer corruption, stale ACK/mailbox/snapshot evidence crossing generations, and exact rejection of a checksum-valid recovery record with stale embedded identity. This is compiler-probe evidence, not an EDU-45 OS implementation claim

## Promoted Coverage Sample

- `01__probe_line_directive_include_nested_macro_undeclared_identifier_location_strict` via `id` -> 01__diag__line_directive_include_nested_macro_undeclared_identifier_location_strict
- `01__probe_line_directive_include_tokenpaste_undeclared_identifier_location_strict` via `id` -> 01__diag__line_directive_include_tokenpaste_undeclared_identifier_location_strict
- `01__probe_line_directive_macro_nonvoid_return_current_zerozero` via `id` -> 01__diag__line_directive_macro_nonvoid_return_current_zerozero
- `01__probe_line_directive_macro_nonvoid_return_location_reject` via `id` -> 01__diag__line_directive_macro_nonvoid_return_location_reject
- `01__probe_line_directive_virtual_line_nonvoid_return_current_zerozero` via `id` -> 01__diag__line_directive_virtual_line_nonvoid_return_current_zerozero
- `01__probe_line_directive_virtual_line_nonvoid_return_location_reject` via `id` -> 01__diag__line_directive_virtual_line_nonvoid_return_location_reject
- `01__probe_line_directive_virtual_line_spelling_reject` via `id` -> 01__diag__line_directive_virtual_line_spelling_reject
- `01__probe_line_directive_virtual_line_undeclared_identifier_location_reject` via `id` -> 01__diag__line_directive_virtual_line_undeclared_identifier_location_reject
- `01__probe_line_directive_virtual_macro_filename_spelling_reject` via `id` -> 01__diag__line_directive_virtual_macro_filename_spelling_reject
- `01__probe_line_mapping_real_file_baseline` via `id` -> 01__diag__line_mapping_real_file_baseline
- `01__probe_nonvoid_return_plain_current_zerozero` via `id` -> 01__diag__nonvoid_return_plain_current_zerozero
- `01__probe_diagjson_line_directive_include_nested_macro_undeclared_identifier_location_strict` via `id` -> 01__diagjson__line_directive_include_nested_macro_undeclared_identifier_location_strict
- `01__probe_diagjson_line_directive_include_tokenpaste_undeclared_identifier_location_strict` via `id` -> 01__diagjson__line_directive_include_tokenpaste_undeclared_identifier_location_strict
- `01__probe_diagjson_line_directive_macro_line_map_strict` via `id` -> 01__diagjson__line_directive_macro_line_map_strict
- `01__probe_diagjson_line_directive_macro_nonvoid_return_current_zerozero` via `id` -> 01__diagjson__line_directive_macro_nonvoid_return_current_zerozero
- `01__probe_diagjson_line_directive_macro_nonvoid_return_location_reject` via `id` -> 01__diagjson__line_directive_macro_nonvoid_return_location_reject
- `01__probe_diagjson_line_directive_nonvoid_return_location_reject` via `id` -> 01__diagjson__line_directive_nonvoid_return_location_reject
- `01__probe_diagjson_line_directive_undeclared_identifier_location_strict` via `id` -> 01__diagjson__line_directive_undeclared_identifier_location_strict
- `01__probe_diagjson_nonvoid_return_plain_current_zerozero` via `id` -> 01__diagjson__nonvoid_return_plain_current_zerozero
- `01__probe_runtime_include_file_line_bridge` via `path` -> 01__runtime__line_directive_include_file_line_bridge
- `01__probe_runtime_include_nested_stringize_depth` via `path` -> 01__runtime__line_directive_include_nested_stringize_depth
- `01__probe_runtime_include_stringize_remap` via `path` -> 01__runtime__line_directive_include_stringize_remap
- `01__probe_runtime_include_tokenpaste_depth` via `path` -> 01__runtime__line_directive_include_tokenpaste_depth
- `01__probe_runtime_include_tokenpaste_stringize_bridge` via `path` -> 01__runtime__line_directive_include_tokenpaste_stringize_bridge
- `01__probe_runtime_nested_include_provenance_bridge` via `path` -> 01__runtime__line_directive_nested_include_provenance_bridge
- `01__probe_runtime_wave13_adjacent_string_file_remap` via `path` -> 01__runtime__wave13_adjacent_string_file_remap
- `01__probe_runtime_wave13_spliced_macro_include_provenance` via `path` -> 01__runtime__wave13_spliced_macro_include_provenance
- `01__probe_runtime_wave13_tokenpaste_line_stringize` via `path` -> 01__runtime__wave13_tokenpaste_line_stringize
- `01__probe_runtime_wave14_nested_include_adjacent_source` via `path` -> 01__runtime__wave14_nested_include_adjacent_source
- `01__probe_runtime_wave14_spliced_nested_macro_file_line` via `path` -> 01__runtime__wave14_spliced_nested_macro_file_line
- `01__probe_runtime_wave14_tokenpaste_stringize_adjacent` via `path` -> 01__runtime__wave14_tokenpaste_stringize_adjacent
- `01__probe_runtime_wave15_comment_splice_file_boundary` via `path` -> 01__runtime__wave15_comment_splice_file_boundary
- `01__probe_runtime_wave15_nested_include_line_boundary` via `path` -> 01__runtime__wave15_nested_include_line_boundary
- `01__probe_runtime_wave15_spliced_macro_arg_source` via `path` -> 01__runtime__wave15_spliced_macro_arg_source
- `01__probe_runtime_wave16_macro_line_filename` via `path` -> 01__runtime__wave16_macro_line_filename
- `01__probe_runtime_wave16_nested_include_resumption` via `path` -> 01__runtime__wave16_nested_include_resumption
- `01__probe_runtime_wave16_splice_stringize_tokenpaste` via `path` -> 01__runtime__wave16_splice_stringize_tokenpaste
- `01__probe_runtime_wave16_splice_stringize_tokenpaste_current` via `path` -> 01__runtime__wave16_splice_stringize_tokenpaste_current
- `01__probe_runtime_wave17_adjacent_string_include_line` via `path` -> 01__runtime__wave17_adjacent_string_include_line
- `01__probe_runtime_wave17_macro_file_paste_line` via `path` -> 01__runtime__wave17_macro_file_paste_line
