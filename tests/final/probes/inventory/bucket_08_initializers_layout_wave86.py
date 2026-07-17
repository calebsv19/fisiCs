from pathlib import Path

from lib.models import DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []
DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='08__probe_diagjson_wave86_no_prototype_float_array_initializer_strict',
        source=PROBE_DIR / 'diagnostics/08__probe_diagjson_wave86_no_prototype_float_array_initializer_strict.c',
        note='wave86 strict: an array of no-prototype function pointers must reject a float-prototyped element initializer',
        expected_codes=[2000],
        expected_line=19007,
        expected_column=0,
        expected_has_file=False,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
    DiagnosticJsonProbe(
        probe_id='08__probe_diagjson_wave86_no_prototype_float_array_initializer_reverse_strict',
        source=PROBE_DIR / 'diagnostics/08__probe_diagjson_wave86_no_prototype_float_array_initializer_reverse_strict.c',
        note='wave86 reverse strict: an array of float-prototyped function pointers must reject a no-prototype element initializer',
        expected_codes=[2000],
        expected_line=19107,
        expected_column=0,
        expected_has_file=False,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
    DiagnosticJsonProbe(
        probe_id='08__probe_diagjson_wave86_no_prototype_double_array_initializer_compatible',
        source=PROBE_DIR / 'diagnostics/08__probe_diagjson_wave86_no_prototype_double_array_initializer_compatible.c',
        note='wave86 compatible control: array elements accept both directions between no-prototype and double-prototyped function pointers',
        require_any_diagnostic=False,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
]
