from pathlib import Path

from lib.models import DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="05__probe_wave30_diagjson_no_prototype_float_complex_equality_clean",
        source=PROBE_DIR / "diagnostics/05__probe_wave30_diagjson_no_prototype_float_complex_equality_clean.c",
        note=(
            "wave30 clean: equality and reversed inequality remain valid between "
            "no-prototype and float-complex-prototyped function pointers"
        ),
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave30_no_prototype_float_complex_equality_clean_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_wave30_diagjson_no_prototype_real_float_equality_strict",
        source=PROBE_DIR / "diagnostics/05__probe_wave30_diagjson_no_prototype_real_float_equality_strict.c",
        note=(
            "wave30 strict control: equality must reject no-prototype and real-float "
            "prototypes because real float promotes to double"
        ),
        expected_codes=[2000],
        expected_line=19907,
        expected_column=12,
        expected_has_file=True,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave30_no_prototype_real_float_equality_strict_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_wave30_diagjson_no_prototype_double_complex_inequality_clean",
        source=PROBE_DIR / "diagnostics/05__probe_wave30_diagjson_no_prototype_double_complex_inequality_clean.c",
        note=(
            "wave30 clean control: double-complex remains unchanged by default "
            "argument promotions and compares compatibly with a no-prototype pointer"
        ),
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave30_no_prototype_double_complex_inequality_clean_diagjson",
    ),
]
