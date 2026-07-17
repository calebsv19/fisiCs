from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_wave45_equivalent_prototype_order_control",
        source=PROBE_DIR / "runtime/04__probe_wave45_equivalent_prototype_order_control.c",
        note=(
            "wave45 declaration-order control: an explicit int prototype remains "
            "compatible across an intervening old-style declaration and a repeated "
            "equivalent prototype, and the definition remains callable"
        ),
        promoted_test_id="04__runtime__wave45_equivalent_prototype_order_control",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="04__probe_wave45_oldstyle_prototype_composite_conflict",
        source=PROBE_DIR / "diagnostics/04__probe_wave45_oldstyle_prototype_composite_conflict.c",
        note=(
            "wave45 strict negative: after an old-style declaration is refined to "
            "an int prototype, a later double prototype must conflict with the "
            "accumulated composite function type"
        ),
        required_substrings=[
            "Conflicting types for function",
            "wave45_prototype_composite",
        ],
        promoted_test_id="04__diag__wave45_oldstyle_prototype_composite_conflict",
    ),
    DiagnosticProbe(
        probe_id="04__probe_wave45_array_bound_composite_conflict",
        source=PROBE_DIR / "diagnostics/04__probe_wave45_array_bound_composite_conflict.c",
        note=(
            "wave45 strict negative: after an incomplete pointer-to-array parameter "
            "is refined to bound 3, a later bound 4 declaration must conflict with "
            "the accumulated composite function type"
        ),
        required_substrings=[
            "Conflicting types for function",
            "wave45_array_composite",
        ],
        promoted_test_id="04__diag__wave45_array_bound_composite_conflict",
    ),
]

DIAG_JSON_PROBES = []
