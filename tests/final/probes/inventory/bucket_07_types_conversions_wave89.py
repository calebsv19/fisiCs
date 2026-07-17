from pathlib import Path

from lib.models import DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="07__probe_wave89_assignment_known_scalar_child_error_strict",
        source=PROBE_DIR / "diagnostics/07__probe_wave89_assignment_known_scalar_child_error_strict.c",
        note=(
            "wave89 strict: an earlier comma-expression child error must not hide "
            "the independent pointer-from-double assignment incompatibility when "
            "the RHS final type remains known"
        ),
        required_substrings=[
            "Undeclared identifier",
            "wave89_missing_scalar",
            "Incompatible assignment operands",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="07__wave89_assignment_known_scalar_child_error_strict",
    ),
    DiagnosticProbe(
        probe_id="07__probe_wave89_assignment_known_aggregate_child_error_strict",
        source=PROBE_DIR / "diagnostics/07__probe_wave89_assignment_known_aggregate_child_error_strict.c",
        note=(
            "wave89 strict: an earlier comma-expression child error must not hide "
            "the independent incompatible-aggregate assignment when the RHS final "
            "aggregate type remains known"
        ),
        required_substrings=[
            "Undeclared identifier",
            "wave89_missing_aggregate",
            "Incompatible assignment operands",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="07__wave89_assignment_known_aggregate_child_error_strict",
    ),
    DiagnosticProbe(
        probe_id="07__probe_wave89_assignment_unknown_child_error_control",
        source=PROBE_DIR / "diagnostics/07__probe_wave89_assignment_unknown_child_error_control.c",
        note=(
            "wave89 control: a direct undeclared RHS has unknown type, so only the "
            "primary child error remains and the dependent assignment cascade stays "
            "suppressed"
        ),
        required_substrings=["Undeclared identifier", "wave89_missing_unknown"],
        forbidden_substrings=["Incompatible assignment operands"],
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="07__wave89_assignment_unknown_child_error_control",
    ),
]

DIAG_JSON_PROBES = []
