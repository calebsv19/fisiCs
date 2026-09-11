from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_edu69_static_output_bridge_clang_gcc16_tri_diff",
        source=PROBE_DIR / "runtime/15__probe_edu69_static_output_bridge_main.c",
        note=(
            "EDU-69 reduction: a file-scope static output buffer passed to an "
            "external multi-TU sink must preserve defined C99 address and byte "
            "semantics across fisiCs, Clang, and GNU GCC 16"
        ),
        inputs=[
            PROBE_DIR / "runtime/15__probe_edu69_static_output_bridge_main.c",
            PROBE_DIR / "runtime/15__probe_edu69_static_output_bridge_lib.c",
        ],
        extra_differential_compiler="gcc",
        expected_exit_code=0,
        expected_stdout="edu69-static-output=676512184622409157,14581518604128775621\n",
        expected_stderr="",
        promoted_test_id="15__runtime_edu69_static_output_bridge",
    ),
]
