from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
SOURCE = PROBE_DIR / "runtime/13__probe_wave57_if_else_recovery_aggregate_copy_return_runtime.c"
RUNTIME_TEST_ID = "13__runtime_wave57_if_else_recovery_aggregate_copy_return"
DIAG_TEST_ID = "13__diag_wave57_if_else_recovery_exact_exit"


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="13__probe_wave57_if_else_recovery_aggregate_copy_return_runtime",
        source=SOURCE,
        note=(
            "wave57 recovery/lowering boundary: malformed if/else recovery must "
            "continue through a valid aggregate copy/return suffix and match the "
            "clean clang control at runtime"
        ),
        fisics_args=["-DWAVE57_MALFORMED=0"],
        clang_args=["-DWAVE57_MALFORMED=0"],
        promoted_test_id=RUNTIME_TEST_ID,
    )
]


DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="13__probe_wave57_if_else_recovery_codegen_markers_diag",
        source=SOURCE,
        note=(
            "wave57 recovery/lowering boundary: retain the exact malformed-if "
            "parser diagnostic while forbidding internal codegen error leakage"
        ),
        expect_any_diagnostic=True,
        required_substrings=[
            "Error: Unexpected token at start of expression",
            "(got ')')",
        ],
        forbidden_substrings=[
            "NULL node in codegen",
            "Failed to generate condition",
        ],
        fisics_args=["-DWAVE57_MALFORMED=1", "-o", "/dev/null"],
        allowed_exit_codes=(1,),
        promoted_test_id=DIAG_TEST_ID,
    )
]


DIAG_JSON_PROBES = []
