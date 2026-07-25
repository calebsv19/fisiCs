from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


def osp3_probe(probe_id, source_name, family, seed, digest):
    return RuntimeProbe(
        probe_id=probe_id,
        source=PROBE_DIR / f"runtime/{source_name}",
        note=(
            f"OS-P3 Slice 0 {family}: fixed seed {seed}, 256-case budget, "
            "pinned runtime oracle, and fisiCs/Clang differential replay"
        ),
        expected_exit_code=0,
        expected_stdout=(
            f"OSP3 {family} seed={seed} cases=256 digest={digest}\n"
        ),
        expected_stderr="",
    )


RUNTIME_PROBES = [
    osp3_probe(
        "15__probe_osp3_admission_precedence_matrix",
        "15__probe_osp3_admission_precedence_matrix.c",
        "admission",
        "31a0d17b",
        "1029102963",
    ),
    osp3_probe(
        "15__probe_osp3_queue_transition_matrix",
        "15__probe_osp3_queue_transition_matrix.c",
        "queue",
        "715ee93d",
        "2629161456",
    ),
    osp3_probe(
        "15__probe_osp3_scheduler_selection_matrix",
        "15__probe_osp3_scheduler_selection_matrix.c",
        "scheduler",
        "5ce4d219",
        "3968571514",
    ),
    osp3_probe(
        "15__probe_osp3_sync_rank_matrix",
        "15__probe_osp3_sync_rank_matrix.c",
        "sync-rank",
        "4a4f92c1",
        "336878484",
    ),
    osp3_probe(
        "15__probe_osp3_extent_overflow_matrix",
        "15__probe_osp3_extent_overflow_matrix.c",
        "extent",
        "e87a3c55",
        "3585114500",
    ),
    osp3_probe(
        "15__probe_osp3_generation_token_matrix",
        "15__probe_osp3_generation_token_matrix.c",
        "token",
        "b5297a4d",
        "3982822612",
    ),
    osp3_probe(
        "15__probe_osp3_scalar_double_dyadic_matrix",
        "15__probe_osp3_scalar_double_dyadic_matrix.c",
        "scalar-double",
        "c3d2e1f0",
        "1219963668",
    ),
    osp3_probe(
        "15__probe_osp3_variadic_struct_return_matrix",
        "15__probe_osp3_variadic_struct_return_matrix.c",
        "variadic-sret",
        "8f7e6d5c",
        "2596335816",
    ),
    osp3_probe(
        "15__probe_osp3_callback_dispatch_matrix",
        "15__probe_osp3_callback_dispatch_matrix.c",
        "callback",
        "126ef4a9",
        "1106495769",
    ),
    osp3_probe(
        "15__probe_osp3_aggregate_checkpoint_matrix",
        "15__probe_osp3_aggregate_checkpoint_matrix.c",
        "aggregate",
        "f1357bd9",
        "864968105",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
