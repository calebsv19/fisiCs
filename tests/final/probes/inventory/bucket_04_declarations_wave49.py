from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_wave49_opaque_aggregate_typedef_prototype_runtime",
        source=PROBE_DIR / "runtime/04__probe_wave49_opaque_aggregate_typedef_prototype_runtime.c",
        note=(
            "wave49 MapForge reduction: file-scope typedefs for incomplete "
            "aggregate tags establish one visible identity across a prototype "
            "and its matching function definition"
        ),
        promoted_test_id="04__runtime__wave49_opaque_aggregate_typedef_prototype",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
