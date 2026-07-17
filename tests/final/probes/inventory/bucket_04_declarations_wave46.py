from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_wave46_nested_callback_equivalent_order_control",
        source=PROBE_DIR / "runtime/04__probe_wave46_nested_callback_equivalent_order_control.c",
        note=(
            "wave46 nested declaration-order control: an int callback prototype "
            "remains compatible across an intervening old-style callback spelling "
            "and a repeated equivalent prototype; a nested incomplete array bound "
            "is likewise refined and repeated, and both callbacks remain callable"
        ),
        promoted_test_id="04__runtime__wave46_nested_callback_equivalent_order_control",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="04__probe_wave46_nested_callback_prototype_composite_conflict",
        source=PROBE_DIR / "diagnostics/04__probe_wave46_nested_callback_prototype_composite_conflict.c",
        note=(
            "wave46 strict negative: after an old-style nested callback is refined "
            "to an int prototype, a later double callback prototype must conflict "
            "with the accumulated enclosing function type"
        ),
        required_substrings=[
            "Conflicting types for function",
            "wave46_nested_prototype",
        ],
        promoted_test_id="04__diag__wave46_nested_callback_prototype_composite_conflict",
    ),
    DiagnosticProbe(
        probe_id="04__probe_wave46_nested_callback_array_bound_composite_conflict",
        source=PROBE_DIR / "diagnostics/04__probe_wave46_nested_callback_array_bound_composite_conflict.c",
        note=(
            "wave46 strict negative: a nested callback parameter's incomplete "
            "pointer-to-array type is refined to bound 3, so a later bound 4 "
            "declaration must conflict with the accumulated enclosing function type"
        ),
        required_substrings=[
            "Conflicting types for function",
            "wave46_nested_array",
        ],
        promoted_test_id="04__diag__wave46_nested_callback_array_bound_composite_conflict",
    ),
]

DIAG_JSON_PROBES = []
