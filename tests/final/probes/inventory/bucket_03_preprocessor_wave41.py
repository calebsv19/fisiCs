from pathlib import Path

from lib.models import DiagnosticExpectation, DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="03__probe_wave41_variadic_paste_clean_control",
        source=PROBE_DIR / "runtime/03__probe_wave41_variadic_paste_clean_control.c",
        note=(
            "wave41 clean control: a standard C99 variadic argument forwarded through "
            "two-level token paste must form and evaluate the intended identifier"
        ),
        promoted_test_id="03__runtime_wave41_variadic_paste_clean_control",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="03__probe_wave41_variadic_paste_argument_spelling_strict",
        source=PROBE_DIR / "diagnostics/03__probe_wave41_variadic_paste_argument_spelling_strict.c",
        note=(
            "wave41 strict: an undeclared identifier formed from a named prefix and a "
            "variadic argument through two-level paste must retain Clang call-site "
            "provenance rather than doubling the remapped line"
        ),
        required_substrings=[
            "virtual_wave41_variadic_paste.c:4106:11",
            "Undeclared identifier",
            "missing_name",
        ],
        promoted_test_id="03__diag_wave41_variadic_paste_argument_spelling_strict",
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="03__probe_diagjson_wave41_variadic_paste_argument_spelling_strict",
        source=PROBE_DIR / "diagnostics/03__probe_wave41_variadic_paste_argument_spelling_strict.c",
        note=(
            "wave41 strict JSON: the pasted semantic token must retain the Clang "
            "primary call-site line and column after #line remapping"
        ),
        expected_diagnostics=(
            DiagnosticExpectation(
                code=2000,
                line=4106,
                column=11,
                has_file=True,
                severity="error",
                stage="semantic",
            ),
        ),
    ),
]
