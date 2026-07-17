from pathlib import Path

from lib.models import DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []
DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='11__probe_wave83_diagjson_prototype_first_float_boundary_strict',
        source=PROBE_DIR / 'diagnostics/11__probe_wave83_diagjson_prototype_first_float_boundary_strict.c',
        note='wave83 reversed-order strict: a float prototype remains incompatible with a later empty parameter list because float does not survive default argument promotion',
        expected_codes=[2000],
        expected_line=19302,
        expected_column=5,
        expected_has_file=True,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
    DiagnosticJsonProbe(
        probe_id='11__probe_wave83_diagjson_prototype_first_double_control_clean',
        source=PROBE_DIR / 'diagnostics/11__probe_wave83_diagjson_prototype_first_double_control.c',
        note='wave83 reversed-order clean control: a double prototype remains compatible with a later empty parameter list',
        require_any_diagnostic=False,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
    DiagnosticJsonProbe(
        probe_id='11__probe_wave83_diagjson_no_prototype_variadic_boundary_strict',
        source=PROBE_DIR / 'diagnostics/11__probe_wave83_diagjson_no_prototype_variadic_boundary_strict.c',
        note='wave83 strict: an empty parameter list is incompatible with a variadic prototype regardless of fixed-parameter promotion compatibility',
        expected_codes=[2000],
        expected_line=19302,
        expected_column=5,
        expected_has_file=True,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
]
