from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="10__probe_wave67_external_inline_definition_canonical_order",
        source=PROBE_DIR / "runtime/10__probe_wave67_external_inline_main.c",
        note="wave67 strict: one C99 external definition must satisfy calls from three translation units that share a header inline definition in canonical source order",
        promoted_test_id="10__runtime__wave67_external_inline_definition_canonical_order",
        inputs=[
            PROBE_DIR / "runtime/10__probe_wave67_external_inline_main.c",
            PROBE_DIR / "runtime/10__probe_wave67_external_inline_provider.c",
            PROBE_DIR / "runtime/10__probe_wave67_external_inline_observer.c",
        ],
    ),
    RuntimeProbe(
        probe_id="10__probe_wave67_external_inline_definition_permuted_mixed_clang_provider",
        source=PROBE_DIR / "runtime/10__probe_wave67_external_inline_main.c",
        note="wave67 strict permutation: fisiCs-built inline callers must link after the observer source against the one C99 external definition staged from a Clang-built provider object",
        promoted_test_id="10__runtime__wave67_external_inline_definition_permuted_mixed_clang_provider",
        inputs=[
            PROBE_DIR / "runtime/10__probe_wave67_external_inline_main.c",
            PROBE_DIR / "runtime/10__probe_wave67_external_inline_observer.c",
        ],
        mixed_clang_inputs=[
            PROBE_DIR / "runtime/10__probe_wave67_external_inline_provider.c",
        ],
    ),
    RuntimeProbe(
        probe_id="10__probe_wave67_static_inline_internal_linkage_control",
        source=PROBE_DIR / "runtime/10__probe_wave67_static_inline_control_main.c",
        note="wave67 reduced control: static inline definitions included by two translation units retain independent internal linkage while producing Clang-parity runtime behavior",
        promoted_test_id="10__runtime__wave67_static_inline_internal_linkage_control",
        inputs=[
            PROBE_DIR / "runtime/10__probe_wave67_static_inline_control_main.c",
            PROBE_DIR / "runtime/10__probe_wave67_static_inline_control_lib.c",
        ],
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
