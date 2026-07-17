from pathlib import Path

from lib.models import DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []
DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='11__probe_wave85_diagjson_no_prototype_float_complex_control_clean',
        source=PROBE_DIR / 'diagnostics/11__probe_wave85_diagjson_no_prototype_float_complex_control.c',
        note='wave85 clean: float _Complex is not changed by C99 default argument promotions, so its prototype remains compatible with an empty parameter list',
        require_any_diagnostic=False,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
    DiagnosticJsonProbe(
        probe_id='11__probe_wave85_diagjson_no_prototype_double_complex_control_clean',
        source=PROBE_DIR / 'diagnostics/11__probe_wave85_diagjson_no_prototype_double_complex_control.c',
        note='wave85 clean: double _Complex survives default argument promotions and remains compatible with an empty parameter list',
        require_any_diagnostic=False,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
    DiagnosticJsonProbe(
        probe_id='11__probe_wave85_diagjson_no_prototype_real_float_strict',
        source=PROBE_DIR / 'diagnostics/11__probe_wave85_diagjson_no_prototype_real_float_strict.c',
        note='wave85 strict control: real float promotes to double and therefore remains incompatible with an empty parameter list declaration',
        expected_codes=[2000],
        expected_line=19502,
        expected_column=5,
        expected_has_file=True,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
]
