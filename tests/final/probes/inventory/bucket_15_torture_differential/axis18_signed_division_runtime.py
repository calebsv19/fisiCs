from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis18_wave1_runtime_clang_gcc16_tri_diff_signed_division_remainder",
        source=PROBE_DIR / "runtime/15__probe_axis18_wave1_runtime_clang_gcc16_tri_diff_signed_division_remainder.c",
        note=(
            "axis18 wave1: defined C99 signed division/remainder truncation cases, "
            "excluding division-by-zero and INT_MIN/-1, must match clang and GNU GCC 16"
        ),
        extra_differential_compiler="gcc",
        expected_exit_code=0,
        expected_stdout="axis18-div=6,933194\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis18_wave1_signed_division_remainder",
    ),
]
