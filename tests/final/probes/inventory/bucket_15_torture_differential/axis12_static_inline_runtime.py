from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_axis12_wave1_runtime_clang_gcc16_tri_diff_multitu_static_inline_local_state",
        source=PROBE_DIR / "runtime/15__probe_axis12_wave1_runtime_clang_gcc16_tri_diff_multitu_static_inline_local_state_main.c",
        note=(
            "axis12 wave1: defined C99 static-inline function-local state must remain "
            "translation-unit-local and match clang and GNU GCC 16"
        ),
        inputs=[
            PROBE_DIR / "runtime/15__probe_axis12_wave1_runtime_clang_gcc16_tri_diff_multitu_static_inline_local_state_main.c",
            PROBE_DIR / "runtime/15__probe_axis12_wave1_runtime_clang_gcc16_tri_diff_multitu_static_inline_local_state_lib.c",
        ],
        extra_differential_compiler="/opt/homebrew/opt/gcc/bin/gcc-16",
        expected_exit_code=0,
        expected_stdout="axis12-inline=7,576,76,51942\n",
        expected_stderr="",
        promoted_test_id="15__runtime_axis12_wave1_multitu_static_inline_local_state",
    ),
]
