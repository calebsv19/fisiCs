from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="11__probe_wave89_realproj_request_callback_routing_runtime",
        source=PROBE_DIR / "runtime/11__probe_wave89_realproj_request_callback_routing_main.c",
        inputs=[
            PROBE_DIR / "runtime/11__probe_wave89_realproj_request_callback_routing_main.c",
            PROBE_DIR / "runtime/11__probe_wave89_realproj_request_callback_routing_lib.c",
        ],
        note="wave89 strict: pinned RayTracing-derived request structs must route distinct callback typedefs and void user-data through cross-TU wrappers before rejecting post-shutdown submission",
        clang_args=["-pedantic-errors", "-Wall", "-Wextra"],
        promoted_test_id="11__probe_wave89_realproj_request_callback_routing_runtime",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
