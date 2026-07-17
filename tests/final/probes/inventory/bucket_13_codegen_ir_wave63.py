from pathlib import Path

from lib.models import DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
CASE_DIR = PROBE_DIR.parent / "cases"

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="13__probe_wave63_for_initializer_incomplete_binary_no_tail_lowering",
        source=CASE_DIR / "13__probe_wave63_for_initializer_incomplete_binary_no_tail_lowering.c",
        note="wave63 strict: an incomplete binary expression in the for initializer must become a semantic hard error and suppress all LLVM tail lowering",
        required_substrings=[
            "Unexpected token at start of expression",
            "Operator '+' requires arithmetic operands",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "@wave63_initializer_tail_global = global",
            "define i32 @wave63_initializer_tail_function",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__probe_wave63_for_initializer_incomplete_binary_no_tail_lowering",
    ),
    DiagnosticProbe(
        probe_id="13__probe_wave63_for_condition_incomplete_binary_no_tail_lowering",
        source=CASE_DIR / "13__probe_wave63_for_condition_incomplete_binary_no_tail_lowering.c",
        note="wave63 strict: an incomplete binary expression in the for condition must become a semantic hard error and suppress all LLVM tail lowering",
        required_substrings=[
            "Unexpected token at start of expression",
            "Operator '+' requires arithmetic operands",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "@wave63_condition_tail_global = global",
            "define i32 @wave63_condition_tail_function",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__probe_wave63_for_condition_incomplete_binary_no_tail_lowering",
    ),
    DiagnosticProbe(
        probe_id="13__probe_wave63_for_increment_incomplete_binary_no_tail_lowering",
        source=CASE_DIR / "13__probe_wave63_for_increment_incomplete_binary_no_tail_lowering.c",
        note="wave63 strict: an incomplete binary expression in the for increment must become a semantic hard error and suppress all LLVM tail lowering",
        required_substrings=[
            "Unexpected token at start of expression",
            "Operator '+' requires arithmetic operands",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "@wave63_tail_global = global",
            "define i32 @wave63_tail_function",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__probe_wave63_for_increment_incomplete_binary_no_tail_lowering",
    ),
]

DIAG_JSON_PROBES = []
