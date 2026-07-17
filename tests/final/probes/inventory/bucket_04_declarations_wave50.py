from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_wave50_anonymous_enum_typedef_prototype_runtime",
        source=PROBE_DIR / "runtime/04__probe_wave50_anonymous_enum_typedef_prototype_runtime.c",
        note=(
            "wave50 LineDrawing reduction: one anonymous-enum typedef keeps "
            "its declaration identity across matching prototypes and definitions"
        ),
        promoted_test_id="04__runtime__wave50_anonymous_enum_typedef_prototype",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="04__probe_wave50_distinct_anonymous_enum_typedef_conflict",
        source=PROBE_DIR / "diagnostics/04__probe_wave50_distinct_anonymous_enum_typedef_conflict.c",
        note=(
            "wave50 strict negative: distinct anonymous-enum typedefs remain "
            "incompatible in function redeclarations"
        ),
        required_substrings=["Conflicting types for function", "wave50_distinct"],
        promoted_test_id="04__diag__wave50_distinct_anonymous_enum_typedef_conflict",
    ),
]

DIAG_JSON_PROBES = []
