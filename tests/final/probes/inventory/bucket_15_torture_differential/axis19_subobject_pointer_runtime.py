from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis19_wave1_runtime_clang_gcc16_tri_diff_multitu_subobject_pointer_walk",
        source=PROBE_DIR / "runtime/15__probe_axis19_wave1_runtime_clang_gcc16_tri_diff_multitu_subobject_pointer_walk_main.c",
        note=(
            "axis19 wave1: defined C99 nested-array subobject pointer traversal, "
            "without cross-subarray pointer arithmetic, must match clang and GNU GCC 16"
        ),
        inputs=[
            PROBE_DIR / "runtime/15__probe_axis19_wave1_runtime_clang_gcc16_tri_diff_multitu_subobject_pointer_walk_main.c",
            PROBE_DIR / "runtime/15__probe_axis19_wave1_runtime_clang_gcc16_tri_diff_multitu_subobject_pointer_walk_lib.c",
        ],
        extra_differential_compiler="gcc",
        expected_exit_code=0,
        expected_stdout="axis19-subobject=2580014508,3519263940\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis19_wave1_multitu_subobject_pointer_walk",
    ),
]
