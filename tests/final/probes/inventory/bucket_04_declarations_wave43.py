from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_wave43_function_typedef_nested_unwind_adjust_strict",
        source=PROBE_DIR / "runtime/04__probe_wave43_function_typedef_nested_unwind_adjust_compile.c",
        note=(
            "wave43 strict: a function-type typedef must unwind a nested callback "
            "prototype scope and reconcile function plus multidimensional-array "
            "parameter adjustments with an explicit pointer-form redeclaration"
        ),
        promoted_test_id="04__compile__wave43_function_typedef_nested_unwind_adjust_strict",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave43_function_typedef_nested_unwind_direct_row_control",
        source=PROBE_DIR / "runtime/04__probe_wave43_function_typedef_nested_unwind_direct_row_control.c",
        note=(
            "wave43 current-threshold control: nested callback prototype-scope "
            "unwinding and direct typedef-array adjustment remain compatible when "
            "the additional outer array layer is removed"
        ),
        promoted_test_id="04__compile__wave43_function_typedef_nested_unwind_direct_row_control",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="04__probe_wave43_function_typedef_nested_shadow_reject",
        source=PROBE_DIR / "diagnostics/04__probe_wave43_function_typedef_nested_shadow_reject.c",
        note=(
            "wave43 strict negative: a named parameter inside the nested function "
            "prototype of a function-type typedef hides the same-spelled typedef "
            "from the later nested parameter"
        ),
        required_substrings=["wave43_nested_t"],
        promoted_test_id="04__diag__wave43_function_typedef_nested_shadow_reject",
    ),
]

DIAG_JSON_PROBES = []
