from pathlib import Path

from lib.models import DiagnosticJsonProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="05__probe_wave32_static_conditional_inactive_call_constant_runtime",
        source=PROBE_DIR / "runtime/05__probe_wave32_static_conditional_inactive_call_constant_runtime.c",
        note=(
            "wave32 constant-expression control: a discarded function call remains "
            "valid in a static-storage conditional initializer and is never evaluated"
        ),
        clang_args=["-pedantic-errors"],
        promoted_test_id="05__runtime_wave32_static_conditional_inactive_call_constant",
    ),
]

DIAG_PROBES = []

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="05__probe_wave32_enum_conditional_inactive_call_ice_reject",
        source=PROBE_DIR / "diagnostics/05__probe_wave32_enum_conditional_inactive_call_ice_reject.c",
        note=(
            "wave32 strict C99 ICE: an enumerator conditional containing a discarded "
            "function call must reject instead of using GNU constant folding"
        ),
        expected_codes=[2000],
        expected_line=32001,
        expected_column=8,
        expected_has_file=True,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave32_enum_conditional_inactive_call_ice_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_wave32_enum_logical_inactive_call_ice_reject",
        source=PROBE_DIR / "diagnostics/05__probe_wave32_enum_logical_inactive_call_ice_reject.c",
        note=(
            "wave32 strict C99 ICE: an enumerator logical expression containing a "
            "short-circuited function call must reject instead of using GNU folding"
        ),
        expected_codes=[2000],
        expected_line=32002,
        expected_column=8,
        expected_has_file=True,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave32_enum_logical_inactive_call_ice_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_wave32_static_conditional_selected_call_reject",
        source=PROBE_DIR / "diagnostics/05__probe_wave32_static_conditional_selected_call_reject.c",
        note=(
            "wave32 static-initializer boundary: a selected function-call arm is not "
            "a constant initializer"
        ),
        expected_codes=[2000],
        expected_line=32004,
        expected_column=0,
        expected_has_file=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave32_static_conditional_selected_call_diagjson",
    ),
]
