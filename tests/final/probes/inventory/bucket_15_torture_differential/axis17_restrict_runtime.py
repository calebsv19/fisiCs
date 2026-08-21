from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis17_wave1_runtime_clang_gcc16_tri_diff_restrict_transform",
        source=PROBE_DIR / "runtime/15__probe_axis17_wave1_runtime_clang_gcc16_tri_diff_restrict_transform.c",
        note=(
            "axis17 wave1: defined C99 restrict-qualified non-overlapping transforms "
            "must preserve the same result for clang and GNU GCC 16"
        ),
        extra_differential_compiler="gcc",
        expected_exit_code=0,
        expected_stdout="axis17-restrict=2915246976,722\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis17_wave1_restrict_transform",
    ),
]
