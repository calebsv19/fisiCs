from pathlib import Path

from lib.models import DiagnosticExpectation, DiagnosticJsonProbe, DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
SOURCE = PROBE_DIR / "diagnostics/12__probe_wave42_sizeof_type_missing_rparen_then_semantic_followup.c"

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="12__probe_wave42_sizeof_type_missing_rparen_clang_parity_strict",
        source=SOURCE,
        note=(
            "wave42 strict: malformed sizeof(type) should report one primary missing-rparen "
            "diagnostic at the remapped semicolon before preserving the independent semantic tail"
        ),
        required_substrings=[
            "Expected ')' after sizeof(type)",
            "virtual_wave42_sizeof_type_missing_rparen.c:14243",
            "Undeclared identifier",
            "wave42_tail_missing",
        ],
        forbidden_substrings=[
            "expected ';' after expression",
            "invalid statement inside block",
        ],
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave42_sizeof_type_missing_rparen_strict",
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="12__probe_wave42_sizeof_type_missing_rparen_diagjson_strict",
        source=SOURCE,
        note=(
            "wave42 strict JSON: the primary parser record belongs at the remapped semicolon, "
            "followed by the independent semantic tail"
        ),
        expected_diagnostics=(
            DiagnosticExpectation(
                code=1000,
                line=14243,
                column=29,
                has_file=True,
                severity="error",
                stage="parse",
            ),
            DiagnosticExpectation(
                code=2000,
                line=14244,
                column=12,
                has_file=True,
                severity="error",
                stage="semantic",
            ),
        ),
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave42_sizeof_type_missing_rparen_strict",
    ),
]
