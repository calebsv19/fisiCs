from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="07__probe_wave90_enum_typedef_member_pointer_runtime",
        source=PROBE_DIR / "runtime/07__probe_wave90_enum_typedef_member_pointer_runtime.c",
        note=(
            "wave90 LineDrawing reduction: an enum-typedef aggregate member "
            "address is compatible with a pointer to the same enum typedef"
        ),
        promoted_test_id="07__runtime__wave90_enum_typedef_member_pointer",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="07__probe_wave90_distinct_enum_typedef_member_pointer_conflict",
        source=PROBE_DIR / "diagnostics/07__probe_wave90_distinct_enum_typedef_member_pointer_conflict.c",
        note=(
            "wave90 strict negative: member addresses of distinct anonymous-enum "
            "typedefs remain incompatible function arguments"
        ),
        required_substrings=["has incompatible type", "wave90_take_right"],
        promoted_test_id="07__diag__wave90_distinct_enum_typedef_member_pointer_conflict",
    ),
]

DIAG_JSON_PROBES = []
