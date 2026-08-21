from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis15_wave1_runtime_clang_gcc16_tri_diff_multitu_flexible_array_packet",
        source=PROBE_DIR / "runtime/15__probe_axis15_wave1_runtime_clang_gcc16_tri_diff_multitu_flexible_array_packet_main.c",
        note=(
            "axis15 wave1: defined C99 flexible-array-member allocation and multi-TU "
            "payload access must match clang and GNU GCC 16"
        ),
        inputs=[
            PROBE_DIR / "runtime/15__probe_axis15_wave1_runtime_clang_gcc16_tri_diff_multitu_flexible_array_packet_main.c",
            PROBE_DIR / "runtime/15__probe_axis15_wave1_runtime_clang_gcc16_tri_diff_multitu_flexible_array_packet_lib.c",
        ],
        extra_differential_compiler="gcc",
        expected_exit_code=0,
        expected_stdout="axis15-fam=3431596577,3374989480\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis15_wave1_multitu_flexible_array_packet",
    ),
]
