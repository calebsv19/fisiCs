from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="08__probe_wave90_casted_static_subobject_address_strict",
        source=PROBE_DIR / "runtime/08__probe_wave90_casted_static_subobject_address_strict.c",
        note=(
            "wave90 strict: static address constants preserve cast-wrapped member "
            "and subscript addresses, conditional selection, and char-byte offsets"
        ),
        fisics_args=("-std=c99",),
        clang_args=("-pedantic-errors",),
        promoted_test_id="08__runtime_wave90_casted_static_subobject_address_strict",
    ),
    RuntimeProbe(
        probe_id="08__probe_wave90_direct_static_object_address_control",
        source=PROBE_DIR / "runtime/08__probe_wave90_direct_static_object_address_control.c",
        note=(
            "wave90 control: direct addresses of static scalar objects preserve "
            "the strict probe's deterministic pointer payload"
        ),
        fisics_args=("-std=c99",),
        clang_args=("-pedantic-errors",),
        promoted_test_id="08__runtime_wave90_direct_static_object_address_control",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
