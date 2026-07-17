from pathlib import Path

from lib.models import DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="05__probe_wave28_diagjson_function_pointer_relational_less",
        source=PROBE_DIR / "diagnostics/05__probe_wave28_diagjson_function_pointer_relational_less.c",
        note=(
            "wave28 strict: C99 relational less-than must reject two otherwise "
            "compatible function pointers"
        ),
        expected_codes=[2000],
        expected_line=19314,
        expected_column=12,
        expected_has_file=True,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave28_function_pointer_relational_less_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_wave28_diagjson_function_pointer_relational_greater_equal",
        source=PROBE_DIR / "diagnostics/05__probe_wave28_diagjson_function_pointer_relational_greater_equal.c",
        note=(
            "wave28 strict: C99 relational greater-than-or-equal must reject two "
            "otherwise compatible function pointers"
        ),
        expected_codes=[2000],
        expected_line=19414,
        expected_column=12,
        expected_has_file=True,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave28_function_pointer_relational_greater_equal_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_wave28_diagjson_function_pointer_equality_clean",
        source=PROBE_DIR / "diagnostics/05__probe_wave28_diagjson_function_pointer_equality_clean.c",
        note=(
            "wave28 clean control: equality and inequality remain valid between "
            "compatible function pointers"
        ),
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave28_function_pointer_equality_clean_diagjson",
    ),
]
