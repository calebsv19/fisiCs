from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="05__probe_wave35_realproj_ray_tracing_fnptr_null_expressions",
        source=PROBE_DIR / "runtime/05__probe_wave35_realproj_ray_tracing_fnptr_null_expressions.c",
        note=(
            "RayTracing regression: function pointers accept standard null "
            "pointer constants in conditional and equality expressions"
        ),
        promoted_test_id="05__runtime_wave35_realproj_ray_tracing_fnptr_null_expressions",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
