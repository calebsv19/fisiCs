from pathlib import Path

from lib.models import DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="13__probe_wave62_hard_error_no_tail_lowering",
        source=PROBE_DIR.parent / "cases/13__probe_wave62_hard_error_no_tail_lowering.c",
        note="wave62 strict: a recovered parser error followed by an independent semantic hard error must stop LLVM lowering before uniquely named tail symbols are emitted",
        required_substrings=[
            "Unexpected token at start of expression",
            "Undeclared identifier",
            "wave62_missing_symbol",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__probe_wave62_hard_error_no_tail_lowering",
    ),
]
DIAG_JSON_PROBES = []
