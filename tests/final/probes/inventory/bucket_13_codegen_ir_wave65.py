from pathlib import Path

from lib.models import DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
CASE_DIR = PROBE_DIR.parent / "cases"

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="13__probe_wave65_macro_include_undeclared_arithmetic_strict",
        source=CASE_DIR / "13__wave65_macro_include_undeclared_arithmetic_no_tail_lowering.c",
        note=(
            "wave65 strict Clang oracle: an undeclared identifier carried through an "
            "include-defined nested arithmetic macro must emit only the primary "
            "undeclared-identifier error and suppress uniquely named tail lowering"
        ),
        required_substrings=[
            "Undeclared identifier",
            "wave65_missing_operand",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "Operator '+' requires arithmetic operands",
            "@wave65_tail_global = global",
            "define i32 @wave65_tail_function",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__wave65_macro_include_undeclared_arithmetic_strict",
    ),
    DiagnosticProbe(
        probe_id="13__probe_wave65_macro_include_undeclared_arithmetic_current_threshold",
        source=CASE_DIR / "13__wave65_macro_include_undeclared_arithmetic_no_tail_lowering.c",
        note=(
            "wave65 promoted regression: preserve the primary macro-routed error, "
            "suppress the dependent arithmetic cascade, and keep LLVM tail lowering "
            "fail-closed"
        ),
        required_substrings=[
            "Undeclared identifier",
            "wave65_missing_operand",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "Operator '+' requires arithmetic operands",
            "@wave65_tail_global = global",
            "define i32 @wave65_tail_function",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__wave65_macro_include_undeclared_arithmetic_current_threshold",
    ),
]

DIAG_JSON_PROBES = []
