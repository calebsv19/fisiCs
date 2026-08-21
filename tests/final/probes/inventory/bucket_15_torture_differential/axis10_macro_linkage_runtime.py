from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis10_wave1_runtime_clang_gcc16_tri_diff_multitu_xmacro_linkage_seed_fold",
        source=PROBE_DIR / "runtime/15__probe_axis10_wave1_runtime_clang_gcc16_tri_diff_multitu_xmacro_linkage_seed_fold_main.c",
        note=(
            "axis10 wave1: defined C99 X-macro-generated enum/prototype linkage and "
            "seeded dispatch across translation units must match clang and GNU GCC 16"
        ),
        inputs=[
            PROBE_DIR / "runtime/15__probe_axis10_wave1_runtime_clang_gcc16_tri_diff_multitu_xmacro_linkage_seed_fold_main.c",
            PROBE_DIR / "runtime/15__probe_axis10_wave1_runtime_clang_gcc16_tri_diff_multitu_xmacro_linkage_seed_fold_lib.c",
        ],
        extra_differential_compiler="/opt/homebrew/opt/gcc/bin/gcc-16",
        expected_exit_code=0,
        expected_stdout="axis10-xmacro=3875489293,14177,25536\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis10_wave1_multitu_xmacro_linkage_seed_fold",
    ),
]
