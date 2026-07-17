from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_wave138_realproj_tile_dispatch_all_fisics",
        source=PROBE_DIR / "runtime/15__probe_wave138_realproj_tile_dispatch_main.c",
        note="wave138 strict real-project extraction: the MapForge-derived tile-row callback pipeline must preserve aggregate state and its externally visible checksum across three fisiCs-built translation units",
        promoted_test_id="15__runtime_wave138_realproj_tile_dispatch_all_fisics",
        inputs=[
            PROBE_DIR / "runtime/15__probe_wave138_realproj_tile_dispatch_main.c",
            PROBE_DIR / "runtime/15__probe_wave138_realproj_tile_dispatch_ops_a.c",
            PROBE_DIR / "runtime/15__probe_wave138_realproj_tile_dispatch_ops_b.c",
        ],
    ),
    RuntimeProbe(
        probe_id="15__probe_wave138_realproj_tile_dispatch_mixed_clang_callbacks",
        source=PROBE_DIR / "runtime/15__probe_wave138_realproj_tile_dispatch_main.c",
        note="wave138 mixed oracle: fisiCs-built aggregate-state orchestration must dispatch callback symbols staged from a Clang-built implementation TU without changing the real-project-derived checksum",
        promoted_test_id="15__runtime_wave138_realproj_tile_dispatch_mixed_clang_callbacks",
        inputs=[
            PROBE_DIR / "runtime/15__probe_wave138_realproj_tile_dispatch_main.c",
            PROBE_DIR / "runtime/15__probe_wave138_realproj_tile_dispatch_ops_b.c",
        ],
        mixed_clang_inputs=[
            PROBE_DIR / "runtime/15__probe_wave138_realproj_tile_dispatch_ops_a.c",
        ],
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
