from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='15__probe_axis5_wave1_reducer_signature_preservation_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave1_reducer_signature_preservation_matrix.c',
        note='axis5 wave1: reducer signature-preservation matrix should keep canonical and encoded reduction signatures identical',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave1_reducer_minimization_idempotence_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave1_reducer_minimization_idempotence_matrix.c',
        note='axis5 wave1: reducer minimization-idempotence matrix should keep first-pass and second-pass reduced signatures identical',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave1_reducer_cross_tu_signature_stability_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave1_reducer_cross_tu_signature_stability_matrix_main.c',
        note='axis5 wave1: reducer cross-TU signature-stability matrix should keep deterministic nonzero signatures across full and partitioned folds',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis5_wave1_reducer_cross_tu_signature_stability_matrix_main.c', PROBE_DIR / 'runtime/15__probe_axis5_wave1_reducer_cross_tu_signature_stability_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave2_reducer_permutation_canonicalization_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave2_reducer_permutation_canonicalization_matrix.c',
        note='axis5 wave2: reducer permutation-canonicalization matrix should keep canonical signatures invariant across equivalent permutations',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave2_reducer_replay_window_stability_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave2_reducer_replay_window_stability_matrix.c',
        note='axis5 wave2: reducer replay-window stability matrix should preserve final reducer signature across chunked replay with state encode/decode',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave2_reducer_cross_tu_repartition_invariance_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave2_reducer_cross_tu_repartition_invariance_matrix_main.c',
        note='axis5 wave2: reducer cross-TU repartition invariance matrix should keep full-fold and partitioned-fold signatures identical',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis5_wave2_reducer_cross_tu_repartition_invariance_matrix_main.c', PROBE_DIR / 'runtime/15__probe_axis5_wave2_reducer_cross_tu_repartition_invariance_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave3_reducer_delta_roundtrip_invariance_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave3_reducer_delta_roundtrip_invariance_matrix.c',
        note='axis5 wave3: reducer delta roundtrip invariance matrix should preserve signature after encode/decode reconstruction',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave3_reducer_shard_fold_associativity_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave3_reducer_shard_fold_associativity_matrix.c',
        note='axis5 wave3: reducer shard-fold associativity matrix should preserve signatures across equivalent merge orderings',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave3_reducer_cross_tu_checkpoint_resume_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave3_reducer_cross_tu_checkpoint_resume_matrix_main.c',
        note='axis5 wave3: reducer cross-TU checkpoint/resume matrix should preserve signatures across checkpoint serialization boundaries',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis5_wave3_reducer_cross_tu_checkpoint_resume_matrix_main.c', PROBE_DIR / 'runtime/15__probe_axis5_wave3_reducer_cross_tu_checkpoint_resume_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave4_reducer_cancel_pair_confluence_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave4_reducer_cancel_pair_confluence_matrix.c',
        note='axis5 wave4: reducer cancel-pair confluence matrix should preserve normal-form signatures across opposing elimination directions',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave4_reducer_tombstone_compaction_invariance_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave4_reducer_tombstone_compaction_invariance_matrix.c',
        note='axis5 wave4: reducer tombstone-compaction invariance matrix should preserve live-set signatures before and after compaction',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave4_reducer_cross_tu_epoch_frontier_replay_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave4_reducer_cross_tu_epoch_frontier_replay_matrix_main.c',
        note='axis5 wave4: reducer cross-TU epoch-frontier replay matrix should preserve signatures across exported frontier replay boundaries',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis5_wave4_reducer_cross_tu_epoch_frontier_replay_matrix_main.c', PROBE_DIR / 'runtime/15__probe_axis5_wave4_reducer_cross_tu_epoch_frontier_replay_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave5_reducer_phase_order_convergence_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave5_reducer_phase_order_convergence_matrix.c',
        note='axis5 wave5: reducer phase-order convergence matrix should preserve signatures across equivalent normalization pipelines',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave5_reducer_neutral_element_elision_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave5_reducer_neutral_element_elision_matrix.c',
        note='axis5 wave5: reducer neutral-element elision matrix should preserve signatures when no-op and zero-payload elements are present',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave5_reducer_cross_tu_speculative_rollback_frontier_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave5_reducer_cross_tu_speculative_rollback_frontier_matrix_main.c',
        note='axis5 wave5: reducer cross-TU speculative-rollback frontier matrix should preserve signatures across rollback boundary export/import',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis5_wave5_reducer_cross_tu_speculative_rollback_frontier_matrix_main.c', PROBE_DIR / 'runtime/15__probe_axis5_wave5_reducer_cross_tu_speculative_rollback_frontier_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave6_reducer_window_rotation_hash_invariance_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave6_reducer_window_rotation_hash_invariance_matrix.c',
        note='axis5 wave6: reducer window-rotation hash invariance matrix should preserve canonical signatures under storage rotation with adjusted head offsets',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave6_reducer_eager_lazy_prune_equivalence_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave6_reducer_eager_lazy_prune_equivalence_matrix.c',
        note='axis5 wave6: reducer eager-vs-lazy prune equivalence matrix should preserve signatures across equivalent zero-elision schedules',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave6_reducer_cross_tu_shard_checkpoint_stitch_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave6_reducer_cross_tu_shard_checkpoint_stitch_matrix_main.c',
        note='axis5 wave6: reducer cross-TU shard-checkpoint stitch matrix should preserve signatures after per-shard encode/decode and merge',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis5_wave6_reducer_cross_tu_shard_checkpoint_stitch_matrix_main.c', PROBE_DIR / 'runtime/15__probe_axis5_wave6_reducer_cross_tu_shard_checkpoint_stitch_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave7_reducer_alpha_renaming_equivalence_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave7_reducer_alpha_renaming_equivalence_matrix.c',
        note='axis5 wave7: reducer alpha-renaming equivalence matrix should preserve multiset signatures under key relabeling',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave7_reducer_chunk_boundary_decode_invariance_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave7_reducer_chunk_boundary_decode_invariance_matrix.c',
        note='axis5 wave7: reducer chunk-boundary decode invariance matrix should preserve signatures across run decoding chunk schedules',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave7_reducer_cross_tu_frontier_join_commutativity_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave7_reducer_cross_tu_frontier_join_commutativity_matrix_main.c',
        note='axis5 wave7: reducer cross-TU frontier-join commutativity matrix should preserve signatures across merge orderings after frontier encode/decode',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis5_wave7_reducer_cross_tu_frontier_join_commutativity_matrix_main.c', PROBE_DIR / 'runtime/15__probe_axis5_wave7_reducer_cross_tu_frontier_join_commutativity_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave8_reducer_lane_projection_fold_equivalence_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave8_reducer_lane_projection_fold_equivalence_matrix.c',
        note='axis5 wave8: reducer lane-projection fold equivalence matrix should preserve signatures across direct and projected fold pipelines',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave8_reducer_snapshot_double_encode_idempotence_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave8_reducer_snapshot_double_encode_idempotence_matrix.c',
        note='axis5 wave8: reducer snapshot double-encode idempotence matrix should preserve signatures across repeated encode/decode passes',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave8_reducer_cross_tu_frontier_roundtrip_associativity_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave8_reducer_cross_tu_frontier_roundtrip_associativity_matrix_main.c',
        note='axis5 wave8: reducer cross-TU frontier roundtrip associativity matrix should preserve signatures across frontier roundtrip and merge ordering',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis5_wave8_reducer_cross_tu_frontier_roundtrip_associativity_matrix_main.c', PROBE_DIR / 'runtime/15__probe_axis5_wave8_reducer_cross_tu_frontier_roundtrip_associativity_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave9_reducer_sparse_dense_materialization_equivalence_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave9_reducer_sparse_dense_materialization_equivalence_matrix.c',
        note='axis5 wave9: reducer sparse-vs-dense materialization equivalence matrix should preserve signatures across equivalent accumulator layouts',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave9_reducer_chunked_prefix_rebase_invariance_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave9_reducer_chunked_prefix_rebase_invariance_matrix.c',
        note='axis5 wave9: reducer chunked-prefix rebase invariance matrix should preserve signatures across chunked and direct prefix decodes',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis5_wave9_reducer_cross_tu_checkpoint_chain_merge_invariance_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis5_wave9_reducer_cross_tu_checkpoint_chain_merge_invariance_matrix_main.c',
        note='axis5 wave9: reducer cross-TU checkpoint-chain merge invariance matrix should preserve signatures after chained checkpoint decode and merge',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis5_wave9_reducer_cross_tu_checkpoint_chain_merge_invariance_matrix_main.c', PROBE_DIR / 'runtime/15__probe_axis5_wave9_reducer_cross_tu_checkpoint_chain_merge_invariance_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_vla_stride_rebase_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_vla_stride_rebase_matrix.c',
        note='VLA stride/rebase matrix lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_struct_union_layout_bridge',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_struct_union_layout_bridge.c',
        note='struct/union layout bridge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_policy_signed_shift_impl_defined_tagged',
        source=PROBE_DIR / 'runtime/15__probe_policy_signed_shift_impl_defined_tagged.c',
        note='implementation-defined signed right-shift lane should remain deterministic on this host',
    ),
    RuntimeProbe(
        probe_id='15__probe_policy_alias_ub_tagged',
        source=PROBE_DIR / 'runtime/15__probe_policy_alias_ub_tagged.c',
        note='restrict-alias UB lane should remain tracked and deterministic in probe mode',
    ),
    RuntimeProbe(
        probe_id='15__probe_policy_impldef_signed_shift_matrix_tagged',
        source=PROBE_DIR / 'runtime/15__probe_policy_impldef_signed_shift_matrix_tagged.c',
        note='implementation-defined signed-right-shift matrix should remain deterministic on this host',
    ),
    RuntimeProbe(
        probe_id='15__probe_policy_impldef_signed_char_matrix_tagged',
        source=PROBE_DIR / 'runtime/15__probe_policy_impldef_signed_char_matrix_tagged.c',
        note='implementation-defined plain-char signedness matrix should remain deterministic on this host',
    ),
    RuntimeProbe(
        probe_id='15__probe_policy_ub_unsequenced_write_tagged',
        source=PROBE_DIR / 'runtime/15__probe_policy_ub_unsequenced_write_tagged.c',
        note='unsequenced-write UB lane should remain tracked and deterministic in probe mode',
    ),
    RuntimeProbe(
        probe_id='15__probe_policy_ub_alias_overlap_tagged',
        source=PROBE_DIR / 'runtime/15__probe_policy_ub_alias_overlap_tagged.c',
        note='overlap-alias UB lane should remain tracked and deterministic in probe mode',
    ),
    RuntimeProbe(
        probe_id='15__probe_policy_impldef_signed_shift_width_matrix_tagged',
        source=PROBE_DIR / 'runtime/15__probe_policy_impldef_signed_shift_width_matrix_tagged.c',
        note='implementation-defined signed-right-shift width matrix should remain deterministic on this host',
    ),
    RuntimeProbe(
        probe_id='15__probe_policy_impldef_char_promotion_matrix_tagged',
        source=PROBE_DIR / 'runtime/15__probe_policy_impldef_char_promotion_matrix_tagged.c',
        note='implementation-defined plain-char promotion matrix should remain deterministic on this host',
    ),
    RuntimeProbe(
        probe_id='15__probe_policy_ub_signed_overflow_chain_tagged',
        source=PROBE_DIR / 'runtime/15__probe_policy_ub_signed_overflow_chain_tagged.c',
        note='signed-overflow UB chain should remain tracked and deterministic in probe mode',
    ),
    RuntimeProbe(
        probe_id='15__probe_policy_ub_eval_order_call_sidefx_tagged',
        source=PROBE_DIR / 'runtime/15__probe_policy_ub_eval_order_call_sidefx_tagged.c',
        note='call-side-effect evaluation-order UB lane should remain tracked and deterministic in probe mode',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_vla_fnptr_feedback_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_vla_fnptr_feedback_matrix.c',
        note='single-TU VLA+function-pointer feedback matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_struct_union_reducer_chain',
        source=PROBE_DIR / 'runtime/15__probe_runtime_struct_union_reducer_chain.c',
        note='struct/union reducer chain should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_state_reseed_pipeline',
        source=PROBE_DIR / 'runtime/15__probe_multitu_state_reseed_pipeline_main.c',
        note='multi-TU state reseed pipeline with static ring should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_state_reseed_pipeline_main.c', PROBE_DIR / 'runtime/15__probe_multitu_state_reseed_pipeline_lib.c', PROBE_DIR / 'runtime/15__probe_multitu_state_reseed_pipeline_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_ceiling_recursion_depth_sweep',
        source=PROBE_DIR / 'runtime/15__probe_ceiling_recursion_depth_sweep.c',
        note='stress-ceiling recursion-depth sweep lane should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_ceiling_decl_density_sweep',
        source=PROBE_DIR / 'runtime/15__probe_ceiling_decl_density_sweep.c',
        note='stress-ceiling declaration-density sweep lane should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_ceiling_aggregate_size_sweep',
        source=PROBE_DIR / 'runtime/15__probe_ceiling_aggregate_size_sweep.c',
        note='stress-ceiling aggregate-size sweep lane should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_ceiling_multitu_layout_pressure_sweep',
        source=PROBE_DIR / 'runtime/15__probe_ceiling_multitu_layout_pressure_sweep_main.c',
        note='stress-ceiling multi-TU layout-pressure sweep lane should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_ceiling_multitu_layout_pressure_sweep_main.c', PROBE_DIR / 'runtime/15__probe_ceiling_multitu_layout_pressure_sweep_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_fuzz_seeded_expr_volume_replay',
        source=PROBE_DIR / 'runtime/15__probe_fuzz_seeded_expr_volume_replay.c',
        note='seeded fuzz-volume expression replay lane should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_fuzz_seeded_stmt_volume_replay',
        source=PROBE_DIR / 'runtime/15__probe_fuzz_seeded_stmt_volume_replay.c',
        note='seeded fuzz-volume statement/control replay lane should match clang runtime behavior',
    ),
]

DIAG_PROBES = []

DIAG_JSON_PROBES = []
