from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent
STRICT_CLANG_ARGS = ["-std=c99", "-pedantic-errors", "-Wall", "-Wextra"]

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="14__probe_runtime_wave354_file_scope_compound_identity_all_fisics",
        source=PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_main.c",
        inputs=[
            PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_main.c",
            PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_owner.c",
            PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_observer.c",
        ],
        note=(
            "wave354 strict: an exported pointer initialized from a file-scope compound "
            "literal must retain one stable object identity and expose alias mutations "
            "across three translation units"
        ),
        clang_args=STRICT_CLANG_ARGS,
    ),
    RuntimeProbe(
        probe_id="14__probe_runtime_wave354_file_scope_compound_identity_reverse_order",
        source=PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_main.c",
        inputs=[
            PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_observer.c",
            PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_owner.c",
            PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_main.c",
        ],
        note=(
            "wave354 order oracle: reversing the three translation-unit compile/link order "
            "must preserve the exported compound-literal object's identity and mutations"
        ),
        clang_args=STRICT_CLANG_ARGS,
    ),
    RuntimeProbe(
        probe_id="14__probe_runtime_wave354_file_scope_compound_identity_reverse_clang_consumers",
        source=PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_owner.c",
        inputs=[
            PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_owner.c",
        ],
        mixed_clang_inputs=[
            PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_observer.c",
            PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_compound_identity_main.c",
        ],
        note=(
            "wave354 reverse mixed oracle: Clang-built consumers must observe and mutate "
            "one stable file-scope compound-literal object emitted by fisiCs"
        ),
        clang_args=STRICT_CLANG_ARGS,
    ),
    RuntimeProbe(
        probe_id="14__probe_runtime_wave354_file_scope_const_compound_identity_readonly",
        source=PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_const_compound_identity_main.c",
        inputs=[
            PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_const_compound_identity_main.c",
            PROBE_DIR / "runtime/14__probe_runtime_wave354_file_scope_const_compound_identity_owner.c",
        ],
        note=(
            "wave354 const control: a const-qualified exported pointer to a const "
            "file-scope compound literal must retain stable identity and values across TUs"
        ),
        clang_args=STRICT_CLANG_ARGS,
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
