from pathlib import Path

from lib.models import DiagnosticExpectation, DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="07__probe_wave87_line_directive_fnptr_aggregate_return_initializer_strict",
        source=PROBE_DIR / "diagnostics/07__probe_diagjson_wave87_line_directive_fnptr_aggregate_return_initializer_strict.c",
        note="wave87 strict: an aggregate-return function pointer cannot be initialized from a scalar-return function under exact #line provenance",
        expected_diagnostics=(
            DiagnosticExpectation(
                code=2000,
                line=19710,
                column=37,
                has_file=True,
                severity="error",
                stage="semantic",
            ),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="07__wave87_line_directive_fnptr_aggregate_return_initializer_diagjson_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_wave87_line_directive_fnptr_aggregate_return_assignment_reverse_strict",
        source=PROBE_DIR / "diagnostics/07__probe_diagjson_wave87_line_directive_fnptr_aggregate_return_assignment_reverse_strict.c",
        note="wave87 strict: a scalar-return function pointer cannot be assigned an aggregate-return function in the reverse direction under exact #line provenance",
        expected_diagnostics=(
            DiagnosticExpectation(
                code=2000,
                line=19811,
                column=5,
                has_file=True,
                severity="error",
                stage="semantic",
            ),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="07__wave87_line_directive_fnptr_aggregate_return_assignment_reverse_diagjson_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_wave87_line_directive_include_fnptr_aggregate_return_initializer_strict",
        source=PROBE_DIR / "diagnostics/07__probe_diagjson_wave87_line_directive_include_fnptr_aggregate_return_initializer_strict.c",
        note="wave87 strict: include-header aggregate-return versus scalar-return function-pointer initialization preserves remapped diagnostic identity",
        expected_diagnostics=(
            DiagnosticExpectation(
                code=2000,
                line=19910,
                column=38,
                has_file=True,
                severity="error",
                stage="semantic",
            ),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="07__wave87_line_directive_include_fnptr_aggregate_return_initializer_diagjson_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_wave87_line_directive_include_fnptr_aggregate_return_assignment_reverse_strict",
        source=PROBE_DIR / "diagnostics/07__probe_diagjson_wave87_line_directive_include_fnptr_aggregate_return_assignment_reverse_strict.c",
        note="wave87 strict: include-header reverse aggregate-return versus scalar-return function-pointer assignment preserves remapped diagnostic identity",
        expected_diagnostics=(
            DiagnosticExpectation(
                code=2000,
                line=20011,
                column=5,
                has_file=True,
                severity="error",
                stage="semantic",
            ),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="07__wave87_line_directive_include_fnptr_aggregate_return_assignment_reverse_diagjson_strict",
    ),
]
