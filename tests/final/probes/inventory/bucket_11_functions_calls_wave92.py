from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
STRICT_CLANG_ARGS = ["-pedantic-errors", "-Wall", "-Wextra"]

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="11__probe_wave92_typedef_payload_table_all_fisics",
        source=PROBE_DIR / "runtime/11__probe_wave92_typedef_payload_table_reverse_main.c",
        inputs=[
            PROBE_DIR / "runtime/11__probe_wave92_typedef_payload_table_reverse_main.c",
            PROBE_DIR / "runtime/11__probe_wave92_typedef_payload_table_reverse_lib.c",
        ],
        note="wave92 control: a pointer to a typedef-defined function-pointer table remains callable when each callback returns a large payload containing a typedef-defined array member",
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="11__runtime_wave92_typedef_payload_table_all_fisics",
    ),
    RuntimeProbe(
        probe_id="11__probe_wave92_typedef_payload_table_reverse_clang_caller",
        source=PROBE_DIR / "runtime/11__probe_wave92_typedef_payload_table_reverse_lib.c",
        inputs=[
            PROBE_DIR / "runtime/11__probe_wave92_typedef_payload_table_reverse_lib.c",
        ],
        mixed_clang_inputs=[
            PROBE_DIR / "runtime/11__probe_wave92_typedef_payload_table_reverse_main.c",
        ],
        note="wave92 reverse mixed oracle: a clang caller dereferences a fisiCs-exported pointer to a typedef-defined callback table and receives large typedef-array-member payloads from fisiCs callbacks",
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="11__runtime_wave92_typedef_payload_table_reverse_clang_caller",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
