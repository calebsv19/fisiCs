from pathlib import Path

from lib.models import DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
CASE_DIR = PROBE_DIR.parent / "cases"

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="13__probe_wave64_else_if_incomplete_binary_no_tail_lowering",
        source=CASE_DIR / "13__probe_wave64_else_if_incomplete_binary_no_tail_lowering.c",
        note="wave64 strict: an incomplete binary condition in an else-if arm must remain a parser and semantic hard error and suppress all LLVM tail lowering",
        required_substrings=[
            "Unexpected token at start of expression",
            "Operator '+' requires arithmetic operands",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "@wave64_else_if_tail_global = global",
            "define i32 @wave64_else_if_tail_function",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__probe_wave64_else_if_incomplete_binary_no_tail_lowering",
    ),
    DiagnosticProbe(
        probe_id="13__probe_wave64_double_else_no_tail_lowering",
        source=CASE_DIR / "13__probe_wave64_double_else_no_tail_lowering.c",
        note="wave64 strict: a duplicate else token must remain a parser hard error and suppress all LLVM tail lowering",
        required_substrings=[
            "Unexpected token at start of expression",
            "Invalid body in 'else' statement",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "@wave64_double_else_tail_global = global",
            "define i32 @wave64_double_else_tail_function",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__probe_wave64_double_else_no_tail_lowering",
    ),
]

DIAG_JSON_PROBES = []
