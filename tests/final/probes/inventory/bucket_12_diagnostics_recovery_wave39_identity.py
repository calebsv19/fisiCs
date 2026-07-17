from pathlib import Path

from lib.models import DiagnosticExpectation, DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="12__probe_wave39_diagjson_macro_trace_identity_collision_strict",
        source=PROBE_DIR / "diagnostics/12__probe_wave39_diagjson_macro_trace_identity_collision_strict.c",
        note=(
            "wave39 strict: an ALPHA arity diagnostic sharing code, line, and column "
            "with its peer remains identifiable by the ordered macro trace"
        ),
        expected_diagnostics=(
            DiagnosticExpectation(
                code=3000,
                line=3900,
                column=13,
                has_file=True,
                severity="error",
                stage="preprocess",
                macro_trace=(
                    {
                        "role": "call_site",
                        "macro": "WAVE39_ALPHA",
                        "file": "virtual_wave39_identity_collision.c",
                        "line": 3900,
                        "column": 13,
                    },
                    {
                        "role": "definition",
                        "macro": "WAVE39_ALPHA",
                        "file": "12__probe_wave39_diagjson_macro_trace_identity_collision_strict.c",
                        "line": 2,
                        "column": 1,
                    },
                ),
            ),
        ),
        promoted_test_id="12__diagjson_wave39_macro_trace_identity_collision_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_wave39_diagjson_macro_trace_identity_collision_peer_strict",
        source=PROBE_DIR / "diagnostics/12__probe_wave39_diagjson_macro_trace_identity_collision_peer_strict.c",
        note=(
            "wave39 strict: an OMEGA arity diagnostic at the same code, line, column, "
            "and virtual file as ALPHA retains its distinct ordered macro trace"
        ),
        expected_diagnostics=(
            DiagnosticExpectation(
                code=3000,
                line=3900,
                column=13,
                has_file=True,
                severity="error",
                stage="preprocess",
                macro_trace=(
                    {
                        "role": "call_site",
                        "macro": "WAVE39_OMEGA",
                        "file": "virtual_wave39_identity_collision.c",
                        "line": 3900,
                        "column": 13,
                    },
                    {
                        "role": "definition",
                        "macro": "WAVE39_OMEGA",
                        "file": "12__probe_wave39_diagjson_macro_trace_identity_collision_peer_strict.c",
                        "line": 2,
                        "column": 1,
                    },
                ),
            ),
        ),
        promoted_test_id="12__diagjson_wave39_macro_trace_identity_collision_peer_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_wave39_diagjson_macro_trace_identity_control",
        source=PROBE_DIR / "diagnostics/12__probe_wave39_diagjson_macro_trace_identity_control.c",
        note="wave39 control: a non-remapped direct macro error preserves its call-site and definition trace",
        expected_diagnostics=(
            DiagnosticExpectation(
                code=3000,
                line=2,
                column=15,
                has_file=True,
                severity="error",
                stage="preprocess",
                macro_trace=(
                    {
                        "role": "call_site",
                        "macro": "WAVE39_CONTROL",
                        "file": "12__probe_wave39_diagjson_macro_trace_identity_control.c",
                        "line": 2,
                        "column": 15,
                    },
                    {
                        "role": "definition",
                        "macro": "WAVE39_CONTROL",
                        "file": "12__probe_wave39_diagjson_macro_trace_identity_control.c",
                        "line": 1,
                        "column": 1,
                    },
                ),
            ),
        ),
        promoted_test_id="12__diagjson_wave39_macro_trace_identity_control",
    ),
]
