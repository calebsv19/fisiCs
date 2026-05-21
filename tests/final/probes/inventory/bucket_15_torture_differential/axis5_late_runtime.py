from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


def _probe(probe_id: str, *files: str) -> RuntimeProbe:
    inputs = [PROBE_DIR / f"runtime/{name}" for name in files]
    payload = dict(
        probe_id=probe_id,
        source=inputs[0],
        note="late promoted axis5 reducer lane should match clang runtime behavior",
    )
    if len(inputs) > 1:
        payload["inputs"] = inputs
    return RuntimeProbe(**payload)


RUNTIME_PROBES = [
    _probe("15__probe_axis5_wave18_reducer_watermark_checkpoint_projection_matrix", "15__probe_axis5_wave18_reducer_watermark_checkpoint_projection_matrix.c"),
    _probe("15__probe_axis5_wave18_reducer_frontier_checkpoint_tie_fold_equivalence_matrix", "15__probe_axis5_wave18_reducer_frontier_checkpoint_tie_fold_equivalence_matrix.c"),
    _probe("15__probe_axis5_wave18_reducer_cross_tu_frontier_checkpoint_lattice_matrix", "15__probe_axis5_wave18_reducer_cross_tu_frontier_checkpoint_lattice_matrix_main.c", "15__probe_axis5_wave18_reducer_cross_tu_frontier_checkpoint_lattice_matrix_lib.c"),
    _probe("15__probe_axis5_wave19_reducer_checkpoint_watermark_stability_matrix", "15__probe_axis5_wave19_reducer_checkpoint_watermark_stability_matrix.c"),
    _probe("15__probe_axis5_wave19_reducer_frontier_projection_tie_equivalence_matrix", "15__probe_axis5_wave19_reducer_frontier_projection_tie_equivalence_matrix.c"),
    _probe("15__probe_axis5_wave19_reducer_cross_tu_checkpoint_watermark_braid_matrix", "15__probe_axis5_wave19_reducer_cross_tu_checkpoint_watermark_braid_matrix_main.c", "15__probe_axis5_wave19_reducer_cross_tu_checkpoint_watermark_braid_matrix_lib.c"),
    _probe("15__probe_axis5_wave20_reducer_checkpoint_frontier_dedup_matrix", "15__probe_axis5_wave20_reducer_checkpoint_frontier_dedup_matrix.c"),
    _probe("15__probe_axis5_wave20_reducer_partial_order_canonicalization_matrix", "15__probe_axis5_wave20_reducer_partial_order_canonicalization_matrix.c"),
    _probe("15__probe_axis5_wave20_reducer_cross_tu_replay_dedup_splice_matrix", "15__probe_axis5_wave20_reducer_cross_tu_replay_dedup_splice_matrix_main.c", "15__probe_axis5_wave20_reducer_cross_tu_replay_dedup_splice_matrix_lib.c"),
    _probe("15__probe_axis5_wave21_reducer_watermark_frontier_canonicalization_matrix", "15__probe_axis5_wave21_reducer_watermark_frontier_canonicalization_matrix.c"),
    _probe("15__probe_axis5_wave21_reducer_checkpoint_watermark_partial_order_matrix", "15__probe_axis5_wave21_reducer_checkpoint_watermark_partial_order_matrix.c"),
    _probe("15__probe_axis5_wave21_reducer_cross_tu_watermark_canonical_splice_matrix", "15__probe_axis5_wave21_reducer_cross_tu_watermark_canonical_splice_matrix_main.c", "15__probe_axis5_wave21_reducer_cross_tu_watermark_canonical_splice_matrix_lib.c"),
    _probe("15__probe_axis5_wave22_reducer_checkpoint_frontier_braid_matrix", "15__probe_axis5_wave22_reducer_checkpoint_frontier_braid_matrix.c"),
    _probe("15__probe_axis5_wave22_reducer_frontier_watermark_partial_order_matrix", "15__probe_axis5_wave22_reducer_frontier_watermark_partial_order_matrix.c"),
    _probe("15__probe_axis5_wave22_reducer_cross_tu_checkpoint_canonical_mesh_matrix", "15__probe_axis5_wave22_reducer_cross_tu_checkpoint_canonical_mesh_matrix_main.c", "15__probe_axis5_wave22_reducer_cross_tu_checkpoint_canonical_mesh_matrix_lib.c"),
    _probe("15__probe_axis5_wave23_reducer_frontier_checkpoint_splice_matrix", "15__probe_axis5_wave23_reducer_frontier_checkpoint_splice_matrix.c"),
    _probe("15__probe_axis5_wave23_reducer_checkpoint_frontier_partial_order_matrix", "15__probe_axis5_wave23_reducer_checkpoint_frontier_partial_order_matrix.c"),
    _probe("15__probe_axis5_wave23_reducer_cross_tu_frontier_canonical_splice_matrix", "15__probe_axis5_wave23_reducer_cross_tu_frontier_canonical_splice_matrix_main.c", "15__probe_axis5_wave23_reducer_cross_tu_frontier_canonical_splice_matrix_lib.c"),
    _probe("15__probe_axis5_wave24_reducer_watermark_frontier_collapse_matrix", "15__probe_axis5_wave24_reducer_watermark_frontier_collapse_matrix.c"),
    _probe("15__probe_axis5_wave24_reducer_replay_checkpoint_dedup_matrix", "15__probe_axis5_wave24_reducer_replay_checkpoint_dedup_matrix.c"),
    _probe("15__probe_axis5_wave24_reducer_cross_tu_watermark_canonical_fanout_matrix", "15__probe_axis5_wave24_reducer_cross_tu_watermark_canonical_fanout_matrix_main.c", "15__probe_axis5_wave24_reducer_cross_tu_watermark_canonical_fanout_matrix_lib.c"),
    _probe("15__probe_axis5_wave25_reducer_frontier_watermark_projection_matrix", "15__probe_axis5_wave25_reducer_frontier_watermark_projection_matrix.c"),
    _probe("15__probe_axis5_wave25_reducer_checkpoint_shadow_canonicalization_matrix", "15__probe_axis5_wave25_reducer_checkpoint_shadow_canonicalization_matrix.c"),
    _probe("15__probe_axis5_wave25_reducer_cross_tu_epoch_watermark_projection_matrix", "15__probe_axis5_wave25_reducer_cross_tu_epoch_watermark_projection_matrix_main.c", "15__probe_axis5_wave25_reducer_cross_tu_epoch_watermark_projection_matrix_lib.c"),
]
