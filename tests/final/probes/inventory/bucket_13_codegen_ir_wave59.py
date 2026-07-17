from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
SOURCE = PROBE_DIR / "runtime/13__probe_wave59_include_line_parser_recovery_runtime.c"

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="13__probe_wave59_include_line_parser_recovery_runtime",
        source=SOURCE,
        note=(
            "wave59 clean include control: a #line-remapped header feeds a valid "
            "downstream aggregate return/copy path that matches the reference runtime"
        ),
        promoted_test_id="13__runtime_wave59_include_line_parser_recovery",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="13__probe_wave59_include_line_parser_recovery_diagnostics",
        source=SOURCE,
        note=(
            "wave59 include recovery diagnostics must preserve exact virtual provenance "
            "without leaking internal backend recovery failures"
        ),
        required_substrings=[
            "virtual_wave59_include_parser_recovery.h:159003",
        ],
        fisics_args=["-DWAVE59_MALFORMED=1"],
        forbidden_substrings=[
            "NULL node in codegen",
            "Failed to generate condition",
            "Internal compiler error",
            "Segmentation fault",
        ],
        allowed_exit_codes=(1,),
        promoted_test_id="13__diag_wave59_include_line_parser_recovery_exact_exit",
    ),
]

DIAG_JSON_PROBES = []
