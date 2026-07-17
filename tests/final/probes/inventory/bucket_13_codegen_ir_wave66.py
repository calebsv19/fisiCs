from pathlib import Path

from lib.models import DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
CASE_DIR = PROBE_DIR.parent / "cases"

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="13__probe_wave66_macro_include_conditional_strict",
        source=CASE_DIR / "13__wave66_macro_include_conditional_no_tail_lowering.c",
        note=(
            "wave66 strict Clang oracle: an undeclared macro leaf inside a "
            "conditional assignment tail must emit only the primary error "
            "and suppress uniquely named LLVM tail symbols"
        ),
        required_substrings=[
            "Undeclared identifier",
            "wave66_missing_value",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "Incompatible assignment operands",
            "@wave66_tail_global = global",
            "define i32 @wave66_tail_function",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__wave66_macro_include_conditional_strict",
    ),
    DiagnosticProbe(
        probe_id="13__probe_wave66_macro_include_conditional_current_threshold",
        source=CASE_DIR / "13__wave66_macro_include_conditional_no_tail_lowering.c",
        note=(
            "wave66 resolved threshold: suppress the dependent assignment cascade "
            "while still failing closed before LLVM tail emission"
        ),
        required_substrings=[
            "Undeclared identifier",
            "wave66_missing_value",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "@wave66_tail_global = global",
            "define i32 @wave66_tail_function",
            "Incompatible assignment operands",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__wave66_macro_include_conditional_current_threshold",
    ),
]

DIAG_JSON_PROBES = []
