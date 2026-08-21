from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis11_wave1_runtime_clang_gcc16_tri_diff_multitu_uint64_rotate_packet",
        source=PROBE_DIR / "runtime/15__probe_axis11_wave1_runtime_clang_gcc16_tri_diff_multitu_uint64_rotate_packet_main.c",
        note=(
            "axis11 wave1: defined C99 fixed-width unsigned rotation and aggregate "
            "return across translation units must match clang and GNU GCC 16"
        ),
        inputs=[
            PROBE_DIR / "runtime/15__probe_axis11_wave1_runtime_clang_gcc16_tri_diff_multitu_uint64_rotate_packet_main.c",
            PROBE_DIR / "runtime/15__probe_axis11_wave1_runtime_clang_gcc16_tri_diff_multitu_uint64_rotate_packet_lib.c",
        ],
        extra_differential_compiler="/opt/homebrew/opt/gcc/bin/gcc-16",
        expected_exit_code=0,
        expected_stdout="axis11-u64=18409338513176342578,1823430409\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis11_wave1_multitu_uint64_rotate_packet",
    ),
]
