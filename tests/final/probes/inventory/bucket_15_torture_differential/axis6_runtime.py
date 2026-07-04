from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


def _probe(probe_id: str, *files: str) -> RuntimeProbe:
    inputs = [PROBE_DIR / f"runtime/{name}" for name in files]
    payload = dict(
        probe_id=probe_id,
        source=inputs[0],
        note="promoted axis6 scheduler lane should match clang runtime behavior",
    )
    if len(inputs) > 1:
        payload["inputs"] = inputs
    return RuntimeProbe(**payload)


RUNTIME_PROBES = [
    _probe("15__probe_axis6_wave1_scheduler_lane_rotation_equivalence_matrix", "15__probe_axis6_wave1_scheduler_lane_rotation_equivalence_matrix.c"),
    _probe("15__probe_axis6_wave1_scheduler_resume_budget_idempotence_matrix", "15__probe_axis6_wave1_scheduler_resume_budget_idempotence_matrix.c"),
    _probe("15__probe_axis6_wave1_scheduler_cross_tu_partition_fanout_matrix", "15__probe_axis6_wave1_scheduler_cross_tu_partition_fanout_matrix_main.c", "15__probe_axis6_wave1_scheduler_cross_tu_partition_fanout_matrix_lib.c"),
    _probe("15__probe_axis6_wave2_scheduler_phase_budget_projection_matrix", "15__probe_axis6_wave2_scheduler_phase_budget_projection_matrix.c"),
    _probe("15__probe_axis6_wave2_scheduler_replay_window_dedup_matrix", "15__probe_axis6_wave2_scheduler_replay_window_dedup_matrix.c"),
    _probe("15__probe_axis6_wave2_scheduler_cross_tu_resume_projection_matrix", "15__probe_axis6_wave2_scheduler_cross_tu_resume_projection_matrix_main.c", "15__probe_axis6_wave2_scheduler_cross_tu_resume_projection_matrix_lib.c"),
    _probe("15__probe_axis6_wave3_scheduler_lane_checkpoint_canonicalization_matrix", "15__probe_axis6_wave3_scheduler_lane_checkpoint_canonicalization_matrix.c"),
    _probe("15__probe_axis6_wave3_scheduler_resume_shadow_dedup_matrix", "15__probe_axis6_wave3_scheduler_resume_shadow_dedup_matrix.c"),
    _probe("15__probe_axis6_wave3_scheduler_cross_tu_checkpoint_fanout_matrix", "15__probe_axis6_wave3_scheduler_cross_tu_checkpoint_fanout_matrix_main.c", "15__probe_axis6_wave3_scheduler_cross_tu_checkpoint_fanout_matrix_lib.c"),
    _probe("15__probe_axis6_wave4_scheduler_phase_watermark_projection_matrix", "15__probe_axis6_wave4_scheduler_phase_watermark_projection_matrix.c"),
    _probe("15__probe_axis6_wave4_scheduler_replay_watermark_dedup_matrix", "15__probe_axis6_wave4_scheduler_replay_watermark_dedup_matrix.c"),
    _probe("15__probe_axis6_wave4_scheduler_cross_tu_watermark_fanout_matrix", "15__probe_axis6_wave4_scheduler_cross_tu_watermark_fanout_matrix_main.c", "15__probe_axis6_wave4_scheduler_cross_tu_watermark_fanout_matrix_lib.c"),
    _probe("15__probe_axis6_wave5_scheduler_shadow_projection_matrix", "15__probe_axis6_wave5_scheduler_shadow_projection_matrix.c"),
    _probe("15__probe_axis6_wave5_scheduler_replay_shadow_window_matrix", "15__probe_axis6_wave5_scheduler_replay_shadow_window_matrix.c"),
    _probe("15__probe_axis6_wave5_scheduler_cross_tu_shadow_mesh_matrix", "15__probe_axis6_wave5_scheduler_cross_tu_shadow_mesh_matrix_main.c", "15__probe_axis6_wave5_scheduler_cross_tu_shadow_mesh_matrix_lib.c"),
    _probe("15__probe_axis6_wave6_scheduler_checkpoint_braid_projection_matrix", "15__probe_axis6_wave6_scheduler_checkpoint_braid_projection_matrix.c"),
    _probe("15__probe_axis6_wave6_scheduler_replay_braid_dedup_matrix", "15__probe_axis6_wave6_scheduler_replay_braid_dedup_matrix.c"),
    _probe("15__probe_axis6_wave6_scheduler_cross_tu_checkpoint_braid_matrix", "15__probe_axis6_wave6_scheduler_cross_tu_checkpoint_braid_matrix_main.c", "15__probe_axis6_wave6_scheduler_cross_tu_checkpoint_braid_matrix_lib.c"),
    _probe("15__probe_axis6_wave7_scheduler_phase_checkpoint_rotation_matrix", "15__probe_axis6_wave7_scheduler_phase_checkpoint_rotation_matrix.c"),
    _probe("15__probe_axis6_wave7_scheduler_watermark_shadow_absorption_matrix", "15__probe_axis6_wave7_scheduler_watermark_shadow_absorption_matrix.c"),
    _probe("15__probe_axis6_wave7_scheduler_cross_tu_phase_rotation_matrix", "15__probe_axis6_wave7_scheduler_cross_tu_phase_rotation_matrix_main.c", "15__probe_axis6_wave7_scheduler_cross_tu_phase_rotation_matrix_lib.c"),
    _probe("15__probe_axis6_wave8_scheduler_frontier_spill_projection_matrix", "15__probe_axis6_wave8_scheduler_frontier_spill_projection_matrix.c"),
    _probe("15__probe_axis6_wave8_scheduler_replay_spill_dedup_matrix", "15__probe_axis6_wave8_scheduler_replay_spill_dedup_matrix.c"),
    _probe("15__probe_axis6_wave8_scheduler_cross_tu_frontier_spill_matrix", "15__probe_axis6_wave8_scheduler_cross_tu_frontier_spill_matrix_main.c", "15__probe_axis6_wave8_scheduler_cross_tu_frontier_spill_matrix_lib.c"),
    _probe("15__probe_axis6_wave9_scheduler_handoff_epoch_fold_matrix", "15__probe_axis6_wave9_scheduler_handoff_epoch_fold_matrix.c"),
    _probe("15__probe_axis6_wave9_scheduler_resume_epoch_shadow_dedup_matrix", "15__probe_axis6_wave9_scheduler_resume_epoch_shadow_dedup_matrix.c"),
    _probe("15__probe_axis6_wave9_scheduler_cross_tu_handoff_epoch_matrix", "15__probe_axis6_wave9_scheduler_cross_tu_handoff_epoch_matrix_main.c", "15__probe_axis6_wave9_scheduler_cross_tu_handoff_epoch_matrix_lib.c"),
    _probe("15__probe_axis6_wave10_scheduler_lease_rebase_projection_matrix", "15__probe_axis6_wave10_scheduler_lease_rebase_projection_matrix.c"),
    _probe("15__probe_axis6_wave10_scheduler_replay_lease_shadow_dedup_matrix", "15__probe_axis6_wave10_scheduler_replay_lease_shadow_dedup_matrix.c"),
    _probe("15__probe_axis6_wave10_scheduler_cross_tu_lease_rebase_matrix", "15__probe_axis6_wave10_scheduler_cross_tu_lease_rebase_matrix_main.c", "15__probe_axis6_wave10_scheduler_cross_tu_lease_rebase_matrix_lib.c"),
    _probe("15__probe_axis6_wave11_scheduler_quorum_frontier_projection_matrix", "15__probe_axis6_wave11_scheduler_quorum_frontier_projection_matrix.c"),
    _probe("15__probe_axis6_wave11_scheduler_replay_quorum_dedup_matrix", "15__probe_axis6_wave11_scheduler_replay_quorum_dedup_matrix.c"),
    _probe("15__probe_axis6_wave11_scheduler_cross_tu_quorum_frontier_matrix", "15__probe_axis6_wave11_scheduler_cross_tu_quorum_frontier_matrix_main.c", "15__probe_axis6_wave11_scheduler_cross_tu_quorum_frontier_matrix_lib.c"),
    _probe("15__probe_axis6_wave12_scheduler_checkpoint_window_fold_matrix", "15__probe_axis6_wave12_scheduler_checkpoint_window_fold_matrix.c"),
    _probe("15__probe_axis6_wave12_scheduler_resume_window_shadow_dedup_matrix", "15__probe_axis6_wave12_scheduler_resume_window_shadow_dedup_matrix.c"),
    _probe("15__probe_axis6_wave12_scheduler_cross_tu_checkpoint_window_matrix", "15__probe_axis6_wave12_scheduler_cross_tu_checkpoint_window_matrix_main.c", "15__probe_axis6_wave12_scheduler_cross_tu_checkpoint_window_matrix_lib.c"),
    _probe("15__probe_axis6_wave13_scheduler_shard_order_canonicalization_matrix", "15__probe_axis6_wave13_scheduler_shard_order_canonicalization_matrix.c"),
    _probe("15__probe_axis6_wave13_scheduler_replay_checkpoint_watermark_collapse_matrix", "15__probe_axis6_wave13_scheduler_replay_checkpoint_watermark_collapse_matrix.c"),
    _probe("15__probe_axis6_wave13_scheduler_cross_tu_shard_watermark_collapse_matrix", "15__probe_axis6_wave13_scheduler_cross_tu_shard_watermark_collapse_matrix_main.c", "15__probe_axis6_wave13_scheduler_cross_tu_shard_watermark_collapse_matrix_lib.c"),
]
