from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_wave44_grouped_row_bound_compat",
        source=PROBE_DIR / "runtime/04__probe_wave44_grouped_row_bound_compat_runtime.c",
        note=(
            "wave44 current-threshold reduction: a grouped pointer-to-array parameter with an enum "
            "bound remains compatible with adjusted nested-array syntax using "
            "the equivalent sizeof expression, and preserves row indexing"
        ),
        promoted_test_id="04__runtime__wave44_grouped_row_bound_compat",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave44_grouped_nested_direct_control",
        source=PROBE_DIR / "runtime/04__probe_wave44_grouped_nested_direct_control_runtime.c",
        note=(
            "wave44 distinct control: a pointer to a direct two-dimensional "
            "array reconciles equivalent enum and sizeof bounds while preserving "
            "both nested bound order and runtime indexing"
        ),
        promoted_test_id="04__runtime__wave44_grouped_nested_direct_control",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave44_grouped_vla_fixed_bound_compat",
        source=PROBE_DIR / "runtime/04__probe_wave44_grouped_vla_fixed_bound_compat.c",
        note=(
            "wave44 VLA compatibility control: a prototype-scope variable inner "
            "bound remains compatible with a later fixed inner bound"
        ),
        promoted_test_id="04__runtime__wave44_grouped_vla_fixed_bound_compat",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="04__probe_wave44_grouped_inner_bound_conflict",
        source=PROBE_DIR / "diagnostics/04__probe_wave44_grouped_inner_bound_conflict.c",
        note=(
            "wave44 strict negative: grouped pointer-to-array redeclarations with "
            "different constant inner bounds must be rejected as conflicting types"
        ),
        required_substrings=[
            "Conflicting types for function",
            "wave44_bound_conflict",
        ],
        promoted_test_id="04__diag__wave44_grouped_inner_bound_conflict",
    ),
]

DIAG_JSON_PROBES = []
