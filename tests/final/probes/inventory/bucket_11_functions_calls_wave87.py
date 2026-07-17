from pathlib import Path

from lib.models import DiagnosticJsonProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='11__probe_wave87_multitu_no_prototype_float_complex_runtime',
        source=PROBE_DIR / 'runtime/11__probe_wave87_multitu_no_prototype_float_complex_runtime_main.c',
        inputs=[
            PROBE_DIR / 'runtime/11__probe_wave87_multitu_no_prototype_float_complex_runtime_main.c',
            PROBE_DIR / 'runtime/11__probe_wave87_multitu_no_prototype_float_complex_runtime_lib.c',
        ],
        note='wave87: a no-prototype caller and float-complex definition in separate translation units must preserve both complex ABI lanes',
        clang_args=['-pedantic-errors', '-Wno-strict-prototypes', '-Wno-deprecated-non-prototype'],
    ),
]
DIAG_PROBES = []
DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='11__probe_wave87_diagjson_multitu_no_prototype_real_float_conflict_strict',
        source=PROBE_DIR / 'diagnostics/11__probe_wave87_diagjson_multitu_no_prototype_real_float_main.c',
        inputs=[
            PROBE_DIR / 'diagnostics/11__probe_wave87_diagjson_multitu_no_prototype_real_float_main.c',
            PROBE_DIR / 'diagnostics/11__probe_wave87_diagjson_multitu_no_prototype_real_float_lib.c',
        ],
        note='wave87 strict: cross-TU signature merging must reject an empty-list declaration paired with a real-float definition',
        expected_codes=[2000],
        expected_line=19751,
        expected_column=12,
        expected_has_file=True,
    ),
]
