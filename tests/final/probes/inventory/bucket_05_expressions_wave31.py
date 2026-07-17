from pathlib import Path

from lib.models import DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="05__probe_wave31_diagjson_no_prototype_float_complex_conditional_left_arity",
        source=PROBE_DIR / "diagnostics/05__probe_wave31_diagjson_no_prototype_float_complex_conditional_left_arity.c",
        note=(
            "wave31 prototype consumer: no-prototype-first conditional merging must "
            "preserve the float-complex prototype and reject a zero-argument call"
        ),
        expected_codes=[2000],
        expected_line=20107,
        expected_column=13,
        expected_has_file=True,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave31_no_prototype_float_complex_conditional_left_arity_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_wave31_diagjson_no_prototype_float_complex_conditional_right_arity",
        source=PROBE_DIR / "diagnostics/05__probe_wave31_diagjson_no_prototype_float_complex_conditional_right_arity.c",
        note=(
            "wave31 reverse-order consumer: prototype-first conditional merging must "
            "preserve the float-complex prototype and reject a zero-argument call"
        ),
        expected_codes=[2000],
        expected_line=20207,
        expected_column=13,
        expected_has_file=True,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave31_no_prototype_float_complex_conditional_right_arity_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_wave31_diagjson_no_prototype_real_float_conditional_strict",
        source=PROBE_DIR / "diagnostics/05__probe_wave31_diagjson_no_prototype_real_float_conditional_strict.c",
        note=(
            "wave31 strict control: conditional merging rejects no-prototype and "
            "real-float prototypes because float changes under default promotion"
        ),
        expected_codes=[2000],
        expected_line=20307,
        expected_column=0,
        expected_has_file=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave31_no_prototype_real_float_conditional_strict_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_wave31_diagjson_no_prototype_double_complex_conditional_clean",
        source=PROBE_DIR / "diagnostics/05__probe_wave31_diagjson_no_prototype_double_complex_conditional_clean.c",
        note=(
            "wave31 clean control: a consumed double-complex conditional call remains "
            "compatible with a no-prototype function pointer"
        ),
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave31_no_prototype_double_complex_conditional_clean_diagjson",
    ),
]
