from pathlib import Path

from lib.models import DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []
DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='11__probe_wave84_diagjson_call_arg_no_proto_expected_float_strict',
        source=PROBE_DIR / 'diagnostics/11__probe_wave84_diagjson_call_arg_no_proto_expected_float_strict.c',
        note='wave84 strict: a call parameter expecting a no-prototype function pointer must reject a float-prototyped function pointer argument',
        expected_codes=[2000],
        expected_line=19408,
        expected_column=35,
        expected_has_file=True,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
    DiagnosticJsonProbe(
        probe_id='11__probe_wave84_diagjson_call_arg_float_expected_no_proto_strict',
        source=PROBE_DIR / 'diagnostics/11__probe_wave84_diagjson_call_arg_float_expected_no_proto_strict.c',
        note='wave84 reverse strict: a call parameter expecting a float-prototyped function pointer must reject a no-prototype function pointer argument',
        expected_codes=[2000],
        expected_line=19408,
        expected_column=32,
        expected_has_file=True,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
    DiagnosticJsonProbe(
        probe_id='11__probe_wave84_diagjson_call_arg_no_proto_expected_double_control_clean',
        source=PROBE_DIR / 'diagnostics/11__probe_wave84_diagjson_call_arg_no_proto_expected_double_control.c',
        note='wave84 clean control: a no-prototype function-pointer parameter remains compatible with a double-prototyped pointer argument',
        require_any_diagnostic=False,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
    DiagnosticJsonProbe(
        probe_id='11__probe_wave84_diagjson_call_arg_double_expected_no_proto_control_clean',
        source=PROBE_DIR / 'diagnostics/11__probe_wave84_diagjson_call_arg_double_expected_no_proto_control.c',
        note='wave84 reverse clean control: a double-prototyped function-pointer parameter remains compatible with a no-prototype pointer argument',
        require_any_diagnostic=False,
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
]
