from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis8_wave1_runtime_clang_gcc_tri_diff_compound_literal_lifetime_copy_matrix",
        source=PROBE_DIR / "runtime/15__probe_axis8_wave1_runtime_clang_gcc_tri_diff_compound_literal_lifetime_copy_matrix.c",
        note=(
            "axis8 wave1: defined C99 automatic compound-literal object identity, "
            "designated initialization, and aggregate-value-copy matrix must match "
            "both clang and Homebrew GNU GCC 16"
        ),
        extra_differential_compiler="/opt/homebrew/opt/gcc/bin/gcc-16",
        expected_exit_code=0,
        expected_stdout="axis8-compound=385517\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis8_wave1_compound_literal_lifetime_copy_matrix",
    ),
]
