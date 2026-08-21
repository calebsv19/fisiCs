from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis14_wave1_runtime_clang_gcc16_tri_diff_vla_sizeof_evaluation",
        source=PROBE_DIR / "runtime/15__probe_axis14_wave1_runtime_clang_gcc16_tri_diff_vla_sizeof_evaluation.c",
        note=(
            "axis14 wave1: defined C99 VLA bound evaluation inside sizeof must occur "
            "at runtime and match clang and GNU GCC 16"
        ),
        extra_differential_compiler="gcc",
        expected_exit_code=0,
        expected_stdout="axis14-vla=53470,64623\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis14_wave1_vla_sizeof_evaluation",
    ),
]
