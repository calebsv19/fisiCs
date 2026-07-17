from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
STRICT_CLANG_ARGS = ["-pedantic-errors", "-Wall", "-Wextra"]

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="11__probe_wave91_typedef_member_array_indirect_all_fisics",
        source=PROBE_DIR / "runtime/11__probe_wave91_typedef_member_array_indirect_main.c",
        inputs=[
            PROBE_DIR / "runtime/11__probe_wave91_typedef_member_array_indirect_main.c",
            PROBE_DIR / "runtime/11__probe_wave91_typedef_member_array_indirect_explicit_lib.c",
        ],
        note="wave91 strict: an adjusted array parameter inside a function-type typedef remains callable after pointer and array typedef wrapping as a struct member, including a large aggregate return",
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="11__runtime_wave91_typedef_member_array_indirect_all_fisics",
    ),
    RuntimeProbe(
        probe_id="11__probe_wave91_typedef_member_array_indirect_mixed_clang_callee",
        source=PROBE_DIR / "runtime/11__probe_wave91_typedef_member_array_indirect_main.c",
        inputs=[
            PROBE_DIR / "runtime/11__probe_wave91_typedef_member_array_indirect_main.c",
        ],
        mixed_clang_inputs=[
            PROBE_DIR / "runtime/11__probe_wave91_typedef_member_array_indirect_explicit_lib.c",
        ],
        note="wave91 mixed oracle: a fisiCs-built caller invokes clang-built explicit-pointer callees obtained from a typedef-array struct member across the adjusted-array and large-return ABI boundaries",
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="11__runtime_wave91_typedef_member_array_indirect_mixed_clang_callee",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
