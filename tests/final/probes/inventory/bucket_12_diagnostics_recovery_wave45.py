from pathlib import Path

from lib.models import DiagnosticExpectation, DiagnosticJsonProbe, DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
ALPHA = PROBE_DIR / "diagnostics/12__probe_wave45_dual_bad_alpha.c"
BETA = PROBE_DIR / "diagnostics/12__probe_wave45_dual_bad_beta.c"

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="12__probe_wave45_dual_bad_alpha_then_beta_text_strict",
        source=ALPHA,
        inputs=[ALPHA, BETA],
        note=(
            "wave45 strict: two independently failing translation units must both "
            "emit their remapped semantic diagnostic when alpha is compiled first"
        ),
        required_substrings=[
            "virtual_wave45_dual_bad_alpha.c:14512:12",
            "wave45_alpha_missing",
            "virtual_wave45_dual_bad_beta.c:14532:12",
            "wave45_beta_missing",
        ],
        forbidden_substrings=["[link]", "<unknown>:0"],
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave45_dual_bad_alpha_then_beta_strict",
    ),
    DiagnosticProbe(
        probe_id="12__probe_wave45_dual_bad_beta_then_alpha_text_strict",
        source=BETA,
        inputs=[BETA, ALPHA],
        note=(
            "wave45 strict order oracle: reversing the failing translation units must "
            "retain both remapped semantic diagnostics"
        ),
        required_substrings=[
            "virtual_wave45_dual_bad_beta.c:14532:12",
            "wave45_beta_missing",
            "virtual_wave45_dual_bad_alpha.c:14512:12",
            "wave45_alpha_missing",
        ],
        forbidden_substrings=["[link]", "<unknown>:0"],
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave45_dual_bad_beta_then_alpha_strict",
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="12__probe_wave45_dual_bad_alpha_then_beta_json_strict",
        source=ALPHA,
        inputs=[ALPHA, BETA],
        note="wave45 strict JSON: aggregate export must contain both TUs in alpha-then-beta order",
        expected_diagnostics=(
            DiagnosticExpectation(code=2000, line=14512, column=12, has_file=True, severity="error", stage="semantic"),
            DiagnosticExpectation(code=2000, line=14532, column=12, has_file=True, severity="error", stage="semantic"),
        ),
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave45_dual_bad_alpha_then_beta_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_wave45_dual_bad_beta_then_alpha_json_strict",
        source=BETA,
        inputs=[BETA, ALPHA],
        note="wave45 strict JSON order oracle: aggregate export must retain both reversed TUs",
        expected_diagnostics=(
            DiagnosticExpectation(code=2000, line=14532, column=12, has_file=True, severity="error", stage="semantic"),
            DiagnosticExpectation(code=2000, line=14512, column=12, has_file=True, severity="error", stage="semantic"),
        ),
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave45_dual_bad_beta_then_alpha_strict",
    ),
]
