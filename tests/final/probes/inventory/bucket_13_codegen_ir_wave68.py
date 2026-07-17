from pathlib import Path

from lib.models import DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
CASE_DIR = PROBE_DIR.parent / "cases"

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="13__probe_wave68_undeclared_compound_target_strict",
        source=CASE_DIR / "13__wave68_undeclared_compound_target_no_tail_lowering.c",
        note=(
            "wave68 strict Clang oracle: an undeclared compound-assignment "
            "target emits only its primary error and suppresses tail IR"
        ),
        required_substrings=[
            "Undeclared identifier",
            "wave68_missing_target",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "Left operand of '+=' must be a modifiable lvalue",
            "Operator '+' requires arithmetic operands",
            "@wave68_target_tail_global = global",
            "define i32 @wave68_target_tail_function",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__wave68_undeclared_compound_target_strict",
    ),
    DiagnosticProbe(
        probe_id="13__probe_wave68_known_rhs_error_strict",
        source=CASE_DIR / "13__wave68_known_rhs_error_no_tail_lowering.c",
        note=(
            "wave68 strict Clang oracle: a known-final-type compound RHS with "
            "an undeclared child emits only the child error and suppresses tail IR"
        ),
        required_substrings=[
            "Undeclared identifier",
            "wave68_missing_leaf",
            "Skipping LLVM code generation due to semantic errors.",
        ],
        forbidden_substrings=[
            "Invalid operands to compound assignment",
            "Operator '+' requires arithmetic operands",
            "Operator '+' requires pointer arithmetic",
            "@wave68_rhs_tail_global = global",
            "define i32 @wave68_rhs_tail_function",
            "Failed to generate",
            "NULL node",
            "LLVM ERROR",
        ],
        fisics_args=["--dump-ir"],
        allowed_exit_codes=(1,),
        promoted_test_id="13__wave68_known_rhs_error_strict",
    ),
]

DIAG_JSON_PROBES = []
