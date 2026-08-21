from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis20_wave1_runtime_clang_gcc16_tri_diff_multitu_static_storage_initialization",
        source=PROBE_DIR / "runtime/15__probe_axis20_wave1_runtime_clang_gcc16_tri_diff_multitu_static_storage_initialization_main.c",
        note=(
            "axis20 wave1: defined C99 file-scope static storage zero and explicit "
            "initialization across translation units must match clang and GNU GCC 16"
        ),
        inputs=[
            PROBE_DIR / "runtime/15__probe_axis20_wave1_runtime_clang_gcc16_tri_diff_multitu_static_storage_initialization_main.c",
            PROBE_DIR / "runtime/15__probe_axis20_wave1_runtime_clang_gcc16_tri_diff_multitu_static_storage_initialization_lib.c",
        ],
        extra_differential_compiler="gcc",
        expected_exit_code=0,
        expected_stdout="axis20-storage=0,495254,4850\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis20_wave1_multitu_static_storage_initialization",
    ),
]
