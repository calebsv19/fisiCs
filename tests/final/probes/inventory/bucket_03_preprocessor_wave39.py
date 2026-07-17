from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="03__probe_wave39_pragma_direct_following_token_strict",
        source=PROBE_DIR / "runtime/03__probe_wave39_pragma_direct_following_token_strict.c",
        note=(
            "wave39 strict: direct C99 _Pragma destringization must consume only "
            "the operator and preserve the following declaration tokens"
        ),
        promoted_test_id="03__runtime_wave39_pragma_direct_following_token_strict",
    ),
    RuntimeProbe(
        probe_id="03__probe_wave39_pragma_macro_nested_following_token_strict",
        source=PROBE_DIR / "runtime/03__probe_wave39_pragma_macro_nested_following_token_strict.c",
        note=(
            "wave39 strict: nested macro argument expansion must produce a valid "
            "_Pragma operand, destringize it, and preserve the following function"
        ),
        promoted_test_id="03__runtime_wave39_pragma_macro_nested_following_token_strict",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="03__probe_wave39_pragma_malformed_operand_reject",
        source=PROBE_DIR / "diagnostics/03__probe_wave39_pragma_malformed_operand_reject.c",
        note=(
            "wave39 current-threshold control: a non-string _Pragma operand must "
            "be rejected with a diagnostic anchored to source line 1"
        ),
        required_substrings=["line 1"],
        promoted_test_id="03__diag_wave39_pragma_malformed_operand_reject",
    ),
]

DIAG_JSON_PROBES = []
