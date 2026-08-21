from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis13_wave1_runtime_clang_gcc16_tri_diff_va_copy_dual_fold",
        source=PROBE_DIR / "runtime/15__probe_axis13_wave1_runtime_clang_gcc16_tri_diff_va_copy_dual_fold.c",
        note=(
            "axis13 wave1: defined C99 va_copy traversal must preserve an independent "
            "variadic argument cursor and match clang and GNU GCC 16"
        ),
        extra_differential_compiler="gcc",
        expected_exit_code=0,
        expected_stdout="axis13-vacopy=20425433,669048751\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis13_wave1_va_copy_dual_fold",
    ),
]
