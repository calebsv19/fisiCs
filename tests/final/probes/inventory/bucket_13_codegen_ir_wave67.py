from pathlib import Path

from lib.models import DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
CASE_DIR = PROBE_DIR.parent / "cases"

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="13__probe_wave67_macro_include_compound_strict",
        source=CASE_DIR / "13__wave67_macro_include_compound_no_tail_lowering.c",
        note=(
            "wave67 strict Clang oracle: an undeclared include-origin macro leaf "
            "used by compound assignment must emit only the primary error and "
            "suppress uniquely named LLVM tail symbols"
        ),
        required_substrings=[
            "Undeclared identifier",
            "wave67_missing_value",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "Operator '+' requires arithmetic operands",
            "@wave67_tail_global = global",
            "define i32 @wave67_tail_function",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__wave67_macro_include_compound_strict",
    ),
]

DIAG_JSON_PROBES = []
