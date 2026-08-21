from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis16_wave1_runtime_clang_gcc16_tri_diff_multitu_volatile_access_order",
        source=PROBE_DIR / "runtime/15__probe_axis16_wave1_runtime_clang_gcc16_tri_diff_multitu_volatile_access_order_main.c",
        note=(
            "axis16 wave1: defined C99 volatile reads and writes across translation "
            "units must retain the sequenced access transcript for clang and GNU GCC 16"
        ),
        inputs=[
            PROBE_DIR / "runtime/15__probe_axis16_wave1_runtime_clang_gcc16_tri_diff_multitu_volatile_access_order_main.c",
            PROBE_DIR / "runtime/15__probe_axis16_wave1_runtime_clang_gcc16_tri_diff_multitu_volatile_access_order_lib.c",
        ],
        extra_differential_compiler="gcc",
        expected_exit_code=0,
        expected_stdout="axis16-volatile=11519,114\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis16_wave1_multitu_volatile_access_order",
    ),
]
