from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
STRICT_CLANG_ARGS = ["-std=c99", "-pedantic-errors"]

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="06__probe_wave26_adjusted_function_parameter_reassign",
        source=PROBE_DIR / "runtime/06__probe_wave26_adjusted_function_parameter_reassign_runtime.c",
        note=(
            "wave26 strict frontier: a parameter declared with function type is "
            "adjusted to a modifiable function-pointer object and may be reassigned"
        ),
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="06__runtime__wave26_adjusted_function_parameter_reassign",
    ),
    RuntimeProbe(
        probe_id="06__probe_wave26_function_pointer_object_assignment_control",
        source=PROBE_DIR / "runtime/06__probe_wave26_function_pointer_object_assignment_control_runtime.c",
        note=(
            "wave26 current-threshold control: a direct function-pointer object "
            "remains assignable between compatible callbacks"
        ),
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="06__runtime__wave26_function_pointer_object_assignment_control",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="06__probe_wave26_file_scope_function_designator_assignment_reject",
        source=PROBE_DIR / "diagnostics/06__probe_wave26_file_scope_function_designator_assignment_reject.c",
        note=(
            "wave26 negative control: a function designator itself is not a "
            "modifiable lvalue and must reject assignment"
        ),
        required_substrings=["modifiable lvalue"],
        promoted_test_id="06__diag__wave26_file_scope_function_designator_assignment_reject",
    ),
]

DIAG_JSON_PROBES = []
