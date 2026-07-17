from pathlib import Path

from lib.models import DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="05__probe_wave29_diagjson_no_prototype_float_equality_strict",
        source=PROBE_DIR / "diagnostics/05__probe_wave29_diagjson_no_prototype_float_equality_strict.c",
        note=(
            "wave29 strict: equality must reject no-prototype and float-prototyped "
            "function pointers because float changes under default argument promotion"
        ),
        expected_codes=[2000],
        expected_line=19607,
        expected_column=12,
        expected_has_file=True,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave29_no_prototype_float_equality_diagjson_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_wave29_diagjson_no_prototype_double_equality_control_clean",
        source=PROBE_DIR / "diagnostics/05__probe_wave29_diagjson_no_prototype_double_equality_control.c",
        note=(
            "wave29 clean control: equality and reversed inequality remain valid "
            "between no-prototype and double-prototyped function pointers"
        ),
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave29_no_prototype_double_equality_control_diagjson",
    ),
]
