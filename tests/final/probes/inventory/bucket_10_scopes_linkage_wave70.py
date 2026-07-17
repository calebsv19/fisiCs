from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
STRICT_CLANG_ARGS = ["-std=c99", "-pedantic-errors", "-Wall", "-Wextra"]
MAIN = PROBE_DIR / "runtime/10__probe_wave70_equal_compound_identity_main.c"
OWNER = PROBE_DIR / "runtime/10__probe_wave70_equal_compound_identity_owner.c"
PEER = PROBE_DIR / "runtime/10__probe_wave70_equal_compound_identity_peer.c"

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="10__probe_wave70_equal_compound_identity",
        source=MAIN,
        inputs=[MAIN, OWNER, PEER],
        note=(
            "wave70 strict: equal-valued file-scope compound-literal occurrences "
            "owned through external and same-spelled internal-linkage pointer "
            "bindings must retain four separate identities and isolated mutations"
        ),
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="10__runtime__wave70_equal_compound_identity",
    ),
    RuntimeProbe(
        probe_id="10__probe_wave70_equal_compound_identity_reverse_order",
        source=MAIN,
        inputs=[PEER, OWNER, MAIN],
        note=(
            "wave70 order oracle: reversing the three translation-unit order must "
            "preserve distinct equal-valued compound-literal objects across internal "
            "and external linkage boundaries"
        ),
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="10__runtime__wave70_equal_compound_identity_reverse_order",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
