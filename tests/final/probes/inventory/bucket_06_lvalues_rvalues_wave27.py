from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
STRICT_CLANG_ARGS = ["-std=c99", "-pedantic-errors"]

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="06__probe_wave27_typedef_function_parameter_rebind",
        source=PROBE_DIR / "runtime/06__probe_wave27_typedef_function_parameter_rebind_runtime.c",
        note=(
            "wave27 strict frontier: a function-type typedef parameter adjusts "
            "to a modifiable function-pointer object before body binding"
        ),
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="06__runtime__wave27_typedef_function_parameter_rebind",
    ),
    RuntimeProbe(
        probe_id="06__probe_wave27_typedef_array_parameter_rebind",
        source=PROBE_DIR / "runtime/06__probe_wave27_typedef_array_parameter_rebind_runtime.c",
        note=(
            "wave27 strict frontier: an array-type typedef parameter adjusts "
            "to a modifiable pointer object before body binding"
        ),
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="06__runtime__wave27_typedef_array_parameter_rebind",
    ),
    RuntimeProbe(
        probe_id="06__probe_wave27_element_const_array_parameter_rebind_control",
        source=PROBE_DIR / "runtime/06__probe_wave27_element_const_array_parameter_rebind_control_runtime.c",
        note=(
            "wave27 qualifier control: element constness survives array adjustment "
            "without making the adjusted parameter pointer nonmodifiable"
        ),
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="06__runtime__wave27_element_const_array_parameter_rebind_control",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="06__probe_wave27_bracket_const_array_parameter_rebind_reject",
        source=PROBE_DIR / "diagnostics/06__probe_wave27_bracket_const_array_parameter_rebind_reject.c",
        note=(
            "wave27 negative qualifier control: const inside array brackets "
            "qualifies the adjusted parameter pointer and forbids reassignment"
        ),
        required_substrings=["modifiable lvalue"],
        promoted_test_id="06__diag__wave27_bracket_const_array_parameter_rebind_reject",
    ),
]

DIAG_JSON_PROBES = []
